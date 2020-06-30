/* dxsound.c: DirectX sound I/O
   Copyright (c) 2003-2004 Marek Januszewski, Philip Kendall

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

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

/* same as for SDL Sound */
#define MAX_AUDIO_BUFFER 8192*5

static LPDIRECTSOUND lpDS; /* DirectSound object */
static LPDIRECTSOUNDBUFFER lpDSBuffer; /* sound buffer */

static DWORD nextpos; /* next position in circular buffer */

static int sixteenbit;

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{

  WAVEFORMATEX pcmwf; /* waveformat struct */ 
  DSBUFFERDESC dsbd; /* buffer description */

  /* Initialize COM */
  CoInitialize(NULL);

  /* create DirectSound object */
  if( CoCreateInstance( &CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
			&IID_IDirectSound, (void**)&lpDS ) != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't create DirectSound object.");
    CoUninitialize();
    return 1;
  }
  
  /* initialize it */    
  if( IDirectSound_Initialize( lpDS, NULL ) != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't initialize DirectSound." );
    CoUninitialize();
    return 1;
  }
  
  /* set normal cooperative level */
  if( IDirectSound_SetCooperativeLevel( lpDS, GetDesktopWindow(),
					DSSCL_NORMAL ) != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't set DirectSound cooperation level." );
    IDirectSound_Release( lpDS );
    CoUninitialize();
    return 1;
  }
  
  /* create wave format description */
  memset( &pcmwf, 0, sizeof( WAVEFORMATEX ) );

  pcmwf.cbSize = 0;
  if( settings_current.sound_force_8bit ) {
    pcmwf.wBitsPerSample = 8;
    sixteenbit = 0;
  } else {
    pcmwf.wBitsPerSample = 16;
    sixteenbit = 1;
  }
  pcmwf.nChannels = *stereoptr ? 2 : 1;
  pcmwf.nBlockAlign = pcmwf.nChannels * ( sixteenbit ? 2 : 1 );
  pcmwf.nSamplesPerSec = *freqptr;

  pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;

  pcmwf.wFormatTag = WAVE_FORMAT_PCM;

  /* create sound buffer description */
  memset( &dsbd, 0, sizeof( DSBUFFERDESC ) );
  dsbd.dwBufferBytes = MAX_AUDIO_BUFFER;

  dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | 
                 DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;

  dsbd.dwSize = sizeof( DSBUFFERDESC );
  dsbd.lpwfxFormat = &pcmwf;
  
  /* attempt to create the buffer */  
  if( IDirectSound_CreateSoundBuffer( lpDS, &dsbd, &lpDSBuffer, NULL )
      != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't create DirectSound buffer." );
    IDirectSound_Release( lpDS );
    CoUninitialize();
    return 1;
  }
  
  /* play buffer */
  if( IDirectSoundBuffer_Play( lpDSBuffer, 0, 0, DSBPLAY_LOOPING ) != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't play sound." );
    IDirectSoundBuffer_Release( lpDSBuffer );
    IDirectSound_Release( lpDS );
    CoUninitialize();
    return 1;
  }

  nextpos = 0;

  return 0;      
}

void
sound_lowlevel_end( void )
{
  if( IDirectSoundBuffer_Stop( lpDSBuffer ) != DS_OK ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "Couldn't stop sound." );
  }

  IDirectSoundBuffer_Release( lpDSBuffer );
  IDirectSound_Release( lpDS );
  CoUninitialize();
}

/* Copying data to the buffer */
void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  HRESULT hres;
  int i1, i2;
  int lsb = 1;

  /* two pair because of circular buffer */
  UCHAR *ucbuffer1, *ucbuffer2;
  DWORD length1, length2;
  DWORD playcursor;
  long cursordiff;

  if( sixteenbit )
    len *= 2;

  while( len ) {
    while( 1 ) {
      IDirectSoundBuffer_GetCurrentPosition( lpDSBuffer, &playcursor, NULL );

      cursordiff = (long)nextpos - (long)playcursor;

      if( playcursor > nextpos )
        cursordiff += MAX_AUDIO_BUFFER;

      if( cursordiff < len * 6 )
        break;

      Sleep(10);
    }

    /* lock the buffer */
    hres = IDirectSoundBuffer_Lock( lpDSBuffer, nextpos, (DWORD)len,
                                  (void **)&ucbuffer1, &length1,
                                  (void **)&ucbuffer2, &length2,
                                  0 );
    if( hres != DS_OK ) return; /* couldn't get a lock on the buffer */

    /* write to the first part of buffer */
    for( i1 = 0; i1 < length1 && len > 0; i1++ ) {
      if( sixteenbit ) {
        if( lsb ) {
          ucbuffer1[ i1 ] = *data & 0xff;
        } else {
          ucbuffer1[ i1 ] = *data >> 8;
          data++;
        }
        lsb = !lsb;
      } else {
        ucbuffer1[ i1 ] = ( ( *data++ ) >> 8 ) ^ 0x80;
      }
      nextpos++;
      len--;
    }

    if( ucbuffer2 ) {
      /* write to the second part of buffer */
      for( i2 = 0; i2 < length2 && len > 0; i2++ ) {
        if( sixteenbit ) {
          if( lsb ) {
            ucbuffer2[ i2 ] = *data & 0xff;
          } else {
            ucbuffer2[ i2 ] = *data >> 8;
            data++;
          }
          lsb = !lsb;
        } else {
          ucbuffer2[ i2 ] = ( ( *data++ ) >> 8 ) ^ 0x80;
        }
        nextpos++;
        len--;
      }
    } else {
      i2 = 0;
    }

    if( nextpos >= MAX_AUDIO_BUFFER ) nextpos -= MAX_AUDIO_BUFFER;

    /* unlock the buffer */
    IDirectSoundBuffer_Unlock( lpDSBuffer, ucbuffer1, i1, ucbuffer2, i2 );
  }
}
