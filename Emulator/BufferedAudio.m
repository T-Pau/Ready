/*
 BufferedAudio.m -- Audio Interface with Internal Buffer.
 Copyright (C) 2020 Dieter Baron

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
