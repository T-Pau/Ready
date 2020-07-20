/*
 videoarch.m -- Video Output
 Copyright (C) 2019 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MY_MAX(a, b) ((a) > (b) ? (a) : (b))

static int lightpen_x;
static int lightpen_y;
static int lightpen_buttons;

static int canvas_index;

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
    canvas_index = 0;
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

    canvas->index = canvas_index++;

    return canvas;
}


/** \brief Free a previously created video canvas and all its
 *         components.
 *  \param[in] canvas The canvas to destroy.
 */
void video_canvas_destroy(struct video_canvas_s *canvas) {
    [viceThread.renderer close];
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
    Renderer *renderer = viceThread.renderers[canvas->index];

    RendererRect screen;
    screen.origin.x = canvas->geometry->gfx_position.x - canvas->viewport->first_x;
    screen.origin.y = canvas->geometry->gfx_position.y - canvas->geometry->first_displayed_line;
    screen.size.width = canvas->geometry->gfx_size.width;
    screen.size.height = canvas->geometry->gfx_size.height;
    renderer.screenPosition = screen;
    
    RendererImage image;
    RendererPoint offset = {xi, yi};
     
    image.data = canvas->draw_buffer->draw_buffer + canvas->draw_buffer->draw_buffer_pitch * ys + xs;
    image.rowSize = canvas->draw_buffer->draw_buffer_pitch;
    image.size.width = w;
    image.size.height = h;

    [renderer render:&image at:offset];
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
    
    size_t width = MY_MIN(canvas->geometry->screen_size.width - canvas->viewport->first_x, canvas->draw_buffer->canvas_width);
    size_t height = canvas->viewport->last_line - canvas->viewport->first_line + 1;

//    int height = canvas->viewport->last_line - canvas->viewport->first_line + 1;
//    int width = canvas->geometry->screen_size.width;

    RendererSize size = { width, height };
    Renderer *renderer = viceThread.renderers[canvas->index];
    [renderer resize:size];
    renderer.palette = palette;
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

    for (size_t i = 0; i < viceThread.renderers.count; i++) {
        Renderer *renderer = viceThread.renderers[i];
        [renderer displayImage];
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

#ifndef VICE_PLUS4
void update_light_pen(int x_in, int y_in, int width, int height, int button_1, int button_2, int is_koala_pad) {
    if (is_koala_pad) {
        mouse_button_press(1, button_1);
        mouse_button_press(2, button_2);
    }
    else {
        lightpen_buttons = (button_1 ? LP_HOST_BUTTON_1 : 0) | (button_2 ? LP_HOST_BUTTON_2 : 0);
    }
    
    if (x_in > 0 && y_in > 0) {
        const RendererSize current_size = viceThread.renderer.currentSize;
        
        double scale = MY_MIN((double)width / current_size.width, (double)height / current_size.height);
        int x_offset = (width - current_size.width * scale) / 2;
        int y_offset = (height - current_size.height * scale) / 2;
        
        double x = (x_in - x_offset) / scale;
        double y = (y_in - y_offset) / scale;
        
        if (x >= 0 && x < current_size.width && y >= 0 && y < current_size.height) {
            if (is_koala_pad) {
                viceThread.mouseX = x / current_size.width * (225 - 44) + 45;
                viceThread.mouseY = (1 - y / current_size.height) * (204 - 6) + 7;
            }
            else {
                const RendererPoint current_offset = viceThread.renderer.currentOffset;
                lightpen_x = x + current_offset.x;
                lightpen_y = y + current_offset.y;
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
