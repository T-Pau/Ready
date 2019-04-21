/*
 Audio.c -- CoreAudio glue code
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

#include "Audio.h"

static AudioUnit *audioUnit = NULL;

static bool debuglog = true;
void printOSStatus(char *name, OSStatus status) {
    if(debuglog) {
        NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
        NSLog(@"%s:%@", name, error);
    }
}

BOOL audioSetup(audio_render_callback_t renderCallback, Float64 sampleRate, UInt32 channels, double preferredBufferDurationInSeconds) {
    printf("audioSetup\n");

    // init audio session
    BOOL success = YES;
    NSError *error;
    audioUnit = (AudioUnit*)malloc(sizeof(AudioUnit));
    success = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
    if(!success) {
        if(debuglog) {
            NSLog(@"AVAudioSessionCategoryPlayback %@", error);
        }
        return NO;
    }
    success = [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:preferredBufferDurationInSeconds error:&error];
    if(!success) {
        if(debuglog) {
            NSLog(@"setPreferredIOBufferDuration %@", error);
        }
        return NO;
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
    status = AudioComponentInstanceNew(component, audioUnit);
    if(status != noErr) {
        printOSStatus("AudioComponentInstanceNew", status);
        return NO;
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
    status = AudioUnitSetProperty(*audioUnit, kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, &streamDescription, sizeof(streamDescription));
    if(status != noErr) {
        printOSStatus("AudioUnitSetProperty kAudioUnitProperty_StreamFormat kAudioUnitScope_Input", status);
        return NO;
    }
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = renderCallback; // render function
    callbackStruct.inputProcRefCon = NULL;
    status = AudioUnitSetProperty(*audioUnit, kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input, 0, &callbackStruct,
                                  sizeof(AURenderCallbackStruct));
    if(status != noErr) {
        printOSStatus("AudioUnitSetProperty kAudioUnitProperty_SetRenderCallback", status);
        return NO;
    }
    
    audioStart();
    
    return YES;
}

BOOL audioStart(void) {
    BOOL ok = YES;
    NSError *error;

    printf("audioStart\n");

    OSStatus status = noErr;
    status = AudioUnitInitialize(*audioUnit);
    if(status != noErr) {
        printOSStatus("AudioUnitInitialize", status);
        ok = NO;
    }
    status = AudioOutputUnitStart(*audioUnit);
    if(status != noErr) {
        printOSStatus("AudioOutputUnitStart", status);
        ok = NO;
    }
    
    ok = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if(!ok) {
        if(debuglog) {
            NSLog(@"setActive %@", error);
        }
    }
    
    return ok;
}

BOOL audioStop(void) {
    BOOL ok = YES;
    NSError *error;

    printf("audioStop\n");
    
    OSStatus status = noErr;
    status = AudioOutputUnitStop(*audioUnit);
    if(status != noErr) {
        printOSStatus("AudioOutputUnitStop", status);
        ok = NO;
    }
    status = AudioUnitUninitialize(*audioUnit);
    if(status != noErr) {
        printOSStatus("AudioUnitUninitialize", status);
        ok = NO;
    }
    
    ok = [[AVAudioSession sharedInstance] setActive:NO error:&error];
    if(!ok) {
        if(debuglog) {
            NSLog(@"setActive %@", error);
        }
    }
    
    return ok;
}

void audioClose(void) {
    free(audioUnit);
    audioUnit = NULL;
}
