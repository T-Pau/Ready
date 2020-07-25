/* tap.c: Routines for handling .tap files
   Copyright (c) 2001-2003 Philip Kendall

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

#include <stddef.h>
#include <string.h>

#include "internals.h"

#define DESCRIPTION_LENGTH 256

static libspectrum_error
write_rom( libspectrum_tape_block *block, libspectrum_buffer *buffer,
	   libspectrum_id_t type );
static libspectrum_error
write_turbo( libspectrum_tape_block *block, libspectrum_buffer *buffer,
             libspectrum_id_t type );
static libspectrum_error
write_pure_data( libspectrum_tape_block *block, libspectrum_buffer *buffer,
                 libspectrum_id_t type );

static libspectrum_error
write_tap_block( libspectrum_buffer *buffer, libspectrum_byte *data,
                 size_t data_length, libspectrum_id_t type );
static libspectrum_error
skip_block( libspectrum_tape_block *block, const char *message );

libspectrum_error
internal_tap_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		   const size_t length, libspectrum_id_t type )
{
  libspectrum_tape_block *block;
  size_t data_length, buf_length; libspectrum_byte *data;

  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  while( ptr < end ) {
    
    /* If we've got less than two bytes for the length, something's
       gone wrong, so gone home */
    if( ( end - ptr ) < 2 ) {
      libspectrum_tape_clear( tape );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_tap_read: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );

    /* Get the length, and move along the buffer */
    data_length = ptr[0] + ptr[1] * 0x100;
    if( type == LIBSPECTRUM_ID_TAPE_SPC ||
	type == LIBSPECTRUM_ID_TAPE_STA ||
	type == LIBSPECTRUM_ID_TAPE_LTP )
      data_length += 2;
    libspectrum_tape_block_set_data_length( block, data_length );
    ptr += 2;

    if( type == LIBSPECTRUM_ID_TAPE_STA )
      buf_length = data_length - 1;
    else
      buf_length = data_length;

    /* Have we got enough bytes left in buffer? */
    if( end - ptr < (ptrdiff_t)buf_length ) {
      libspectrum_tape_clear( tape );
      libspectrum_free( block );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_tap_read: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Allocate memory for the data */
    data = libspectrum_new( libspectrum_byte, data_length );
    libspectrum_tape_block_set_data( block, data );

    /* Copy the block data across */
    memcpy( data, ptr, buf_length );

    /* Fix the parity byte for the SPC and STA tape formats */
    if( type == LIBSPECTRUM_ID_TAPE_SPC ) {
      data[ data_length - 1 ] ^= data[0];
    } else if( type == LIBSPECTRUM_ID_TAPE_STA ) {
      libspectrum_byte parity;
      size_t i;

      parity = 0x00;
      for( i = 0; i < data_length - 1; i++ ) {
	parity ^= data[i];
      }
      data[ data_length - 1 ] = parity;
    }
 
    /* Move along the buffer */
    ptr += buf_length;

    /* Give a 1s pause after each block */
    libspectrum_set_pause_ms( block, 1000 );

    libspectrum_tape_append_block( tape, block );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
internal_tap_write( libspectrum_buffer *buffer, libspectrum_tape *tape,
                    libspectrum_id_t type )
{
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *block;
  libspectrum_error error;

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator ) )
  {
    int done = 0;

    switch( libspectrum_tape_block_type( block ) ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = write_rom( block, buffer, type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      done = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = write_turbo( block, buffer, type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      done = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = write_pure_data( block, buffer, type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      done = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:     /* Could do better? */
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
    case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
      error = skip_block( block, "conversion almost certainly won't work" );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return 1; }
      done = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
      error = skip_block( block, "conversion may not work" );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return 1; }
      done = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
      error = skip_block( block, NULL );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return 1; }
      done = 1;
      break;
    }

    if( !done ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
        "libspectrum_tap_write: unknown block type 0x%02x",
        libspectrum_tape_block_type( block )
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_rom( libspectrum_tape_block *block, libspectrum_buffer *buffer,
	   libspectrum_id_t type )
{
  libspectrum_error error;

  error = write_tap_block( buffer, libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ), type );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_turbo( libspectrum_tape_block *block, libspectrum_buffer *buffer,
             libspectrum_id_t type )
{
  libspectrum_error error;

  /* Print out a warning about converting a turbo block */
  libspectrum_print_error(
    LIBSPECTRUM_ERROR_WARNING,
    "write_turbo: converting Turbo Speed Data Block (ID 0x11); conversion may well not work"
  );

  error = write_tap_block( buffer, libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ), type );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_pure_data( libspectrum_tape_block *block, libspectrum_buffer *buffer,
		 libspectrum_id_t type )
{
  libspectrum_error error;

  /* Print out a warning about converting a pure data block */
  libspectrum_print_error(
    LIBSPECTRUM_ERROR_WARNING,
    "write_pure_data: converting Pure Data Block (ID 0x14); conversion almost certainly won't work"
  );

  error = write_tap_block( buffer, libspectrum_tape_block_data( block ),
			   libspectrum_tape_block_data_length( block ), type );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_tap_block( libspectrum_buffer* buffer, libspectrum_byte *data,
                 size_t data_length, libspectrum_id_t type )
{
  size_t buf_length;
  libspectrum_byte parity;

  /* Discard the parity byte for STA files */
  if( type == LIBSPECTRUM_ID_TAPE_STA )
    buf_length = data_length - 1;
  else
    buf_length = data_length;

  if( type == LIBSPECTRUM_ID_TAPE_SPC ||
      type == LIBSPECTRUM_ID_TAPE_STA ||
      type == LIBSPECTRUM_ID_TAPE_LTP ) {
    if( data_length < 2 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			       "write_tap_block: block too short" );
      return LIBSPECTRUM_ERROR_INVALID;
    }
    data_length -= 2;
  }

  /* Write out the length and the data */
  libspectrum_buffer_write_byte( buffer, data_length & 0x00ff );
  libspectrum_buffer_write_byte( buffer, ( data_length & 0xff00 ) >> 8 );
  libspectrum_buffer_write( buffer, data, buf_length - 1 );
  
  /* Fix the parity for SPC files */
  parity = data[ buf_length - 1];
  if( type == LIBSPECTRUM_ID_TAPE_SPC )
    parity ^= data[0];
  libspectrum_buffer_write_byte( buffer, parity );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_block( libspectrum_tape_block *block, const char *message )
{
  char description[ DESCRIPTION_LENGTH ];
  libspectrum_error error;

  error = libspectrum_tape_block_description( description, DESCRIPTION_LENGTH,
					      block );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  if( message ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_WARNING,
			     "skip_block: skipping %s (ID 0x%02x); %s",
			     description, libspectrum_tape_block_type( block ),
			     message );
  } else {
    libspectrum_print_error( LIBSPECTRUM_ERROR_WARNING,
			     "skip_block: skipping %s (ID 0x%02x)",
			     description,
			     libspectrum_tape_block_type( block ) );
  }

  return LIBSPECTRUM_ERROR_NONE;
}
