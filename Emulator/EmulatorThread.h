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

#include "Audio.h"
#include "Renderer.h"

@protocol EmulatorThreadDelegate
@required
- (void)updateImage: (UIImage *_Nullable)image;
@end

@interface EmulatorThread : NSThread {
    id _delegate;
}

@property Renderer * _Nonnull renderer;

@property id _Nullable delegate;

@property int borderMode;
@property int initialBorderMode;

//- (int)borderMode;
//- (void)setBorderMode: (int)borderMode;

@end

#endif /* HAD_EMULATOR_THREAD_H */
