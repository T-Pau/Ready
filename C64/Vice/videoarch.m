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

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MY_MAX(a, b) ((a) > (b) ? (a) : (b))

unsigned int border_animation[] = {
    1, 1,
    2, 2,  2, 2,
    3, 3, 3,  3, 3, 3,  3, 3, 3,
    4
};
int num_animation = sizeof(border_animation) / sizeof(border_animation[0]);

static int lightpen_x;
static int lightpen_y;
static int lightpen_buttons;

/* For border auto hiding */
/* Number of consecutive frames border must not change until it's hidden */
#define BORDER_HIDE_FRAMES 25
/* Number of consecutive frames border must change until it's shown */
#define BORDER_SHOW_FRAMES 5

#define TICKSPERSECOND  1000000000L  /* Nanoseconds resolution. */

/* taken from https://www.pepto.de/projects/colorvic/ */
static uint32_t c64_palette[] = {
    0x000000FF,
    0xFFFFFFFF,
    0x813338FF,
    0x75CEC8FF,
    0x8E3C97FF,
    0x56AC4DFF,
    0x2E2C9BFF,
    0xEDF171FF,
    0x8E5029FF,
    0x553800FF,
    0xC46C71FF,
    0x4A4A4AFF,
    0x7B7B7BFF,
    0xA9FF9FFF,
    0x706DEBFF,
    0xB2B2B2FF
};

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

    
    canvas->show_border = viceThread.currentBorderMode == BORDER_MODE_SHOW ? 1 : 0;
    canvas->border_width = 0xff;
    canvas->border_color = 0xe; // light blue
    canvas->transition_countdown = BORDER_SHOW_FRAMES;

    return canvas;
}


/** \brief Free a previously created video canvas and all its
 *         components.
 *  \param[in] canvas The canvas to destroy.
 */
void video_canvas_destroy(struct video_canvas_s *canvas) {
}


