/*
 Audio.h -- CoreAudio glue code
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

#ifndef HAD_AUDIO_H
#define HAD_AUDIO_H

#import <AVFoundation/AVFoundation.h>

typedef OSStatus(*audio_render_callback_t)(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers);

@interface Audio : NSObject

- (instancetype)initSampleRate: (Float64)sampleRate channels: (UInt32)channels samplesPerBuffer: (UInt32)samplesPerBuffer callback: (audio_render_callback_t)renderCallback userData: (void *)userData;

- (BOOL)start;
- (BOOL)stop;

@property AudioUnit *audioUnit;
@property BOOL playing;

@end

#endif /* HAD_AUDIO_H */
