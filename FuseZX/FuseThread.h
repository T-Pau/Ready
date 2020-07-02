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
@end

@interface FuseThread : NSThread
@property UIImageView * _Nullable imageView;

@property NSMutableData * _Nullable imageData;
@property size_t bytesPerRow;

- (void)main;

- (void)updateBitmapWidth: (size_t)width height: (size_t)height;

@end

extern FuseThread * _Nullable fuseThread;

#endif /* HAD_FUSE_THREAD_H */
