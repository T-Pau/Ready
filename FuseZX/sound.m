/* sdlsound.c: SDL sound I/O
   Copyright (c) 2002-2015 Alexander Yurchenko, Russell Marks, Philip Kendall,
			   Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

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

#include "../C64/Vice/Audio.h"

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

    dispatch_sync(dispatch_get_main_queue(), ^{
        if (!audioSetup(audio_render_callback, (Float64)*freqptr, channels, sound_framesize)) {
            sfifo_flush(&sound_fifo);
            sfifo_close(&sound_fifo);
            // TODO: propagate error
        }
    });
    
#ifdef DEBUG_FUSE_AUDIO
    printf("fuse sound started: %d channels, %dkHz, frame size %d duration %gs\n", channels, *freqptr, sound_framesize, 1 / hz);
#endif

    return 0;
}

void sound_lowlevel_end(void) {
#ifdef DEBUG_FUSE_AUDIO
    printf("fuse sound ending\n");
#endif
    dispatch_sync(dispatch_get_main_queue(), ^{
        audioStop();
        audioClose();
        sfifo_flush(&sound_fifo);
        sfifo_close(&sound_fifo);
    });
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
