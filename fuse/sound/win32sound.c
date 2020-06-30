/* win32sound.c: Win32 sound using MS Windows Multimedia API
   Copyright (c) 2007 Marek Januszewski
   Copyright (c) 2015 Sergio Baldoví

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

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

static HWAVEOUT hwaveout;
static WAVEHDR wavehdr[2];
static HANDLE sem_sound_done;
static CRITICAL_SECTION sound_lock;

#define BUFFER_SIZE 4096

static void *buffers[2];
static int buffer_used[2];
static int current_buffer;
static char buffer1[ BUFFER_SIZE ];
static char buffer2[ BUFFER_SIZE ];

static int sixteenbit;

static void
sound_display_mmresult( const char * const func, MMRESULT result );

static void CALLBACK
sound_callback( HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  WAVEFORMATEX pcmwf; /* waveformat struct */
  MMRESULT result;

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

  /* Open sound device for output */
  result = waveOutOpen( &hwaveout, WAVE_MAPPER,
                        &pcmwf, ( DWORD_PTR ) sound_callback, 0,
                        CALLBACK_FUNCTION );
  if( result != MMSYSERR_NOERROR )
    sound_display_mmresult( "waveOutOpen", result );

  buffers[0] = buffer1;
  buffers[1] = buffer2;
  buffer_used[0] = 0;
  buffer_used[1] = 0;
  current_buffer = 0;
  memset( &wavehdr[0], 0, sizeof( WAVEHDR ) );
  memset( &wavehdr[1], 0, sizeof( WAVEHDR ) );

  sem_sound_done = CreateSemaphore( NULL, 0, 2, NULL );
  InitializeCriticalSection( &sound_lock );

  return 0;
}

void
sound_lowlevel_end( void )
{
  MMRESULT result;

  /* reset the sound */
  result = waveOutReset( hwaveout );
  if( result != MMSYSERR_NOERROR ) {
    settings_current.sound = 0;
    sound_display_mmresult( "Couldn't stop sound (waveOutReset)", result );
    return;
  }

  /* unprepare wave headers */
  if( wavehdr[ 0 ].dwFlags & WHDR_PREPARED ) {
    result = waveOutUnprepareHeader( hwaveout, &wavehdr[ 0 ], sizeof( WAVEHDR ) );
    if( result != MMSYSERR_NOERROR )
      sound_display_mmresult( "waveOutUnprepareHeader", result );
  }
  
  if( wavehdr[ 1 ].dwFlags & WHDR_PREPARED ) {
    result = waveOutUnprepareHeader( hwaveout, &wavehdr[ 1 ], sizeof( WAVEHDR ) );
    if( result != MMSYSERR_NOERROR )
      sound_display_mmresult( "waveOutUnprepareHeader", result );
  }

  /* close the device */
  result = waveOutClose( hwaveout );
  if( result != MMSYSERR_NOERROR ) {
    settings_current.sound = 0;
    sound_display_mmresult( "Couldn't stop sound (waveOutClose)", result );
  }

  CloseHandle( sem_sound_done );
  DeleteCriticalSection( &sound_lock );
}

void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  static unsigned char buf8[4096];
  MMRESULT result;

  /* Convert to bytes */
  unsigned char *bytes = (unsigned char *) data;
  len <<= 1;
  if( !sixteenbit ) {
    libspectrum_signed_word *src;
    unsigned char *dst;
    int f;

    src = data; dst = buf8;
    len >>= 1;
    /* TODO: confirm byteorder on IA64 */
    for( f = 0; f < len; f++ )
      *dst++ = 128 + (int)( (*src++) / 256 );

    bytes = buf8;
  }
  
  if( len > BUFFER_SIZE ) {
    ui_error( UI_ERROR_WARNING, "%s: requested wave size exceeds the buffer size", __func__ );
    return;
  }

  /* wait for the buffer to finish playing */
  if( buffer_used[ current_buffer ] > 0 )
    WaitForSingleObject( sem_sound_done, INFINITE );
  
  /* unprepare the header if it's prepared */
  if( wavehdr[ current_buffer ].dwFlags & WHDR_PREPARED ) {
    result = waveOutUnprepareHeader( hwaveout, &wavehdr[ current_buffer ], sizeof( WAVEHDR ) );
    if( result != MMSYSERR_NOERROR )
      sound_display_mmresult( "waveOutUnprepareHeader", result );
  }
  
  /* copy the new wave into the buffer */
  memcpy( buffers[ current_buffer ], bytes, len );
  buffer_used[ current_buffer ] = len;
  
  /* prepare the header */
  wavehdr[ current_buffer ].lpData = ( LPSTR ) bytes;
  wavehdr[ current_buffer ].dwBufferLength = len;
  wavehdr[ current_buffer ].dwLoops |= WHDR_BEGINLOOP | WHDR_ENDLOOP;

  result = waveOutPrepareHeader( hwaveout, &wavehdr[ current_buffer ], sizeof( WAVEHDR ) );
  if( result != MMSYSERR_NOERROR )
    sound_display_mmresult( "waveOutPrepareHeader", result );

  /* play */
  result = waveOutWrite( hwaveout, &wavehdr[ current_buffer ], sizeof( WAVEHDR ) );
  if( result != MMSYSERR_NOERROR )
    sound_display_mmresult( "waveOutWrite", result );
  
  /* FIXME this could be done way easier */
  current_buffer++;
  if( current_buffer == 2 )
    current_buffer = 0;
}

static void
sound_display_mmresult( const char * const func, MMRESULT result )
{
  const char *mmresult;

  switch ( result ) {
    case WAVERR_BADFORMAT:     mmresult = "WAVERR_BADFORMAT"; break;
    case WAVERR_SYNC:          mmresult = "WAVERR_SYNC"; break;
    case WAVERR_STILLPLAYING:  mmresult = "WAVERR_STILLPLAYING"; break;
    case MMSYSERR_NOERROR:     mmresult = "MMSYSERR_NOERROR"; break;
    case MMSYSERR_BADDEVICEID: mmresult = "MMSYSERR_BADDEVICEID"; break;
    case MMSYSERR_NODRIVER:    mmresult = "MMSYSERR_NODRIVER"; break;
    case MMSYSERR_INVALHANDLE: mmresult = "MMSYSERR_INVALHANDLE"; break;
    case MMSYSERR_NOMEM:       mmresult = "MMSYSERR_NOMEM"; break;
    case MMSYSERR_ALLOCATED:   mmresult = "MMSYSERR_ALLOCATED"; break;
    default:                   mmresult = "UNKNOWN"; break;
  }

  ui_error( UI_ERROR_ERROR, "%s error: %s(error no: %d)", func, mmresult, result );
}

static void CALLBACK
sound_callback( HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
  if( uMsg == WOM_DONE ) {
    EnterCriticalSection( &sound_lock );
    ReleaseSemaphore( sem_sound_done, 1, NULL );
    LeaveCriticalSection( &sound_lock );
  }
}
