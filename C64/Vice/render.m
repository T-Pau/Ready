//
//  render.c
//  C64
//
//  Created by Dieter Baron on 18.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "render.h"

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BORDER_COLOR_UNKNOWN (-1)
#define BORDER_COLOR_MULTIPLE (-2)

#define BORDER_WIDTH_MAX 0xffff

/* For border auto hiding */
/* Number of consecutive frames border must not change until it's hidden */
#define BORDER_HIDE_FRAMES 25
/* Number of consecutive frames border must change until it's shown */
#define BORDER_SHOW_FRAMES 5


size_t border_animation[] = {
    1, 1,
    2, 2,  2, 2,
    3, 3, 3,  3, 3, 3,  3, 3, 3,
    4
};
size_t num_animation = sizeof(border_animation) / sizeof(border_animation[0]);


struct render {
    const uint32_t *palette;
    uint32_t *data;
    render_size_t bitmap_size;
    
    render_border_mode_t border_mode;
    
    render_size_t current_size;     /* current size of rendered image */
    render_point_t current_offset;  /* curront offset to source image du to border hiding */
    render_rect_t current_screen;   /* current position of screen with in image */
    
    /* For border auto hiding */
    int last_border_color;      /* color of the border in last frame*/
    bool show_border;           /* wether we currently show the border */
    int transition_countdown;   /* number of frames until transition */
    size_t border_width;        /* current widht of displayed border (used during transition animation) */
};


static int get_border_color(const render_image_t *image, const uint32_t *palette);


const render_size_t *render(render_t *render, const render_image_t *image, render_border_mode_t border_mode) {
    if (image->size.width > render->bitmap_size.width || image->size.width > render->bitmap_size.width) {
        printf("image (%zu, %zu) bigger than bitmap (%zu, %zu)\n", image->size.width, image->size.height, render->bitmap_size.width, render->bitmap_size.height);
        return NULL;
    }
    if (image->screen.origin.x + image->screen.size.width > image->size.width || image->screen.origin.y + image->screen.size.height > image->size.height) {
        printf("screen partly outside image\n");
        return NULL;
    }
    
    if (border_mode != render->border_mode) {
        switch (border_mode) {
        case BORDER_MODE_AUTO:
            if (render->show_border) {
                if (render->last_border_color >= 0) {
                    render->show_border = false;
                    render->transition_countdown = BORDER_SHOW_FRAMES;
                }
                else {
                    render->transition_countdown = BORDER_HIDE_FRAMES;
                }
            }
            else {
                if (render->last_border_color >= 0) {
                    render->transition_countdown = BORDER_SHOW_FRAMES;
                }
                else {
                    render->show_border = true;
                    render->transition_countdown = BORDER_HIDE_FRAMES;
                }
            }
            break;
            
        case BORDER_MODE_HIDE:
            render->show_border = false;
            break;
            
        case BORDER_MODE_SHOW:
            render->show_border = true;
            break;
        }
    }
    
    render->border_mode = border_mode;
    
    if (border_mode == BORDER_MODE_AUTO) {
        int border_color = get_border_color(image, render->palette);
        
        bool border_changed = (border_color == BORDER_COLOR_MULTIPLE || border_color != render->last_border_color);
        render->last_border_color = border_color;
        
        if (render->show_border == border_changed) {
            /* reset transition countdown */
            render->transition_countdown = border_changed ? BORDER_MODE_HIDE : BORDER_SHOW_FRAMES;
        }
        else {
            render->transition_countdown -= 1;
            if (render->transition_countdown == 0) {
                render->show_border = !render->show_border;
                render->transition_countdown = render->show_border ? BORDER_HIDE_FRAMES : BORDER_SHOW_FRAMES;
            }
        }
    }
    
    size_t full_left_border = image->screen.origin.x;
    size_t full_right_border = image->size.width - (image->screen.origin.x + image->screen.size.width);
    size_t full_top_border = image->screen.origin.y;
    size_t full_bottom_border = image->size.height - (image->screen.origin.y + image->screen.size.height);

    size_t border_max = full_left_border;
    if (full_right_border > border_max) {
        border_max = full_right_border;
    }
    if (full_top_border > border_max) {
        border_max = full_top_border;
    }
    if (full_bottom_border > border_max) {
        border_max = full_bottom_border;
    }
    
    if (render->border_width == BORDER_WIDTH_MAX) {
        if (render->show_border) {
            render->border_width = border_max;
        }
        else {
            render->border_width = 0;
        }
    }
    
    size_t diff = MY_MIN(render->border_width, border_max - render->border_width);
    size_t step = border_animation[MY_MIN(diff, num_animation - 1)];
    
    if (render->show_border) {
        if (render->border_width < border_max) {
            render->border_width += step;
        }
    }
    else {
        if (render->border_width > 0) {
            render->border_width -= step;
        }
    }
    
    size_t left_border = MY_MIN(full_left_border, render->border_width);
    size_t right_border = MY_MIN(full_right_border, render->border_width);
    size_t top_border = MY_MIN(full_top_border, render->border_width);
    size_t bottom_border = MY_MIN(full_bottom_border, render->border_width);

    render->current_offset.x = full_left_border - left_border;
    render->current_offset.y = full_top_border - top_border;
    render->current_screen.origin.x = left_border;
    render->current_screen.origin.y = top_border;
    render->current_screen.size = image->screen.size;
    render->current_size.width = left_border + image->screen.size.width + right_border;
    render->current_size.height = top_border + image->screen.size.height + bottom_border;
    
    size_t x0 = image->screen.origin.x - left_border;
    size_t y0 = image->screen.origin.y - top_border;
    
    const uint8_t *source = image->data + y0 * image->row_size + x0;
    uint32_t *destination = render->data;
    
    for (size_t y = 0; y < render->current_size.height; y++) {
        for (size_t x = 0; x < render->current_size.width; x++) {
            destination[x] = render->palette[source[x]];
        }
        
        source += image->row_size;
        destination += render->bitmap_size.width;
    }
    
    return &render->current_size;
}


