/* uidisplay-ios.c: Display Routines for Integration with Ready
 Copyright (C) 2020 Dieter Baron
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "fuse.h"
#include "ui/ui.h"

#import <CoreImage/CoreImage.h>
#import "FuseThread.h"
#include "render.h"

static render_image_t *screen;
static render_t *renderer;

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
    render_image_free(screen);
    render_free(renderer);
    return 0;
}

void uidisplay_frame_end(void) {
    const render_size_t *current_size = render(renderer, screen, fuseThread.newBorderMode);
    [fuseThread updateBitmapWidth: current_size->width height: current_size->height];
    fuseThread.currentBorderMode = fuseThread.newBorderMode;
}

int uidisplay_hotswap_gfx_mode(void) {
    return 0;
}

int uidisplay_init(int width, int height) {
    if ((screen = render_image_new(width, height)) == NULL) {
        fuse_exiting = 1;
        return 0;
    }

    // TODO: handle doubled resolution for Timex machines (640x480)

    screen->screen.origin.x = 32;
    screen->screen.origin.y = 24;
    screen->screen.size.width = width - 64;
    screen->screen.size.height = height - 48;
    
    [fuseThread initBitmapWidth: width height: height];

    if ((renderer = render_new(screen->size, fuseThread.imageData.mutableBytes, palette, fuseThread.currentBorderMode)) == NULL) {
        render_image_free(screen);
        fuse_exiting = 1;
        return 0;
    }
    
    display_ui_initialised = 1;
    
    return 0;
}


void uidisplay_plot8(int x, int y, libspectrum_byte data, libspectrum_byte ink, libspectrum_byte paper) {
    uint8_t *dest = screen->data + y * screen->row_size + x * 8;
    
    for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
        *dest = data & mask ? ink : paper;
        dest += 1;
    }
}


void uidisplay_plot16(int x, int y, libspectrum_word data, libspectrum_byte ink, libspectrum_byte paper) {
    printf("put 16 pixels at (%d, %d)\n", x, y);
    uint8_t *dest = screen->data + y * screen->row_size + x * 16;
    
    for (uint16_t mask = 0x8000; mask != 0; mask >>= 1) {
        *dest = data & mask ? ink : paper;
        dest += 1;
    }
}


void uidisplay_putpixel(int x, int y, int colour) {
    screen->data[y * screen->row_size + x] = colour;
}
