/* wiisound.c: Wii sound routines
   Copyright (c) 2008-2009 Bjoern Giesler, Philip Kendall

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

#include <string.h>
#include <unistd.h>

#include "fuse.h"
#include "sfifo.h"

#include <gccore.h>
#include <ogc/audio.h>

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

int samplerate;
int streamstate;
sfifo_t sound_fifo;

#define BUFSIZE 16384
u8 dmabuf[BUFSIZE<<1] ATTRIBUTE_ALIGN(32);
int dmalen = BUFSIZE;

static void
sound_dmacallback( void )
{
  if( sfifo_used( &sound_fifo ) < 128) return;
  
  dmalen = MIN( BUFSIZE, sfifo_used( &sound_fifo ) );
  sfifo_read( &sound_fifo, dmabuf, dmalen );
  DCFlushRange( dmabuf, dmalen );
  AUDIO_InitDMA( (u32)dmabuf, dmalen );
  AUDIO_StartDMA();
}

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  switch(*freqptr) {
  case 32000:
    samplerate = AI_SAMPLERATE_32KHZ;
    break;
  case 48000:
    samplerate = AI_SAMPLERATE_48KHZ;
    break;
  default:
    printf("Sample rate %d not supported on Wii\n", *freqptr);
    return 1;
  }

  sfifo_init( &sound_fifo, BUFSIZE );
  *stereoptr = 1;
  
  AUDIO_Init( NULL );
  AUDIO_SetDSPSampleRate( samplerate );

#ifndef DISPLAY_AUDIO
  AUDIO_RegisterDMACallback( sound_dmacallback );
  memset( dmabuf, 0, BUFSIZE );
  AUDIO_InitDMA( (u32)dmabuf, BUFSIZE );
  DCFlushRange( dmabuf, dmalen );
  AUDIO_StartDMA();
#endif
  
  return 0;
}

void
sound_lowlevel_end( void )
{
  sfifo_flush( &sound_fifo );
  sfifo_close( &sound_fifo );
  AUDIO_StopDMA();
}

void
sound_lowlevel_frame(libspectrum_signed_word *data, int len)
{
  int i;
  
  libspectrum_signed_byte *bytes = (libspectrum_signed_byte*)data;
  len <<= 1;

  while(len) {
    if( ( i = sfifo_write( &sound_fifo, bytes, len ) ) < 0 ) 
      break;
    else if( !i )
      usleep(10000);
    bytes += i;
    len -= i;
  }
}
