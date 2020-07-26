/*
 EmulatorThread.h -- Base Class for Thread Running Emulator
 Copyright (C) 2019-2020 Dieter Baron
 
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

#ifndef HAD_EMULATOR_THREAD_H
#define HAD_EMULATOR_THREAD_H

@import UIKit;

#include "Audio.h"
#include "Renderer.h"

#define DISPLAY_SCREENS_ALL (-1)
#define DISPLAY_SCREENS_AUTO (-2)

@protocol EmulatorThreadDelegate
@required
- (void)updateDisplayedScreensAnimated:(BOOL)animated;
@end

@interface EmulatorThread : NSThread {
    id _delegate;
    int _displayedScreens;
    int _argc;
    char ** _argv;
}

@property NSArray * _Nonnull args;
@property Renderer * _Nonnull renderer;
@property NSMutableArray * _Nonnull renderers;

@property (weak) id _Nullable delegate;

@property int borderMode;
@property int displayedScreens;
@property BOOL * _Nonnull screenVisible;
@property int * _Nonnull screenActivity;

- (void)displayImage;

- (void)main;

// Override this in emulator implementation.
- (void)runEmulator;

//- (int)borderMode;
//- (void)setBorderMode: (int)borderMode;

@end

#endif /* HAD_EMULATOR_THREAD_H */
