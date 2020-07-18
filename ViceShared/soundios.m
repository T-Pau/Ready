/*
 soundios.m -- Sound Glue Code
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
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
