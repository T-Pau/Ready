/*
 EmulatorThread.m -- Base Class for Thread Running Emulator
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

@import UIKit;

#include "EmulatorThread.h"

@implementation EmulatorThread

- (id)init {
    if ((self = [super init]) == nil) {
        return nil;
    }
    _delegate = NULL;
    _screenVisible = NULL;
    self.args = [[NSArray alloc] init];
    self.qualityOfService = NSQualityOfServiceUserInteractive;
    self.renderer = [[Renderer alloc] init];
    self.renderers = [[NSMutableArray alloc] init];
    [self.renderers addObject:self.renderer];
    return self;
}

- (int)borderMode {
    Renderer *renderer = _renderers[0];
    return renderer.borderMode;
}

- (void)setBorderMode:(int)borderMode {
    for (size_t i = 0; i < _renderers.count; i++) {
        Renderer *renderer = _renderers[i];
        renderer.borderMode = borderMode;
    }
}

- (int)displayedScreens {
    return _displayedScreens;
}

- (void)setDisplayedScreens:(int)displayedScreens {
    BOOL initial = _screenVisible == NULL;
    if (initial) {
        _screenVisible = (BOOL *)malloc(_renderers.count * sizeof(_screenVisible[0]));
    }
    else if (_displayedScreens == displayedScreens) {
        return;
    }
    
    _displayedScreens = displayedScreens;

    // TODO: for auto, consult last change
    BOOL visible = displayedScreens < 0 ? YES : NO;
    for (size_t i = 0; i < _renderers.count; i++) {
        _screenVisible[i] = visible;
    }
    if (displayedScreens >= 0) {
        _screenVisible[displayedScreens] = YES;
    }

    [_delegate updateDisplayedScreensAnimated:!initial];
}

- (id)delegate {
    return _delegate;
}

- (void)setDelegate:(id)delegate {
    _delegate = delegate;
    for (size_t i = 0; i < _renderers.count; i++) {
        Renderer *renderer = _renderers[i];
        renderer.delegate = delegate;
    }
}

- (void)displayImage {
    // TODO: implement auto hiding
    for (size_t i = 0; i < _renderers.count; i++) {
        Renderer *renderer = _renderers[i];
        [renderer displayImage];
    }
}

- (void)main {
    _argv = (char **)malloc(sizeof(_argv[0]) * (self.args.count) + 1);
    _argc = (int)self.args.count;
    
    for (int i = 0; i < self.args.count; i++) {
        _argv[i] = (char *)[self.args[i] cStringUsingEncoding:NSUTF8StringEncoding];
    }
    _argv[_argc] = NULL;
    
    [self runEmulator];
    
    printf("emulator thread exiting\n");
    fflush(stdout);
    [NSThread exit];
}

-(void)dealloc {
    printf("emulator thread freed\n");
}

@end
