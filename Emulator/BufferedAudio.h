//
//  BufferedAudio.m
//  Emulator
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Audio.h"

#include "ringbuffer.h"

@interface BufferedAudio : Audio

- (instancetype)initSampleRate: (Float64)sampleRate channels: (UInt32)channels samplesPerBuffer: (UInt32)samplesPerBuffer numberOfBuffers: (UInt32)numberOfBuffers;

- (size_t)available;
- (void)write: (const uint8_t *)buffer length: (size_t)length;

- (OSStatus)fillBuffer: (uint8_t *)buffer numberOfFrames: (UInt32)numberOfFrames;

@property UInt32 sampleSize;
@property pthread_mutex_t ringbufferMutex;
@property ringbuffer_t *ringbuffer;

@end
