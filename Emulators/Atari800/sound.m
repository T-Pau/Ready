/*
 sound.m -- Platform Specific Sound Functions for Atari800 Emulator
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

#include "Atari800Thread.h"
#include "platform.h"

int PLATFORM_SoundSetup(Sound_setup_t *setup) {
    PLATFORM_SoundExit();

    if (setup->buffer_frames == 0) {
        /* Set buffer_frames automatically. */
        setup->buffer_frames = setup->freq / 50;
    }
    
    atari800Thread.audio = [[BufferedAudio alloc] initSampleRate:setup->freq channels:setup->channels samplesPerBuffer:setup->buffer_frames numberOfBuffers:2];

    if (atari800Thread.audio == nil) {
        return 0;
    }
    
    return 1;
}

void PLATFORM_SoundExit(void) {
    atari800Thread.audio = nil;
}

void PLATFORM_SoundPause(void) {
    [atari800Thread.audio stop];
}

void PLATFORM_SoundContinue(void) {
    [atari800Thread.audio start];
}

unsigned int PLATFORM_SoundAvailable(void) {
    return (unsigned int)[atari800Thread.audio available];
}

void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size) {
    [atari800Thread.audio write:buffer length:size];
}
