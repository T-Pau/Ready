/*
 Audio.h -- CoreAudio glue code
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

#ifndef Audio_h
#define Audio_h

#import <AVFoundation/AVFoundation.h>

typedef OSStatus(*audio_render_callback_t)(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers);

BOOL audioSetup(audio_render_callback_t renderCallback, Float64 sampleRate, UInt32 channels, double preferredBufferDurationInSeconds);

BOOL audioStart(void);
BOOL audioStop(void);
void audioClose(void);

#endif /* AduioSetup_h */
