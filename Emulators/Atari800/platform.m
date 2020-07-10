//
//  platform.m
//  Atari800
//
//  Created by Dieter Baron on 10.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Atari800Thread.h"
#include "platform.h"
#include "akey.h"

int PLATFORM_Exit (int run_monitor) {
    atari800Thread.running = NO;
    return 0;
}

int PLATFORM_Initialise(int *argc, char *argv[]) {
    return TRUE;
}


int PLATFORM_Keyboard (void) {
    // TODO: implement
    return AKEY_NONE;
}

int PLATFORM_PORT(int num) {
    return joystickPort[num] & 0xf;
}

int PLATFORM_TRIG(int num) {
    return joystickPort[num] >> 4;
}

