/* uidisplay-ios.c: Display Routines for Integration with Ready
 Copyright (C) 2020 Dieter Baron
 
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

#include "fuse.h"
#include "ui/ui.h"

#import <CoreImage/CoreImage.h>
#import "FuseThread.h"

static RendererImage *screen;

static uint32_t palette[] = {
    0x000000FF,
    0x0000D7FF,
    0xD70000FF,
    0xD700D7FF,
    0x00D700FF,
    0x00D7D7FF,
    0xD7D700FF,
    0xD7D7D7FF,
    0x000000FF,
    0x0000FFFF,
    0xFF0000FF,
    0xFF00FFFF,
    0x00FF00FF,
    0x00FFFFFF,
    0xFFFF00FF,
    0xFFFFFFFF
};

void uidisplay_area(int x, int y, int w, int h) {
    // area for optimization when render supports partial updates
}

int uidisplay_end(void) {
    renderer_image_free(screen);
    [fuseThread.renderer close];
    return 0;
}

void uidisplay_frame_end(void) {
    [fuseThread.renderer render:screen];
}

int uidisplay_hotswap_gfx_mode(void) {
    return 0;
}

int uidisplay_init(int width, int height) {
    if ((screen = renderer_image_new(width, height)) == NULL) {
        fuse_exiting = 1;
        return 0;
    }

    // TODO: handle doubled resolution for Timex machines (640x480)

    screen->screen.origin.x = 32;
    screen->screen.origin.y = 24;
    screen->screen.size.width = width - 64;
    screen->screen.size.height = height - 48;
    
    [fuseThread.renderer resize:screen->size];

    if (fuseThread.renderer == nil) {
        renderer_image_free(screen);
        fuse_exiting = 1;
        return 0;
    }
    
    fuseThread.renderer.palette = palette;
    
    display_ui_initialised = 1;
    
    return 0;
}


void uidisplay_plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper) {
    uint8_t *dest = screen->data + y * screen->rowSize + x * 8;
    
    for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        *dest = data & mask ? ink : paper;
        dest += 1;
    }
}


void uidisplay_plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper) {
    printf("put 16 pixels at (%d, %d)\n", x, y);
    uint8_t *dest = screen->data + y * screen->rowSize + x * 16;
    
    for (uint16_t mask = 0x8000; mask != 0; mask >>= 1) {
        *dest = data & mask ? ink : paper;
        dest += 1;
    }
}


void uidisplay_putpixel(int x, int y, int colour) {
    screen->data[y * screen->rowSize + x] = colour;
}
