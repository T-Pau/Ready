/*
 sound.m -- Platform Sepcific Audio Functions for Fuse Emulatro
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

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>


#include "settings.h"
#include "sound/sfifo.h"
#include "sound.h"
#include "ui/ui.h"

#include "FuseThread.h"

#undef DEBUG_FUSE_AUDIO

sfifo_t sound_fifo;

static int sample_size;

/* Number of Spectrum frames audio latency to use */
#define NUM_FRAMES 3


#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

static OSStatus audio_render_callback(void *userData, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *audioTimeStamp, UInt32 busNumber, UInt32 numFrames, AudioBufferList *buffers) {
    uint8_t *inputFrames = (uint8_t *)(buffers->mBuffers->mData);
    int length = numFrames * sample_size;

    /* Only read complete samples */
    int fifo_length = MIN(length, sfifo_used(&sound_fifo));
    fifo_length -= fifo_length % sample_size;
    
#ifdef DEBUG_FUSE_AUDIO
    printf("fuse sound getting %d bytes", length);
    if (fifo_length < length) {
        printf(", fifo short by %d", (length - fifo_length));
    }
    printf("\n");
#endif

    int n = 0;
    int i;
    while (n < fifo_length && (i = sfifo_read(&sound_fifo, inputFrames + n, fifo_length - n)) > 0) {
        n += i;
    }

    if (n < length) {
        memset(inputFrames + n, 0, length - n);
    }
    
    return noErr;
}
    
int sound_lowlevel_init(const char *device, int *freqptr, int *stereoptr) {
    int channels = *stereoptr ? 2 : 1;

    /* Adjust relative processor speed to deal with adjusting sound generation
     frequency against emulation speed (more flexible than adjusting generated
     sample rate) */
    Float64 hz = (Float64)sound_get_effective_processor_speed() / machine_current->timings.tstates_per_frame;
    
    /* Size of audio data we will get from running a single Spectrum frame */
    int sound_framesize = ( float )*freqptr / hz;

    sample_size = sizeof(int16_t) * channels;
    
    if (sfifo_init(&sound_fifo, NUM_FRAMES * channels * sound_framesize + 1 )) {
        printf("can't init sfifo\n");
        return 1;
    }

    fuseThread.audio = [[Audio alloc] initSampleRate:*freqptr channels:channels samplesPerBuffer:sound_framesize callback:audio_render_callback userData:NULL];

    if (fuseThread.audio == nil) {
        sfifo_flush(&sound_fifo);
        sfifo_close(&sound_fifo);
        return -1;
    }
    
    [fuseThread.audio start];

    return 0;
}

void sound_lowlevel_end(void) {
#ifdef DEBUG_FUSE_AUDIO
    printf("fuse sound ending\n");
#endif
    fuseThread.audio = nil;
    sfifo_flush(&sound_fifo);
    sfifo_close(&sound_fifo);
}

/* Copy data to fifo */
void sound_lowlevel_frame(libspectrum_signed_word *data, int len) {
    int i = 0;

#ifdef DEBUG_FUSE_AUDIO
    bool silence = true;
    for (int x = 0; x < len; x++) {
        if (data[x] != 0) {
            silence = false;
            break;
        }
    }
    printf("fuse sound writing %d bytes%s\n", 2 * len, silence ? ", all silence" : "");
#endif

    /* Convert to bytes */
    libspectrum_signed_byte* bytes = (libspectrum_signed_byte*)data;
    len *= 2;

    while (len) {
        if ((i = sfifo_write(&sound_fifo, bytes, len)) < 0) {
            break;
        }
        else if (i == 0) {
            usleep(10000);
        }
        bytes += i;
        len -= i;
    }
    if (i < 0) {
        printf("can't write to sfifo\n");
    }
}
