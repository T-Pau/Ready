//
//  Atari800Thread.h
//  Ready
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#ifndef HAD_X16_THREAD_H
#define HAD_X16_THREAD_H

@import UIKit;
@import Emulator;

@protocol X16ThreadDelegate
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

@interface X16Thread : EmulatorThread

@property NSArray *args;
@property NSString *dataDir;

@property BufferedAudio *_Nullable audio;

- (void)main;

@end

extern X16Thread * _Nullable x16Thread;

int x16_emulator_main(int argc, char **argv);

#endif /* HAD_X16_THREAD_H */
