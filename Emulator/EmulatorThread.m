//
//  EmulatorThread.m
//  Emulator
//
//  Created by Dieter Baron on 03.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

@import UIKit;

#include "EmulatorThread.h"

@implementation EmulatorThread

- (id)init {
    if ((self = [super init]) == nil) {
        return nil;
    }
    self.qualityOfService = NSQualityOfServiceUserInteractive;
    self.renderer = [[Renderer alloc] init];
    return self;
}

- (int)borderMode {
    if (_renderer != nil) {
        return _renderer.borderMode;
    }
    else {
        return _initialBorderMode;
    }
}

- (void)setBorderMode:(int)borderMode {
    if (_renderer != nil) {
        _renderer.borderMode = borderMode;
    }
    else {
        _initialBorderMode = borderMode;
    }
}

- (id)delegate {
    return _delegate;
}
- (void)setDelegate:(id)delegate {
    _delegate = delegate;
    _renderer.delegate = delegate;
}

@end
