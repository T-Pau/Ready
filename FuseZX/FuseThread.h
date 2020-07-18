//
//  FuseThread.h
//  Ready
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#ifndef HAD_FUSE_THREAD_H
#define HAD_FUSE_THREAD_H

@import UIKit;
@import Emulator;

@protocol FuseThreadDelegate
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

@interface FuseThread : EmulatorThread

@property NSArray *args;

@property Audio * _Nullable audio;

- (void)main;

@end

extern FuseThread * _Nullable fuseThread;

#endif /* HAD_FUSE_THREAD_H */
