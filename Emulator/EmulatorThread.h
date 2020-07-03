//
//  EmulatorThread.h
//  Ready
//
//  Created by Dieter Baron on 03.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#ifndef HAD_EMULATOR_THREAD_H
#define HAD_EMULATOR_THREAD_H

@import UIKit;

@protocol EmulatorThreadDelegate
@required
- (void)updateImage: (UIImage *_Nullable)image;
@end

@interface EmulatorThread : NSThread

@property NSMutableData * _Nullable imageData;
@property size_t bytesPerRow;

@property id _Nullable delegate;

- (void)initBitmapWidth: (size_t)width height: (size_t)height;
- (void)updateBitmapWidth: (size_t)width height: (size_t)height;

@end

#endif /* HAD_EMULATOR_THREAD_H */
