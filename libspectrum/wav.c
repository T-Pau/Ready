/* wav.c: Routines for handling WAV raw audio files
   Copyright (c) 2007-2015 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>
#include <string.h>

#ifdef HAVE_LIB_AUDIOFILE

#include <audiofile.h>

#include "internals.h"
#include "tape_block.h"

libspectrum_error
libspectrum_wav_read( libspectrum_tape *tape, const char *filename )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_byte *tape_buffer; size_t tape_length;
  size_t data_length;
  libspectrum_tape_block *block = NULL;
  int frames;

  /* Our filehandle from libaudiofile */
  AFfilehandle handle;

  /* The track we're using in the file */
  int track = AF_DEFAULT_TRACK; 

  if( !filename ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_wav_read: no filename provided - wav files can only be loaded from a file"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  handle = afOpenFile( filename, "r", NULL );
  if( handle == AF_NULL_FILEHANDLE ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_wav_read: audiofile failed to open file:%s", filename
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  if( afSetVirtualSampleFormat( handle, track, AF_SAMPFMT_UNSIGNED, 8 ) ) {
    afCloseFile( handle );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_wav_read: audiofile failed to set virtual sample format"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  if( afSetVirtualChannels( handle, track, 1 ) ) {
    afCloseFile( handle );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_wav_read: audiofile failed to set virtual channel count"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  length = afGetFrameCount( handle, track );

  tape_length = length;
  if( tape_length%8 ) tape_length += 8 - (tape_length%8);

  buffer = libspectrum_new0( libspectrum_byte,
			     tape_length * afGetChannels(handle, track) );

  frames = afReadFrames( handle, track, buffer, length );
  if( frames == -1 ) {
    libspectrum_free( buffer );
    afCloseFile( handle );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_wav_read: can't calculate number of frames in audio file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( !length ) {
    libspectrum_free( buffer );
    afCloseFile( handle );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_wav_read: empty audio file, nothing to load"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( frames != length ) {
    libspectrum_free( buffer );
    afCloseFile( handle );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_wav_read: read %d frames, but expected %lu\n", frames,
      (unsigned long)length
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );

  /* 44100 Hz 79 t-states 22050 Hz 158 t-states */
  libspectrum_tape_block_set_bit_length( block,
                                         3500000/afGetRate( handle, track ) );
  libspectrum_set_pause_ms( block, 0 );
  libspectrum_tape_block_set_bits_in_last_byte( block,
              length % LIBSPECTRUM_BITS_IN_BYTE ?
                length % LIBSPECTRUM_BITS_IN_BYTE : LIBSPECTRUM_BITS_IN_BYTE );
  data_length = tape_length / LIBSPECTRUM_BITS_IN_BYTE;
  libspectrum_tape_block_set_data_length( block, data_length );

  tape_buffer = libspectrum_new0( libspectrum_byte, data_length );

  libspectrum_byte *from = buffer;
  libspectrum_byte *to = tape_buffer;
  length = tape_length;
  do {
    libspectrum_byte val = 0;
    int i;
    for( i = 7; i >= 0; i-- ) {
      if( *from++ > 127 ) val |= 1 << i;
    }
    *to++ = val;
  } while ((length -= 8) > 0);

  libspectrum_tape_block_set_data( block, tape_buffer );

  libspectrum_tape_append_block( tape, block );

  if( afCloseFile( handle ) ) {
    libspectrum_free( buffer );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_wav_read: failed to close audio file"
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_free( buffer );

  /* Successful completion */
  return LIBSPECTRUM_ERROR_NONE;
}

#endif    /* #ifdef HAVE_LIB_AUDIOFILE */
