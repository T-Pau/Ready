/* csw.c: Routines for handling CSW raw audio files
   Copyright (c) 2002-2015 Darren Salt, Fredrick Meunier
   Based on tap.c, copyright (c) 2001 Philip Kendall

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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>
#include <string.h>

#include "internals.h"
#include "tape_block.h"

/* The .csw file signature (first 23 bytes) */
static const char * const csw_signature = "Compressed Square Wave\x1a";

libspectrum_error
libspectrum_csw_read( libspectrum_tape *tape,
		      const libspectrum_byte *buffer, size_t length )
{
  libspectrum_tape_block *block = NULL;
  libspectrum_tape_rle_pulse_block *csw_block;

  int compressed;

  size_t signature_length = strlen( csw_signature );

  if( length < signature_length + 2 ) goto csw_short;

  if( memcmp( csw_signature, buffer, signature_length ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_csw_read: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Claim memory for the block */
  block = libspectrum_new( libspectrum_tape_block, 1 );

  /* Set the block type */
  block->type = LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE;
  csw_block = &block->types.rle_pulse;

  buffer += signature_length;
  length -= signature_length;

  switch( buffer[0] ) {

  case 1:
    if( length < 9 ) goto csw_short;
    csw_block->scale = buffer[2] | buffer[3] << 8;
    if( buffer[4] != 1 ) goto csw_bad_compress;
    compressed = 0;
    buffer += 9;
    length -= 9;
    break;

  case 2:
    if( length < 29 ) goto csw_short;

    csw_block->scale =
      buffer[2]       |
      buffer[3] <<  8 |
      buffer[4] << 16 |
      buffer[5] << 24;
    compressed = buffer[10] - 1;

    if( compressed != 0 && compressed != 1 ) goto csw_bad_compress;

    if( length < 29 - buffer[12] ) goto csw_short;
    length -= 29 - buffer[12];
    buffer += 29 + buffer[12];

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_csw_read: unknown CSW version" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  if (csw_block->scale)
    csw_block->scale = 3500000 / csw_block->scale; /* approximate CPU speed */

  if( csw_block->scale < 0 || csw_block->scale >= 0x80000 ) {
    libspectrum_print_error (LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_csw_read: bad sample rate" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( !length ) goto csw_empty;

  if( compressed ) {
    /* Compressed data... */
#ifdef HAVE_ZLIB_H
    libspectrum_error error;

    csw_block->data = NULL;
    csw_block->length = 0;
    error = libspectrum_zlib_inflate( buffer, length, &csw_block->data,
                                      &csw_block->length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
#else
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "zlib not available to decompress gzipped file" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
#endif
  } else {
    /* Claim memory for the data (it's one big lump) */
    csw_block->length = length;
    csw_block->data = libspectrum_new( libspectrum_byte, length );

    /* Copy the data across */
    memcpy( csw_block->data, buffer, length );
  }

  libspectrum_tape_append_block( tape, block );

  /* Successful completion */
  return LIBSPECTRUM_ERROR_NONE;

  /* Error returns */

 csw_bad_compress:
  libspectrum_free( block );
  libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			   "libspectrum_csw_read: unknown compression type" );
  return LIBSPECTRUM_ERROR_CORRUPT;

 csw_short:
  libspectrum_free( block );
  libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			   "libspectrum_csw_read: not enough data in buffer" );
  return LIBSPECTRUM_ERROR_CORRUPT;

 csw_empty:
  libspectrum_free( block );
  /* Successful completion */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_dword
find_sample_rate( libspectrum_tape *tape )
{
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *block;
  libspectrum_dword sample_rate = 44100;
  int found = 0;

  /* FIXME: If tape has only one block that is a sampled type, just use it's rate
     and if it RLE, we should just zlib and write */
  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator ) )
  {
    switch( libspectrum_tape_block_type( block ) ) {

    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
      {
      libspectrum_dword block_rate =
        3500000 / libspectrum_tape_block_bit_length( block );

      if( found ) {
        if( block_rate != sample_rate ) {
          libspectrum_print_error(
            LIBSPECTRUM_ERROR_WARNING,
            "find_sample_rate: converting tape with mixed sample rates; conversion may well not work"
          );
        }
      }
      sample_rate = block_rate;
      found = 1;
      }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
      break;

    default:
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
        "find_sample_rate: unknown block type 0x%02x",
	libspectrum_tape_block_type( block )
      );

    }
  }

  return sample_rate;
}

static libspectrum_error
csw_write_body( libspectrum_buffer *buffer, libspectrum_tape *tape,
                libspectrum_dword sample_rate,
                size_t* body_uncompressed_length )
{
  libspectrum_error error;
  int flags = 0;
  libspectrum_dword pulse_tstates = 0;
  libspectrum_dword balance_tstates = 0;
  long scale = 3500000/sample_rate;
  libspectrum_tape_block_state it;

  if( libspectrum_tape_block_internal_init( &it, tape ) ) {
    while( !(flags & LIBSPECTRUM_TAPE_FLAGS_STOP) ) {
      libspectrum_dword pulse_length = 0;

      /* Use internal version of this that doesn't bugger up the
         external tape status */
      error = libspectrum_tape_get_next_edge_internal( &pulse_tstates, &flags,
                                                       tape, &it );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;

      balance_tstates += pulse_tstates;

      if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) continue;

      /* next RLE value is: balance_tstates / scale; */
      pulse_length = balance_tstates / scale;
      balance_tstates = balance_tstates % scale;

      if( pulse_length ) {
        if( pulse_length <= 0xff ) {
          libspectrum_buffer_write_byte( buffer, pulse_length );
        } else {
          libspectrum_buffer_write_byte( buffer, 0 );
          libspectrum_buffer_write_dword( buffer, pulse_length );
        }
      }
    }
  }

  /* Write the length in */
  *body_uncompressed_length = libspectrum_buffer_get_data_size( buffer );

  /* compression type */
#ifdef HAVE_ZLIB_H
  if( *body_uncompressed_length ) {
    libspectrum_byte *compressed_data = NULL;
    size_t compressed_length;

    error = libspectrum_zlib_compress( libspectrum_buffer_get_data( buffer ),
                                       libspectrum_buffer_get_data_size( buffer ),
                                       &compressed_data, &compressed_length );
    if( error ) return error;

    libspectrum_buffer_clear( buffer );
    libspectrum_buffer_write( buffer, compressed_data, compressed_length );
    libspectrum_free( compressed_data );
  }
#endif

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_csw_write( libspectrum_buffer *new_buffer, libspectrum_tape *tape )
{
  libspectrum_error error = LIBSPECTRUM_ERROR_NONE;
  libspectrum_dword sample_rate;
  size_t i;

  libspectrum_buffer* body_buffer = libspectrum_buffer_alloc();
  size_t body_uncompressed_length = 0;

  sample_rate = find_sample_rate( tape );

  error =
    csw_write_body( body_buffer, tape, sample_rate, &body_uncompressed_length );
  if( error != LIBSPECTRUM_ERROR_NONE ) goto exit;

  /* First, write the .csw signature and the rest of the header */
  libspectrum_buffer_write( new_buffer, csw_signature, strlen( csw_signature ) );

  libspectrum_buffer_write_byte( new_buffer, 2 ); /* Major version number */
  libspectrum_buffer_write_byte( new_buffer, 0 ); /* Minor version number */

  /* sample rate */
  libspectrum_buffer_write_dword( new_buffer, sample_rate );

  /* Store where the total number of pulses (after decompression) will
     be written, and skip over those bytes */
  libspectrum_buffer_write_dword( new_buffer, body_uncompressed_length );

  /* compression type */
#ifdef HAVE_ZLIB_H
  libspectrum_buffer_write_byte( new_buffer, 2 ); /* Z-RLE */
#else
  libspectrum_buffer_write_byte( new_buffer, 1 ); /* RLE */
#endif

  /* flags */
  libspectrum_buffer_write_byte( new_buffer, 0 );		/* No flags */

  /* header extension length in bytes */
  libspectrum_buffer_write_byte( new_buffer, 0 );		/* No header extension */

  /* encoding application description */
  /* No creator for now */
  for( i = 0; i < 16; i++ ) {
    libspectrum_buffer_write_byte( new_buffer, 0 );
  }

  /* header extension data is zero so on to the data */
  libspectrum_buffer_write_buffer( new_buffer, body_buffer );

exit:
  libspectrum_buffer_free( body_buffer );
  return error;
}
