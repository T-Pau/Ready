/*
 Audio.m -- CoreAudio glue code
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

#include "Audio.h"

static bool debuglog = true;

void printOSStatus(char *name, OSStatus status) {
    if(debuglog) {
        NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
        NSLog(@"%s:%@", name, error);
    }
}

@implementation Audio

- (instancetype)initSampleRate: (Float64)sampleRate channels: (UInt32)channels samplesPerBuffer: (UInt32)samplesPerBuffer callback: (audio_render_callback_t)renderCallback userData: (void *)userData {
    // init audio session
    __block BOOL ok = YES;
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        BOOL success;
        NSError *error;
        _audioUnit = (AudioUnit*)malloc(sizeof(AudioUnit));
        success = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
        if(!success) {
            if(debuglog) {
                NSLog(@"AVAudioSessionCategoryPlayback %@", error);
            }
            ok = NO;
            return;
        }
        success = [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:samplesPerBuffer / sampleRate error:&error];
        if(!success) {
            if(debuglog) {
                NSLog(@"setPreferredIOBufferDuration %@", error);
            }
            ok = NO;
            return;
        }
        // init audio streams
        OSStatus status = noErr;
        AudioComponentDescription componentDescription;
        componentDescription.componentType = kAudioUnitType_Output;
        componentDescription.componentSubType = kAudioUnitSubType_RemoteIO;
        componentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
        componentDescription.componentFlags = 0;
        componentDescription.componentFlagsMask = 0;
        AudioComponent component = AudioComponentFindNext(NULL, &componentDescription);
        status = AudioComponentInstanceNew(component, _audioUnit);
        if(status != noErr) {
            printOSStatus("AudioComponentInstanceNew", status);
            ok = NO;
            return;
        }
        // the stream will be set up as a 16bit signed integer interleaved stereo PCM.
        AudioStreamBasicDescription streamDescription;
        streamDescription.mSampleRate = sampleRate;
        streamDescription.mFormatID = kAudioFormatLinearPCM;
        streamDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
        streamDescription.mChannelsPerFrame = channels;
        streamDescription.mBytesPerPacket = sizeof(SInt16) * channels;
        streamDescription.mFramesPerPacket = 1;
        streamDescription.mBytesPerFrame = sizeof(SInt16) * channels;
        streamDescription.mBitsPerChannel = sizeof(SInt16) * 8;
        streamDescription.mReserved = 0;
        
        // input stream
        // (it's a bit confusing, but apparently it's called input because the samples we write in the callback are considered an input)
        status = AudioUnitSetProperty(*_audioUnit, kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input, 0, &streamDescription, sizeof(streamDescription));
        if(status != noErr) {
            printOSStatus("AudioUnitSetProperty kAudioUnitProperty_StreamFormat kAudioUnitScope_Input", status);
            ok = NO;
            return;
        }
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = renderCallback; // render function
        callbackStruct.inputProcRefCon = userData;
        status = AudioUnitSetProperty(*_audioUnit, kAudioUnitProperty_SetRenderCallback,
                                      kAudioUnitScope_Input, 0, &callbackStruct,
                                      sizeof(AURenderCallbackStruct));
        if(status != noErr) {
            printOSStatus("AudioUnitSetProperty kAudioUnitProperty_SetRenderCallback", status);
            ok = NO;
            return;
        }
    });
       
    if (!ok) {
        return nil;
    }
    
    _playing = NO;
    
    printf("audioOpen\n");
    
    return self;
}

- (BOOL)start {
    if (_playing) {
        return YES;
    }
    
    __block BOOL ok = YES;
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSError *error;
    
        printf("audioStart\n");
    
        OSStatus status = noErr;
        status = AudioUnitInitialize(*_audioUnit);
        if(status != noErr) {
            printOSStatus("AudioUnitInitialize", status);
            ok = NO;
        }
        status = AudioOutputUnitStart(*_audioUnit);
        if(status != noErr) {
            printOSStatus("AudioOutputUnitStart", status);
            ok = NO;
        }
        
        if (ok) {
            ok = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        }
        if(!ok) {
            if(debuglog) {
                NSLog(@"setActive %@", error);
            }
        }
    });
    
    if (ok) {
        _playing = YES;
    }
    return ok;
}

- (BOOL)stop {
    if (!_playing) {
        return YES;
    }
    
    __block BOOL ok = YES;
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSError *error;
        
        printf("audioStop\n");
        
        OSStatus status = noErr;
        status = AudioOutputUnitStop(*_audioUnit);
        if(status != noErr) {
            printOSStatus("AudioOutputUnitStop", status);
            ok = NO;
        }
        status = AudioUnitUninitialize(*_audioUnit);
        if(status != noErr) {
            printOSStatus("AudioUnitUninitialize", status);
            ok = NO;
        }
        
        if (ok) {
            ok = [[AVAudioSession sharedInstance] setActive:NO error:&error];
        }
        if(!ok) {
            if(debuglog) {
                NSLog(@"setActive %@", error);
            }
        }
    });
    
    if (ok) {
        _playing = NO;
    }
    
    return ok;
}

- (void)dealloc {
    [self stop];
    printf("audioClose\n");
    free(_audioUnit);
}

@end
