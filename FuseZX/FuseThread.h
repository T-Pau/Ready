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

@interface FuseThread : NSThread
@property UIImageView * _Nullable imageView;

@property NSMutableData * _Nullable imageData;
@property size_t bytesPerRow;

@property (weak) id _Nullable delegate;

- (void)main;

- (void)updateBitmapWidth: (size_t)width height: (size_t)height;

@end

extern FuseThread * _Nullable fuseThread;

#endif /* HAD_FUSE_THREAD_H */
