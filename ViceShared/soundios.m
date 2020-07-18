/*
 soundios.m -- Sound Glue Code
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

#import <AVFoundation/AVFoundation.h>


#include "vice.h"
#include "sound.h"

#include "ViceThread.h"

#undef AUDIO_TRACE

static int ios_bufferspace(void) {
    int samples_free = (int)([viceThread.audio available] / viceThread.audio.sampleSize);
#ifdef AUDIO_TRACE
    printf("available %d samples\n", samples_free);
#endif
    return samples_free;
}


static void ios_close(void) {
    viceThread.audio = nil;
}


static int ios_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels) {
    viceThread.audio = [[BufferedAudio alloc] initSampleRate:*speed channels:*channels samplesPerBuffer:(*fragsize * *fragnr)/2 numberOfBuffers:2];

    if (viceThread.audio == nil) {
        return -1;
    }
    
    [viceThread.audio start];
    return 0;
}


static int ios_resume(void) {
    [viceThread.audio start];
    return 0;
}


static int ios_suspend(void) {
    [viceThread.audio stop];
    return 0;
}



static int ios_write(int16_t *pbuf, size_t nr) {
    [viceThread.audio write:pbuf length:nr * viceThread.audio.sampleSize];
    return 0;
}

static sound_device_t ios_device =
{
    "ios",
    ios_init,
    ios_write,
    NULL,
    NULL,
    ios_bufferspace,
    ios_close,
    ios_suspend,
    ios_resume,
    0,
    2
};

int sound_init_ios_device(void)
{
    return sound_register_device(&ios_device);
}
