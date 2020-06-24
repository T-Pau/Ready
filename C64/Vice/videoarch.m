/*
 videoarch.m -- Video Output
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

#include "videoarch.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mach/mach_time.h>

#include "joy.h"
#include "palette.h"
#include "video.h"
#include "viewport.h"
#include "archdep.h"
#include "kbdbuf.h"
#include "machine.h"
#include "viciitypes.h"
#include "lightpen.h"
#include "mousedrv.h"

#import <CoreImage/CoreImage.h>
#import "ViceThread.h"
#import "ViceThreadC.h"
#include "render.h"

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MY_MAX(a, b) ((a) > (b) ? (a) : (b))

static int lightpen_x;
static int lightpen_y;
static int lightpen_buttons;

/* For border auto hiding */
/* Number of consecutive frames border must not change until it's hidden */
#define BORDER_HIDE_FRAMES 25
/* Number of consecutive frames border must change until it's shown */
#define BORDER_SHOW_FRAMES 5

#define TICKSPERSECOND  1000000000L  /* Nanoseconds resolution. */

extern uint32_t palette[];


/** \brief  Arch-specific initialization for a video canvas
 *  \param[in,out] canvas The canvas being initialized
 *  \sa video_canvas_create
 */
void video_arch_canvas_init(struct video_canvas_s *canvas) {
    lightpen_x = -1;
    lightpen_y = -1;
    lightpen_buttons = 0;
}


/** \brief  Initialize command line options for generic video resouces
 *
 * \return  0 on success, < 0 on failure
 */
int video_arch_cmdline_options_init(void) {
    return 0;
}


/** \brief  Initialize video-related resources
 *
 * \return  0 on success, < on failure
 */
int video_arch_resources_init(void) {
    return 0;
}


/** \brief Clean up any memory held by arch-specific video resources. */
void video_arch_resources_shutdown(void) {
}


/** \brief Query whether a canvas is resizable.
 *  \param canvas The canvas to query
 *  \return TRUE if the canvas can be resized.
 */
char video_canvas_can_resize(video_canvas_t *canvas)
{
    return 1;
}


/** \brief Create a new video_canvas_s.
 *  \param[in,out] canvas A freshly allocated canvas object.
 *  \param[in]    width  Pointer to a width value. May be NULL if canvas
 *                       size is not yet known.
 *  \param[in]    height Pointer to a height value. May be NULL if canvas
 *                       size is not yet known.
 *  \param        mapped Unused.
 *  \return The completely initialized canvas. The window that holds
 *          it will be visible in the UI at time of return.
 */
video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped) {
    canvas->created = 1;
    canvas->initialized = 1;

    canvas->render = NULL;
    canvas->bitmap = NULL;
    canvas->bitmap_row_size = 0;

    return canvas;
}


/** \brief Free a previously created video canvas and all its
 *         components.
 *  \param[in] canvas The canvas to destroy.
 */
void video_canvas_destroy(struct video_canvas_s *canvas) {
    render_free(canvas->render);
}




/** \brief Update the display on a video canvas to reflect the machine
 *         state.
 * \param canvas The canvas to update.
 * \param xs     A parameter to forward to video_canvas_render()
 * \param ys     A parameter to forward to video_canvas_render()
 * \param xi     X coordinate of the leftmost pixel to update
 * \param yi     Y coordinate of the topmost pixel to update
 * \param w      Width of the rectangle to update
 * \param h      Height of the rectangle to update
 */
