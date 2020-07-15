//
//  sdl_stubs.c
//  X16
//
//  Created by Dieter Baron on 14.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#include <time.h>

#include "SDL_error_c.h"
#include "SDL.h"

#import "X16Thread.h"

static struct timespec ticks_epoch;

int SDL_Init(Uint32 flags) {
    clock_gettime(CLOCK_MONOTONIC, &ticks_epoch);
    return 0;
}

void SDL_Quit(void) {
}

Uint32 SDL_GetTicks(void) {
    struct timespec ticks_now;
    
    clock_gettime(_CLOCK_MONOTONIC, &ticks_now);
    
    Uint32 ticks = (Uint32)((ticks_now.tv_sec - ticks_epoch.tv_sec) * 1000 + (ticks_now.tv_nsec - ticks_epoch.tv_nsec) / 1000000);
    if (ticks_epoch.tv_nsec > ticks_now.tv_nsec) {
//        ticks += 1000;
    }
    return ticks;
}

char *SDL_GetBasePath(void) {
    const char *dataDir = [x16Thread.dataDir cStringUsingEncoding:NSUTF8StringEncoding];
    size_t length = strlen(dataDir) + 2;
    char *path = (char *)malloc(length);
    snprintf(path, length, "%s/", dataDir);
    return path;\
}


SDL_error *
SDL_GetErrBuf(void)
{
    /* Non-thread-safe global error variable */
    static SDL_error SDL_global_error;
    return &SDL_global_error;
}
