/*
 ViceThread.h -- Vice Emulator Thread
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

#ifndef ViceThread_h
#define ViceThread_h

#import <UIKit/UIKit.h>
@import GameController;
@import Emulator;

#if VICE_C64
#define ViceThread ViceThreadC64
#elif VICE_C128
#define ViceThread ViceThreadC128
#elif VICE_PLUS4
#define ViceThread ViceThreadPlus4
#elif VICE_VIC20
#define ViceThread ViceThreadVIC20
#endif

@protocol ViceThreadDelegate<EmulatorThreadDelegate>
@required
- (NSString *_Nonnull)getDirectoryPath;
- (void)updateDriveUnit:(int) unit track: (double)track;
- (void)updateDriveUnit:(int) uint led1Intensity: (double)intensity1 led2Intensity: (double)intensity2;
- (void)updateTapeControlStatus: (int)control;
- (void)updateTapeCounter: (double)counter;
- (void)updateTapeIsMotorOn: (int)motor;
- (void)setupVice;
- (void)viceSetResources;
- (void)autostartInjectDeviceInfo;
- (BOOL)handleEvents;
- (void)updateStatusBar;
@end

@interface ViceThread : EmulatorThread

// these are read by vice thread, written by main thread
@property int mouseX;
@property int mouseY;
@property unsigned long mouseTimestamp;


@property BufferedAudio * _Nullable audio;

@property bool firstFrame;

- (void)runEmulator;

- (BOOL)vsync;

// called on the main thread
- (void)pressKeyRow: (int)row column: (int) column NS_SWIFT_NAME(pressKey(row:column:));
- (void)releaseKeyRow: (int)row column: (int) column NS_SWIFT_NAME(releaseKey(row:column:));

@end

extern ViceThread * _Nullable viceThread;

#endif /* ViceThread_h */
