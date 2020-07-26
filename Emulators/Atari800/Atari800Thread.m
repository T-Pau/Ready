/*
 Atari800Thread.m -- Emulator Thread for Atari800 Emulator
 Copyright (C) 2020 Dieter Baron
 
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

#import "Atari800Thread.h"

void atari800_main(int argc, char **argv);

@implementation Atari800Thread

- (void)runEmulator {
    joystickPort[0] = 0x1f;
    joystickPort[1] = 0x1f;

    display_init();
    atari800_main(_argc, _argv);
    display_fini();
    
    atari800Thread = nil;
}

- (void)updateJoystick: (int)port directions: (int)directions fire: (BOOL)fire {
    if (port % 2 == 0) {
        joystickPort[port/2] = (joystickPort[port/2] & 0xf0) | directions;
    }
    else {
        joystickPort[port/2] = (joystickPort[port/2] & 0x0f) | (directions << 4);
    }
    joystickTrigger[port] = fire ? 0 : 1;
}


@end

int joystickPort[2];
int joystickTrigger[4];

Atari800Thread *atari800Thread;


void atari800_main(int argc, char **argv) {
    /* initialise Atari800 core */
    int ret;

    INPUT_key_code = AKEY_NONE;
    INPUT_key_shift = 0;
    INPUT_key_consol = 0x7;
    Sound_enabled = 1;

    if (!(ret=Atari800_Initialise(&argc, argv))) {
        printf("init failed: %d\n", ret);
        return;
    }
    
    MEMORY_ram_size = atari800Thread.ramSize;

    printf("starting main loop\n");
    atari800Thread.running = YES;
    /* main loop */
    while (atari800Thread.running) {
        (void)[atari800Thread.delegate handleEvents];
        Atari800_Frame();
        if (Atari800_display_screen) {
            PLATFORM_DisplayScreen();
        }
    }
    
    Atari800_Exit(FALSE);
    printf("exiting\n");
}
