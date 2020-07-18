/*
 sdl_stubs.m -- SDL Replacement Functions for X16 Emulator
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
