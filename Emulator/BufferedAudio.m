//
//  BufferedAudio.m
//  Emulator
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <pthread.h>

#import "BufferedAudio.h"

static OSStatus callback(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers) {
    BufferedAudio *this = (__bridge BufferedAudio *)(userData);
    
    return [this fillBuffer:(uint8_t *)buffers->mBuffers->mData numberOfFrames:numFrames];
}

@implementation BufferedAudio

- (instancetype)initSampleRate: (Float64)sampleRate channels: (UInt32)channels samplesPerBuffer: (UInt32)samplesPerBuffer numberOfBuffers: (UInt32)numberOfBuffers {
    if ((self = [super initSampleRate:sampleRate channels:channels samplesPerBuffer:samplesPerBuffer callback:callback userData: (__bridge void *)(self)]) == nil) {
        return nil;
    }
    
    _sampleSize = channels * 2;
    pthread_mutex_init(&_ringbufferMutex, NULL);
    _ringbuffer = ringbuffer_new(_sampleSize * samplesPerBuffer * numberOfBuffers);
    return self;
}

- (void)dealloc {
    [self stop];
    ringbuffer_free(_ringbuffer);
    pthread_mutex_destroy(&_ringbufferMutex);
}

- (size_t)available {
    pthread_mutex_lock(&_ringbufferMutex);
    size_t available = ringbuffer_bytes_free(_ringbuffer);
    pthread_mutex_unlock(&_ringbufferMutex);
    
    return available;
}

- (void)write:(const void *)buffer length:(size_t)length {
    pthread_mutex_lock(&_ringbufferMutex);
    ringbuffer_memcpy_into(_ringbuffer, buffer, length);
    pthread_mutex_unlock(&_ringbufferMutex);
}

- (OSStatus)fillBuffer: (uint8_t *)buffer numberOfFrames: (UInt32)numberOfFrames {
    size_t totalBytes = numberOfFrames * _sampleSize;
    
    pthread_mutex_lock(&_ringbufferMutex);
    void *p = ringbuffer_memcpy_from(buffer, _ringbuffer, totalBytes);
    pthread_mutex_unlock(&_ringbufferMutex);
    
    if (p == NULL) {
        printf("underflow\n");
        memset(buffer, 0, totalBytes);
    }
    
    return noErr;
}

@end
