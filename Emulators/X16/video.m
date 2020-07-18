/*
 video.m -- Platform Specific Video Functions for X16 Emulator
 Copyright (C) 2019-2020 Dieter Baron
 
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

#import <Foundation/Foundation.h>

#import "X16Thread.h"

#include "debugger.h"
#include "video.h"
#include "platform.h"

static RendererSize screen_size = {640, 480};

bool video_init(int window_scale, char *quality) {
    video_reset();

    [x16Thread.renderer resize:screen_size];
    
    if (debugger_enabled) {
        //DEBUGInitUI(renderer);
    }

    return true;
}

bool platform_video_update(const uint32_t *framebuffer) {
    RendererImage image;
    
    image.data = (uint8_t *)framebuffer;
    image.rowSize = screen_size.width * sizeof(uint32_t);
    image.size = screen_size;
    image.screen.origin.x = 0;
    image.screen.origin.y = 0;
    image.screen.size = screen_size;
    
    [x16Thread.renderer renderRGB:&image];
    
    if (debugger_enabled && showDebugOnRender != 0) {
        //DEBUGRenderDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);
        //SDL_RenderPresent(renderer);
        return true;
    }
    
    return [x16Thread.delegate handleEvents];
}

void
video_end()
{
    if (debugger_enabled) {
        DEBUGFreeUI();
    }
    
    [x16Thread.renderer close];
}
