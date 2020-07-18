/*
 ViceThread.h -- Vice Emulator Thread
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
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
