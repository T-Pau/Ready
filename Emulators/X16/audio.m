//
//  audio.m
//  X16
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import "X16Thread.h"

#include "platform.h"

size_t platform_audio_available(void) {
    return [x16Thread.audio available];
}
void platform_audio_close(void) {
    x16Thread.audio = nil;
}

int platform_audio_init(int samplerate, int samples_per_buffer, int channels) {
    x16Thread.audio = [[BufferedAudio alloc] initSampleRate:samplerate channels:channels samplesPerBuffer:samples_per_buffer numberOfBuffers:3];
    return x16Thread.audio != nil;
}

void platform_audio_write(void *buffer, size_t length) {
    [x16Thread.audio write:buffer length:length];
}