void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h) {
    
    size_t full_width = MY_MIN(canvas->geometry->screen_size.width - canvas->viewport->first_x, canvas->draw_buffer->canvas_width);
    size_t full_height = canvas->viewport->last_line - canvas->viewport->first_line + 1;
    
    render_image_t image;
    
    uint8_t *source = canvas->draw_buffer->draw_buffer + canvas->draw_buffer->draw_buffer_pitch * ys + xs;

    if (canvas_has_partial_updates == 0) {
        image.data = source;
        image.row_size = canvas->draw_buffer->draw_buffer_pitch;
    }
    else {
        const render_size_t *size = render_get_bitmap_size(canvas->render);
        if (canvas->bitmap == NULL) {
            canvas->bitmap = calloc(size->width * size->height, 1);
            canvas->bitmap_row_size = size->width;
        }
        
        xi = MY_MIN(xi, size->width);
        yi = MY_MIN(yi, size->height);
        w = MY_MIN(w, size->width - xi);
        h = MY_MIN(h, size->height - yi);
        uint8_t *destination = canvas->bitmap + yi * canvas->bitmap_row_size + xi;
        
        for (size_t y = 0; y < h; y++) {
            memcpy(destination, source, w);
            source += canvas->draw_buffer->draw_buffer_pitch;
            destination += canvas->bitmap_row_size;
        }
        
        image.data = canvas->bitmap;
        image.row_size = canvas->bitmap_row_size;
    }
    
    image.size.width = full_width;
    image.size.height = full_height;
    image.screen.origin.x = canvas->geometry->gfx_position.x - canvas->viewport->first_x;
    image.screen.origin.y = canvas->geometry->gfx_position.y - canvas->geometry->first_displayed_line;
    image.screen.size.width = canvas->geometry->gfx_size.width;
    image.screen.size.height = canvas->geometry->gfx_size.height;

    const render_size_t *current_size = render(canvas->render, &image, viceThread.newBorderMode);
    if (current_size != NULL) {
        [viceThread updateBitmapWidth: current_size->width height: current_size->height];
    }
    viceThread.currentBorderMode = viceThread.newBorderMode;
}



/** \brief Update canvas size to match the draw buffer size requested
 *         by the emulation core.
 * \param canvas The video canvas to update.
 * \param resize_canvas Ignored - the canvas will always resize.
 */

void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas) {
#if 0
    printf("creating canvas:\n");
    printf("  draw_buffer:\n");
    printf("    canvas_width/height: (%d, %d)\n", canvas->draw_buffer->canvas_width, canvas->draw_buffer->canvas_height);
    printf("    canvas_pysical_width/height: (%d, %d)\n", canvas->draw_buffer->canvas_physical_width, canvas->draw_buffer->canvas_physical_height);
    printf("    draw_buffer_width/height: (%d, %d)\n", canvas->draw_buffer->draw_buffer_width, canvas->draw_buffer->draw_buffer_height);
    printf("    draw_buffer_pitch: %d\n", canvas->draw_buffer->draw_buffer_pitch);
    printf("  geometry:\n");
    printf("    char_pixel_width: %d\n", canvas->geometry->char_pixel_width);
    printf("    extra_offscreen_border_left/right: %d / %d\n", canvas->geometry->extra_offscreen_border_left, canvas->geometry->extra_offscreen_border_right);
    printf("    first/last_displayed_line: %d / %d\n", canvas->geometry->first_displayed_line, canvas->geometry->last_displayed_line);
    printf("    gfx_area_moves: %d\n", canvas->geometry->gfx_area_moves);
    printf("    gfx_position: (%d, %d)\n", canvas->geometry->gfx_position.x, canvas->geometry->gfx_position.y);
    printf("    gfx_size: (%d, %d)\n", canvas->geometry->gfx_size.width, canvas->geometry->gfx_size.height);
    printf("    pixel_aspect_ratio: %g\n", canvas->geometry->pixel_aspect_ratio);
    printf("    screen_size: (%d, %d)\n", canvas->geometry->screen_size.width, canvas->geometry->screen_size.height);
    printf("    text_size: (%d, %d)\n", canvas->geometry->text_size.width, canvas->geometry->text_size.height);
    printf("  viewport:\n");
    printf("    title: %s\n", canvas->viewport->title);
    printf("    first/last_line: %d / %d\n", canvas->viewport->first_line, canvas->viewport->last_line);
    printf("    first_x: %d\n", canvas->viewport->first_x);
    printf("    x/y_offset: (%d, %d)\n", canvas->viewport->x_offset, canvas->viewport->y_offset);
#endif
    
    int height = canvas->viewport->last_line - canvas->viewport->first_line + 1;
    int width = canvas->geometry->screen_size.width;

    render_free(canvas->render);
    
    render_size_t size = { width, height };
    viceThread.bytesPerRow = width * 4;
    viceThread.imageData = [NSMutableData dataWithLength:render_data_size(size)];
    
    if ((canvas->render = render_new(size, viceThread.imageData.mutableBytes, palette, viceThread.currentBorderMode)) == NULL) {
        printf("can't create render\n");
    }
}


/** \brief Assign a palette to the canvas.
 * \param canvas The canvas to update the palette
 * \param palette The new palette to assign
 * \return Zero on success, nonzero on failure
 */
