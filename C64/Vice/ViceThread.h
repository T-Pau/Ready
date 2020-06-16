/*
 ViceThread.h -- Vice Emulator Thread
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

#ifndef ViceThread_h
#define ViceThread_h

#import <UIKit/UIKit.h>
@import GameController;
#import "Emulator/Emulator.h"
#import <Emulator/Emulator-Swift.h>

@protocol ViceThreadDelegate
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

@interface ViceThread : NSThread

@property UIImageView * _Nullable imageView;

@property NSMutableData * _Nullable imageData;
@property size_t bytesPerRow;

// these are read by vice thread, written by main thread
@property int mouseX;
@property int mouseY;
@property unsigned long mouseTimestamp;


@property NSArray * _Nullable argv;

@property NSMutableArray * _Nonnull eventQueue;

@property int currentBorderMode;
@property int newBorderMode;
@property bool firstFrame;

@property (weak) id _Nullable delegate;

- (void)main;

- (void)updateBitmapWidth: (unsigned int)width height: (unsigned int)height;
- (BOOL)vsync;

// called on the main thread
- (void)pressKeyRow: (int)row column: (int) column NS_SWIFT_NAME(pressKey(row:column:));
- (void)releaseKeyRow: (int)row column: (int) column NS_SWIFT_NAME(releaseKey(row:column:));

@end

extern ViceThread * _Nullable viceThread;

#endif /* ViceThread_h */