// get color of border, or 0xff if it contains more than one color
static uint8_t get_border_color(struct video_canvas_s *canvas) {
    /*
     geometry:
     screen size - incl. border
     gfx_size - excl. border
     gfx_position - offset of screen w/o border
     first_displayed_line / last_displayed_line - ignore parts outside
     */

    struct geometry_s *geometry = canvas->geometry;
    
    unsigned int width = geometry->screen_size.width;
    unsigned int height = geometry->last_displayed_line - geometry->first_displayed_line + 1;
    unsigned int top_border_end = geometry->gfx_position.y - geometry->first_displayed_line;
    unsigned int bottom_border_start = top_border_end + geometry->gfx_size.height;
    unsigned int left_border_end = geometry->gfx_position.x;
    unsigned int right_border_start = left_border_end + geometry->gfx_size.width;
    
    unsigned int pitch = canvas->draw_buffer->draw_buffer_pitch;
    uint8_t *buffer = canvas->draw_buffer->draw_buffer + pitch * geometry->first_displayed_line;
    
    uint8_t color = buffer[0];
    
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            if (x == left_border_end && y >= top_border_end && y < bottom_border_start) {
                x = right_border_start;
            }
            if (buffer[x + y * pitch] != color) {
//                printf("%xx%x: %x != %x\n", x, y, buffer[x + y * pitch], color);
                return 0xff;
            }
        }
    }
    
    return color;
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
    
    if (viceThread.newBorderMode != viceThread.currentBorderMode) {
        switch (viceThread.newBorderMode) {
            case BORDER_MODE_AUTO: {
                if (canvas->show_border) {
                    if (canvas->border_color != 0xff) {
                        canvas->show_border = false;
                        canvas->transition_countdown = BORDER_SHOW_FRAMES;
                    }
                    else {
                        canvas->transition_countdown = BORDER_HIDE_FRAMES;
                    }
                }
                else {
                    if (canvas->border_color != 0xff) {
                        canvas->transition_countdown = BORDER_SHOW_FRAMES;
                    }
                    else {
                        canvas->show_border = true;
                        canvas->transition_countdown = BORDER_HIDE_FRAMES;
                    }
                }
                break;
            }
                
            case BORDER_MODE_HIDE:
                canvas->show_border = false;
                break;
                
            case BORDER_MODE_SHOW:
                canvas->show_border = true;
                break;
        }
        
        viceThread.currentBorderMode = viceThread.newBorderMode;
    }
    
    if (viceThread.currentBorderMode == BORDER_MODE_AUTO) {
        uint8_t border_color = get_border_color(canvas);
        uint8_t border_changed = (border_color == 0xff || border_color != canvas->border_color);
        canvas->border_color = border_color;
        
        if (canvas->show_border == border_changed) {
            /* reset transition countdown */
            canvas->transition_countdown = border_changed ? BORDER_HIDE_FRAMES : BORDER_SHOW_FRAMES;
        }
        else {
            canvas->transition_countdown--;
            printf("transition to %s in %u\n", canvas->show_border ? "hide" : "show", canvas->transition_countdown);
            if (canvas->transition_countdown == 0) {
                canvas->show_border = !canvas->show_border;
                printf("%s border\n", canvas->show_border ? "show" : "hide");
                canvas->transition_countdown = border_changed ? BORDER_HIDE_FRAMES : BORDER_SHOW_FRAMES;
            }
        }
    }
    
    unsigned int left_border = canvas->geometry->gfx_position.x - xs;
    unsigned int right_border = w - left_border - canvas->geometry->gfx_size.width;
    unsigned int top_border = canvas->geometry->gfx_position.y - ys;
    unsigned int bottom_border = h - top_border - canvas->geometry->gfx_size.height;
    unsigned int max_border = MY_MAX(left_border, right_border);
    max_border = MY_MAX(max_border, top_border);
    max_border = MY_MAX(max_border, bottom_border);
    
    if (canvas->border_width == 0xff) {
        if (canvas->show_border) {
            canvas->border_width = max_border;
        }
        else {
            canvas->border_width = 0;
        }
    }

    unsigned int diff = MY_MIN(canvas->border_width, max_border - canvas->border_width);
    unsigned int step = border_animation[MY_MIN(diff, num_animation - 1)];
    
    if (canvas->show_border) {
        if (canvas->border_width < max_border) {
            canvas->border_width += step;
        }
    }
    else {
        if (canvas->border_width > 0) {
            canvas->border_width -= step;
        }
    }
    
    canvas->border_offset.x = left_border;
    canvas->border_offset.y = top_border;
    canvas->current_offset.x = MY_MIN(left_border, canvas->border_width);
    canvas->current_offset.y = MY_MIN(top_border, canvas->border_width);
    canvas->current_size.width = canvas->geometry->gfx_size.width + canvas->current_offset.x + MY_MIN(right_border, canvas->border_width);
    canvas->current_size.height = canvas->geometry->gfx_size.height + canvas->current_offset.y + MY_MIN(bottom_border, canvas->border_width);
    xs = canvas->geometry->gfx_position.x - canvas->current_offset.x;
    ys = canvas->geometry->gfx_position.y - canvas->current_offset.y;

    uint8_t *source = canvas->draw_buffer->draw_buffer + canvas->draw_buffer->draw_buffer_pitch * ys + xs;
    uint32_t *destination = canvas->bitmapData + canvas->bitmapWidth * yi + xi;
    for (int y = 0; y < canvas->current_size.height; y++) {
        for (int x = 0; x < canvas->current_size.width; x++) {
            destination[x] = c64_palette[source[x]];
        }
        
        source += canvas->draw_buffer->draw_buffer_pitch;
        destination += canvas->bitmapWidth;
    }
    
    [viceThread updateBitmapWidth: canvas->current_size.width height: canvas->current_size.height];
    
}



/** \brief Update canvas size to match the draw buffer size requested
 *         by the emulation core.
 * \param canvas The video canvas to update.
 * \param resize_canvas Ignored - the canvas will always resize.
 */

void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas) {
    int height = canvas->viewport->last_line - canvas->viewport->first_line + 1;
    int width = canvas->draw_buffer->draw_buffer_width;

    viceThread.bytesPerRow = width * 4;
    viceThread.imageData = [NSMutableData dataWithLength:width * height * 4];
    
    canvas->bitmapData = viceThread.imageData.mutableBytes;
    canvas->bitmapWidth = width;
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
        double scale = MY_MIN((double)width / canvas->current_size.width, (double)height / canvas->current_size.height);
        int x_offset = (width - canvas->current_size.width * scale) / 2;
        int y_offset = (height - canvas->current_size.height * scale) / 2;
        
        double x = (x_in - x_offset) / scale;
        double y = (y_in - y_offset) / scale;
        
        if (x >= 0 && x < canvas->current_size.width && y >= 0 && y < canvas->current_size.height) {
            if (is_koala_pad) {
                viceThread.mouseX = x / canvas->current_size.width * (225 - 44) + 45;
                viceThread.mouseY = (1 - y / canvas->current_size.height) * (204 - 6) + 7;
            }
            else {
                lightpen_x = x - canvas->current_offset.x + canvas->border_offset.x;
                lightpen_y = y - canvas->current_offset.y + canvas->border_offset.y;
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