int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette) {
    canvas->palette = palette;
    return 0;
}


/** \brief Perform any frontend-specific initialization.
 *  \return 0 on success, nonzero on failure
 */
int video_init(void) {
   // archdep_set_resources();
    return 0;
}


/** \brief Perform any frontend-specific uninitialization. */
void video_shutdown(void) {
}


/* Get time in timer units. */
unsigned long vsyncarch_gettime(void)
{
    static uint64_t factor = 0;
    uint64_t time = mach_absolute_time();
    if (!factor) {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        factor = info.numer / info.denom;
    }
    return time * factor;
}


/* Display speed (percentage) and frame rate (frames per second). */
void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled) {
}


/* Number of timer units per second. */
unsigned long vsyncarch_frequency(void)
{
    return TICKSPERSECOND;
}


void vsyncarch_init(void) {
}


/* this is called after vsync_do_vsync did the synchroniation */
void vsyncarch_postsync(void) {
}


void vsyncarch_presync(void) {
    if (!viceThread.vsync) {
        return;
    }
    
    lightpen_update(0, lightpen_x, lightpen_y, lightpen_buttons);
    kbdbuf_flush();
}


/* for error measurement */
static unsigned long delay_error;

/* Sleep a number of timer units. */
void vsyncarch_sleep(unsigned long delay)
{
    struct timespec ts;
    unsigned long thistime, timewait, targetdelay;

    thistime = vsyncarch_gettime();
    
    /* compensate for delay inaccuracy */
    targetdelay = (delay <= delay_error) ? delay : (delay - delay_error);
    
    /* repeatedly sleep until the requested delay is over. we do this so we get
     a somewhat accurate delay even if the sleep function itself uses the
     wall clock, which under certain circumstance may wait less than the
     requested time */
    for (;;) {
        timewait = vsyncarch_gettime() - thistime;
        
        if (timewait >= targetdelay) {
            delay_error = (delay_error * 3 + timewait - targetdelay + 1) / 4;
            break;
        }
        timewait = targetdelay - timewait;
        
        if (timewait < TICKSPERSECOND) {
            ts.tv_sec = 0;
            ts.tv_nsec = timewait;
        } else {
            ts.tv_sec = timewait / TICKSPERSECOND;
            ts.tv_nsec = (timewait % TICKSPERSECOND);
        }
        nanosleep(&ts, NULL);
    }
}

#ifdef VICE_C64
void update_light_pen(int x_in, int y_in, int width, int height, int button_1, int button_2, int is_koala_pad) {
    video_canvas_t *canvas = vicii.raster.canvas;
    
    if (is_koala_pad) {
        mouse_button_press(1, button_1);
        mouse_button_press(2, button_2);
    }
    else {
        lightpen_buttons = (button_1 ? LP_HOST_BUTTON_1 : 0) | (button_2 ? LP_HOST_BUTTON_2 : 0);
    }
    
    if (x_in > 0 && y_in > 0) {
        const render_size_t *current_size = render_get_current_size(canvas->render);
//        const render_rect_t *current_screen = render_get_current_screen(canvas->render);
        
        double scale = MY_MIN((double)width / current_size->width, (double)height / current_size->height);
        int x_offset = (width - current_size->width * scale) / 2;
        int y_offset = (height - current_size->height * scale) / 2;
        
        double x = (x_in - x_offset) / scale;
        double y = (y_in - y_offset) / scale;
        
        if (x >= 0 && x < current_size->width && y >= 0 && y < current_size->height) {
            if (is_koala_pad) {
                viceThread.mouseX = x / current_size->width * (225 - 44) + 45;
                viceThread.mouseY = (1 - y / current_size->height) * (204 - 6) + 7;
            }
            else {
                const render_point_t *current_offset = render_get_current_offset(canvas->render);
                lightpen_x = x + current_offset->x;
                lightpen_y = y + current_offset->y;
                /* printf("light pen at %dx%d, buttons: %d\n", lightpen_x, lightpen_y, lightpen_buttons); */
            }
            return;
        }
    }
    
    if (is_koala_pad) {
        viceThread.mouseX = 0xff;
        viceThread.mouseY = 0xff;
    }
    else {
        lightpen_x = -1;
        lightpen_y = -1;
    }
}
#else
void update_light_pen(int x_in, int y_in, int width, int height, int button_1, int button_2, int is_koala_pad) {
}
#endif
