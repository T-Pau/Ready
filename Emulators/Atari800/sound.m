//
//  sound.m
//  Atari800
//
//  Created by Dieter Baron on 10.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <pthread.h>
#include "Audio.h"
#include "ringbuffer.h"

#include "platform.h"

typedef enum {
    AUDIO_CLOSED,
    AUDIO_PAUSED,
    AUDIO_RUNNING
} audio_state_t;

static audio_state_t audio_state = AUDIO_CLOSED;

static unsigned int sample_size;
static ringbuffer_t *ringbuffer;
static pthread_mutex_t ringbuffer_mutex;

static OSStatus callback(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers) {
    uint8_t *inputFrames = (uint8_t *)(buffers->mBuffers->mData);
    size_t totalBytes = numFrames * sample_size;
    
    pthread_mutex_lock(&ringbuffer_mutex);
    void *p = ringbuffer_memcpy_from(inputFrames, ringbuffer, totalBytes);
    pthread_mutex_unlock(&ringbuffer_mutex);
    
    if (p == NULL) {
        memset(inputFrames, 0, totalBytes);
    }
    
    return noErr;
}

int PLATFORM_SoundSetup(Sound_setup_t *setup) {
    PLATFORM_SoundExit();

    if (setup->buffer_frames == 0) {
        /* Set buffer_frames automatically. */
        setup->buffer_frames = setup->freq / 50;
    }
    
    pthread_mutex_init(&ringbuffer_mutex, NULL);
    sample_size = setup->sample_size * setup->channels;

    ringbuffer = ringbuffer_new(setup->buffer_frames * 2 * sample_size);

    __block BOOL ok;
    dispatch_sync(dispatch_get_main_queue(), ^{
        ok = audioSetup(callback, setup->freq, setup->channels, setup->buffer_frames);
    });
    if (!ok) {
        return 0;
    }
    
    audio_state = AUDIO_PAUSED;
    return 1;
}

void PLATFORM_SoundExit(void) {
    if (audio_state == AUDIO_CLOSED) {
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioStop();
        audioClose();
    });
    pthread_mutex_lock(&ringbuffer_mutex);
    ringbuffer_free(ringbuffer);
    ringbuffer = NULL;
    pthread_mutex_unlock(&ringbuffer_mutex);
    pthread_mutex_destroy(&ringbuffer_mutex);
    audio_state = AUDIO_CLOSED;
}

void PLATFORM_SoundPause(void) {
    if (audio_state == AUDIO_RUNNING) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            audioStop();
        });
        audio_state = AUDIO_PAUSED;
    }
}

void PLATFORM_SoundContinue(void) {
    if (audio_state == AUDIO_PAUSED) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            audioStart();
        });
        audio_state = AUDIO_RUNNING;
    }
}

unsigned int PLATFORM_SoundAvailable(void) {
    pthread_mutex_lock(&ringbuffer_mutex);
    unsigned int byte_free = (unsigned int)ringbuffer_bytes_free(ringbuffer);
    pthread_mutex_unlock(&ringbuffer_mutex);
    return byte_free;
}

void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size) {
    pthread_mutex_lock(&ringbuffer_mutex);
    ringbuffer_memcpy_into(ringbuffer, buffer, size);
    pthread_mutex_unlock(&ringbuffer_mutex);
}
