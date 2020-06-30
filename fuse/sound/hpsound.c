/* hpsound.c: HP-UX sound I/O
   Copyright (c) 2002-2004 Alexander Yurchenko, Russell Marks, Philip Kendall
			   Matan Ziv-Av, Stuart Brady

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

#ifdef AUDIO_FORMAT_LINEAR16BIT

#include <sys/types.h>
#include <sys/audio.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

static int soundfd = -1;
static int sixteenbit = 1;

int sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  int flags, tmp, frag;

  /* select a default device if we weren't explicitly given one */
  if( device == NULL ) device = "/dev/audio";
  
  /* Open the sound device non-blocking to avoid hangs if it is being
   * used by something else, but then set it blocking again as that's what
   * we actually want */
  if( ( soundfd = open( device, O_WRONLY | O_NONBLOCK ) ) == -1 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't open sound device '%s'", device );
    return 1;
  }

  if( ( flags = fcntl( soundfd, F_GETFL ) ) == -1 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't fcntl sound device '%s'", device );
    close( soundfd );
    return 1;
  }

  flags &= ~O_NONBLOCK;

  if( fcntl( soundfd, F_SETFL, flags ) == -1 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't set sound device '%s' blocking",
	      device );
    close( soundfd );
    return 1;
  }

  tmp = AUDIO_FORMAT_LINEAR16BIT;
  if( settings_current.sound_force_8bit                ||
      ioctl( soundfd, AUDIO_SET_DATA_FORMAT, tmp ) < 0    ) {

    /* try 8-bit - may be an 8-bit only device */
    tmp = AUDIO_FORMAT_LINEAR8BIT;
    if( ioctl( soundfd, AUDIO_SET_DATA_FORMAT, tmp ) < 0 ) {
      settings_current.sound = 0;

      if( settings_current.sound_force_8bit ) {
	ui_error( UI_ERROR_ERROR,
		  "Couldn't set sound device '%s' into 8-bit mode", device );
      } else {
	ui_error(
	  UI_ERROR_ERROR,
	  "Couldn't set sound device '%s' in either 16-bit or 8-bit mode",
	  device
	);
      }

      close( soundfd );
      return 1;
    }

    sixteenbit = 0;
  }

  tmp = ( *stereoptr ) ? 2 : 1;
  if( ioctl( soundfd, AUDIO_SET_CHANNELS, tmp ) < 0 ) {
    tmp = ( *stereoptr ) ? 1 : 2;
    if( ioctl( soundfd, AUDIO_SET_CHANNELS, tmp ) < 0 ) {
      settings_current.sound = 0;
      ui_error(
        UI_ERROR_ERROR,
        "Couldn't set sound device '%s' into either mono or stereo mode",
	device
      );
      close( soundfd );
      return 1;
    }
    *stereoptr = tmp;
  }

  if( ioctl( soundfd, AUDIO_SET_SAMPLE_RATE, *freqptr ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR,"Couldn't set sound device '%s' speed to %d",
	      device, *freqptr ); 
    close( soundfd );
    return 1;
  }

  frag = 16384;
  ioctl( soundfd, AUDIO_SET_FRAGMENT, frag );

  return 0;
}

void
sound_lowlevel_end( void )
{
  if( soundfd != -1 ) close( soundfd );
}

void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  static unsigned char buf8[4096];
  unsigned char *data8=(unsigned char *)data;
  int ret=0, ofs=0;

  len <<= 1;	/* now in bytes */
  if( !sixteenbit ) {
    libspectrum_signed_word *src;
    unsigned char *dst;
    int f;

    src = data; dst = buf8;
    len >>= 1;
    /* TODO: confirm byteorder on IA64 */
    for( f = 0; f < len; f++ )
      *dst++ = 128 + (int)( (*src++) / 256 );

    data8 = buf8;
  }

  while( len ) {
    ret = write( soundfd, data8 + ofs, len );
    if( ret > 0 ) {
      ofs += ret;
      len -= ret;
    }
  }
}

#endif				/* #ifdef AUDIO_FORMAT_LINEAR16BIT */
