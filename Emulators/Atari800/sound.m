//
//  sound.m
//  Atari800
//
//  Created by Dieter Baron on 10.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
