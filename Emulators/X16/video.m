//
//  video.m
//  X16
//
//  Created by Dieter Baron on 14.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
