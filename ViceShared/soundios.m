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

#include <pthread.h>

#include "../../C64/Vice/Audio.h"
#include "ringbuffer.h"

#include "vice.h"
#include "sound.h"

static ringbuffer_t *ringbuffer;
static size_t sample_size;
static pthread_mutex_t ringbuffer_mutex;

#undef AUDIO_TRACE

static int ios_bufferspace(void) {
#ifdef AUDIO_TRACE
    printf("available %zu bytes\n", ringbuffer_bytes_free(ringbuffer));
#endif
    return (int)(ringbuffer_bytes_free(ringbuffer) / sample_size);
}


static void ios_close(void) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioStop(); // TODO: propagate error
        audioClose();
    });
    pthread_mutex_lock(&ringbuffer_mutex);
    ringbuffer_free(ringbuffer);
    ringbuffer = NULL;
    pthread_mutex_unlock(&ringbuffer_mutex);
    pthread_mutex_destroy(&ringbuffer_mutex);
}

static OSStatus audio_render_callback(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers) {
    uint8_t *inputFrames = (uint8_t *)(buffers->mBuffers->mData);
    size_t totalBytes = numFrames * sample_size;

    pthread_mutex_lock(&ringbuffer_mutex);
    void *p = ringbuffer_memcpy_from(inputFrames, ringbuffer, totalBytes);
    pthread_mutex_unlock(&ringbuffer_mutex);
    
    if (p == NULL) {
        memset(inputFrames, 0, totalBytes);
        //printf("buffer underrun\n");
    }
#ifdef AUDIO_TRACE
    printf("read %zu bytes\n", totalBytes);
#endif

    return noErr;
}

static int ios_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels) {
    sample_size = sizeof(int16_t) * *channels;
    UInt32 buffer_size = *fragnr * *fragsize;
    ringbuffer = ringbuffer_new(buffer_size * sample_size);
    
//    printf("INIT: sample rate: %d, buffer size: %zu bytes, duration: %f\n", *speed, *fragnr * *fragsize * *channels * sizeof(int16_t), (double)buffer_size / *speed / 2);

    pthread_mutex_init(&ringbuffer_mutex, NULL);
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioSetup(audio_render_callback, (Float64)*speed, *channels, buffer_size / 2); // TODO: propagate error
    });
    
    return 0;
}


static int ios_resume(void) {
    printf("ios_resume\n");
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioStart(); // TODO: propagate error
    });
    return 0;
}


static int ios_suspend(void) {
    printf("ios_suspend\n");
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioStop(); // TODO: propagate error
    });
    return 0;
}



static int ios_write(int16_t *pbuf, size_t nr)
{
    ringbuffer_memcpy_into(ringbuffer, pbuf, nr * sample_size);
    
#ifdef AUDIO_TRACE
    printf("wrote %zu bytes\n", nr * sample_size);
#endif

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
