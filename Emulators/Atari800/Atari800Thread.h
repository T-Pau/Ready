//
//  Atari800Thread.h
//  Ready
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#ifndef HAD_ATARI800_THREAD_H
#define HAD_ATARI800_THREAD_H

@import UIKit;
@import Emulator;

@protocol Atari800ThreadDelegate
@required
//- (NSString *_Nonnull)getDirectoryPath;
//- (void)updateDriveUnit:(int) unit track: (double)track;
//- (void)updateDriveUnit:(int) uint led1Intensity: (double)intensity1 led2Intensity: (double)intensity2;
//- (void)updateTapeControlStatus: (int)control;
//- (void)updateTapeCounter: (double)counter;
//- (void)updateTapeIsMotorOn: (int)motor;
//- (void)setupVice;
//- (void)viceSetResources;
//- (void)autostartInjectDeviceInfo;
- (BOOL)handleEvents;
//- (void)updateStatusBar;
@end

@interface Atari800Thread : EmulatorThread

@property NSArray *args;

@property BOOL running;

@property int currentBorderMode;
@property int newBorderMode;


- (void)main;

@end

extern int joystickPort[2];

extern Atari800Thread * _Nullable atari800Thread;

void display_fini();
int display_init();

#endif /* HAD_ATARI800_THREAD_H */