size_t render_data_size(const render_size_t size) {
    return size.width * size.height * sizeof(uint32_t);
}


void render_free(render_t *render) {
    free(render);
}


const render_size_t *render_get_bitmap_size(const render_t *render) {
    return &render->bitmap_size;
}


const render_point_t *render_get_current_offset(const render_t *render) {
    return &render->current_offset;
}


const render_rect_t *render_get_current_screen(const render_t *render) {
    return &render->current_screen;
}


const render_size_t *render_get_current_size(const render_t *render) {
    return &render->current_size;
}


render_t *render_new(render_size_t size, void *data, const uint32_t *palette, render_border_mode_t border_mode) {
    render_t *render = malloc(sizeof(*render));
    if (render == NULL) {
        return NULL;
    }
    
    render->data = data;
    render->bitmap_size = size;
    render->palette = palette;
    
    render->border_mode = border_mode;
    render->last_border_color = BORDER_COLOR_UNKNOWN;
    if (border_mode == BORDER_MODE_SHOW) {
        render->show_border = true;
        render->transition_countdown = BORDER_HIDE_FRAMES;
    }
    else {
        render->show_border = false;
        render->transition_countdown = BORDER_SHOW_FRAMES;
    }
    render->border_width = BORDER_WIDTH_MAX;
    
    return render;
}


void render_set_palette(render_t *render, const uint32_t *palette) {
    render->palette = palette;
}


static int get_border_color(const render_image_t *image, const uint32_t *palette) {
    
    int border_color = BORDER_COLOR_UNKNOWN;
    
    for (size_t y = 0; y < image->size.height; y++) {
        for (size_t x = 0; x < image->size.width; x++) {
            if (x == image->screen.origin.x && y >= image->screen.origin.y && y < image->screen.origin.y + image->screen.size.height) {
                x = image->screen.origin.x + image->screen.size.width;
            }
            int color = image->data[x + y * image->row_size];
            if (border_color == BORDER_COLOR_UNKNOWN) {
                border_color = color;
            }
            else if (border_color != color) {
                return BORDER_COLOR_MULTIPLE;
            }
        }
    }
    
    if (border_color == BORDER_COLOR_UNKNOWN) {
        return border_color;
    }
    return palette[border_color];
}


void render_image_free(render_image_t *image) {
    if (image) {
        free(image->data);
        free(image);
    }
}

render_image_t *render_image_new(size_t width, size_t height) {
    render_image_t *image;
    
    if ((image = malloc(sizeof(*image))) == NULL) {
        return NULL;
    }
    if ((image->data = calloc(width * height, 1)) == NULL) {
        free(image);
        return NULL;
    }
    
    image->row_size = width;
    image->size.width = width;
    image->size.height = height;
    image->row_size = width;
    image->screen.origin.x = 0;
    image->screen.origin.y = 0;
    image->screen.size.width = width;
    image->screen.size.height = height;

    return image;
}
