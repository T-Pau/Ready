/*
 ViceThread.h -- Vice Emulator Thread
 Copyright (C) 2019 Dieter Baron
 
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

#import "ViceThread.h"

#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "main.h"
#include "joyport.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "maincpu.h"
#include "attach.h"
#include "resources.h"

#import "ViceThread.h"

@implementation ViceThread

- (id)init {
    if ((self = [super init]) == nil) {
        return nil;
    }
    self.qualityOfService = NSQualityOfServiceUserInteractive;
    return self;
}

- (void)main {
    const char *argv[256];
    int argc = 0;

    for (NSString *arg in self.argv) {
        argv[argc++] = [arg cStringUsingEncoding:NSUTF8StringEncoding];
    }
    
    argv[argc] = NULL;
    
    bundle_directory = strdup([[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSUTF8StringEncoding]);
    _firstFrame = true;
    
    main_program(argc, (char **)argv);
    
    printf("vice exiting\n");
    
    archdep_vice_exit(0);
}

- (void)dealloc {
    free(bundle_directory);
}

- (BOOL)vsync {
    if (_firstFrame) {
        _firstFrame = false;
        [self.delegate autostartInjectDeviceInfo];
    }
    
    BOOL continueProcessing = [self.delegate handleEvents];
    
    if (continueProcessing) {
        [self.delegate updateStatusBar];
    }
    
    return continueProcessing;
}

- (void)pressKeyRow: (int)row column: (int) column {
    keyarr[row] |= 1 << column;
    rev_keyarr[column] |= 1 << row;
}

- (void)releaseKeyRow: (int)row column: (int) column {
    keyarr[row] &= ~(1 << column);
    rev_keyarr[column] &= ~(1 << row);
}

@end

ViceThread *viceThread;
