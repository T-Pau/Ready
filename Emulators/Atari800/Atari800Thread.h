/*
 Atari800Thread.h -- Emulator Thread for Atari800 Emulator
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

#ifndef HAD_ATARI800_THREAD_H
#define HAD_ATARI800_THREAD_H

@import UIKit;
@import Emulator;

@interface Atari800Thread : EmulatorThread

@property NSArray *args;

@property int ramSize;
@property BOOL running;

@property BufferedAudio * _Nullable audio;

- (void)runEmulator;
- (void)updateJoystick: (int)port directions: (int)directions fire: (BOOL)fire;

@end

extern int joystickPort[2];
extern int joystickTrigger[4];

extern Atari800Thread * _Nullable atari800Thread;

void display_fini(void);
int display_init(void);

#endif /* HAD_ATARI800_THREAD_H */
