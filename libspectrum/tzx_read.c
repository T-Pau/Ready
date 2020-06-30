/* tzx_read.c: Routines for reading .tzx files
   Copyright (c) 2001-2015 Philip Kendall, Darren Salt

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

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "internals.h"

/* The .tzx file signature (first 8 bytes) */
const char * const libspectrum_tzx_signature = "ZXTape!\x1a";

/*** Local function prototypes ***/

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end );
static libspectrum_error
tzx_read_pure_tone( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_pulses_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end );
static libspectrum_error
tzx_read_pure_data( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_raw_data( libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end );
static libspectrum_error
tzx_read_generalised_data( libspectrum_tape *tape,
			   const libspectrum_byte **ptr,
			   const libspectrum_byte *end );
static libspectrum_error
tzx_read_pause( libspectrum_tape *tape, const libspectrum_byte **ptr,
		const libspectrum_byte *end );
static libspectrum_error
tzx_read_group_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end );
static libspectrum_error
tzx_read_jump( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end );
static libspectrum_error
tzx_read_loop_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		     const libspectrum_byte *end );
static libspectrum_error
tzx_read_select( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end );
static libspectrum_error
tzx_read_stop( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end );
static libspectrum_error
tzx_read_set_signal_level( libspectrum_tape *tape, const libspectrum_byte **ptr,
                           const libspectrum_byte *end );
static libspectrum_error
tzx_read_comment( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end );
static libspectrum_error
tzx_read_message( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end );
static libspectrum_error
tzx_read_archive_info( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end );
static libspectrum_error
tzx_read_hardware( libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end );
static libspectrum_error
tzx_read_custom( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end );
static libspectrum_error
tzx_read_concat( const libspectrum_byte **ptr, const libspectrum_byte *end );

static void
tzx_read_empty_block( libspectrum_tape *tape, libspectrum_tape_type id );

static libspectrum_error
tzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t *length, int bytes, libspectrum_byte **data );
static libspectrum_error
tzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 char **dest );

/*** Function definitions ***/

/* The main load function */

libspectrum_error
internal_tzx_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		   const size_t length )
{

  libspectrum_error error;

  const libspectrum_byte *ptr, *end;
  size_t signature_length = strlen( libspectrum_tzx_signature );

  ptr = buffer; end = buffer + length;

  /* Must be at least as many bytes as the signature, and the major/minor
     version numbers */
  if( length < signature_length + 2 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_tzx_create: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Now check the signature */
  if( memcmp( ptr, libspectrum_tzx_signature, signature_length ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_tzx_create: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }
  ptr += signature_length;
  
  /* Just skip the version numbers */
  ptr += 2;

  while( ptr < end ) {

    /* Get the ID of the next block */
    libspectrum_tape_type id = *ptr++;

    switch( id ) {
    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = tzx_read_rom_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = tzx_read_turbo_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tzx_read_pure_tone( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = tzx_read_pulses_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = tzx_read_pure_data( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
      error = tzx_read_raw_data( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
      error = tzx_read_generalised_data( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = tzx_read_pause( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      error = tzx_read_group_start( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      tzx_read_empty_block( tape, id );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = tzx_read_jump( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      error = tzx_read_loop_start( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      tzx_read_empty_block( tape, id );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      error = tzx_read_select( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      error = tzx_read_stop( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
      error = tzx_read_set_signal_level( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      error = tzx_read_comment( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      error = tzx_read_message( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      error = tzx_read_archive_info( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      error = tzx_read_hardware( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = tzx_read_custom( tape, &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
      error = tzx_read_concat( &ptr, end );
      if( error ) { libspectrum_tape_clear( tape ); return error; }
      break;

    default:	/* For now, don't handle anything else */
      libspectrum_tape_clear( tape );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
        "libspectrum_tzx_create: unknown block type 0x%02x", id
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  size_t length; libspectrum_byte *data;
  libspectrum_error error;

  /* Check there's enough left in the buffer for the pause and the
     data length */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_rom_block: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );

  /* Get the pause length */
  libspectrum_set_pause_ms( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  /* And the data */
  error = tzx_read_data( ptr, end, &length, 2, &data );
  if( error ) { libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  size_t length; libspectrum_byte *data;
  libspectrum_byte bits_in_last_byte;
  libspectrum_error error;

  /* Check there's enough left in the buffer for all the metadata */
  if( end - (*ptr) < 18 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_turbo_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_TURBO );

  /* Get the metadata */
  libspectrum_tape_block_set_pilot_length( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_sync1_length( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_sync2_length( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_bit0_length ( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_bit1_length ( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_pilot_pulses( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  bits_in_last_byte = **ptr;
  (*ptr)++;

  libspectrum_set_pause_ms               ( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  /* Read the data in */
  error = tzx_read_data( ptr, end, &length, 3, &data );
  if( error ) { libspectrum_free( block ); return error; }

  if( bits_in_last_byte == 0 && length >= 1 ) {
    bits_in_last_byte = 8;
    length -= 1;
  }

  if( bits_in_last_byte > 8 ) {
    bits_in_last_byte = 8;
  }

  libspectrum_tape_block_set_bits_in_last_byte( block, bits_in_last_byte );

  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pure_tone( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;

  /* Check we've got enough bytes */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_pure_tone: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PURE_TONE );

  /* Read in the data, and move along */
  libspectrum_tape_block_set_pulse_length( block,
					   (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_count( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  
  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pulses_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_dword *lengths;
  size_t i, count;

  /* Check the count byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_pulses_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PULSES );

  /* Get the count byte */
  count = **ptr; (*ptr)++;
  libspectrum_tape_block_set_count( block, count );

  /* Check enough data exists for every pulse */
  if( end - (*ptr) < (ptrdiff_t)( 2 * count ) ) {
    libspectrum_free( block );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_pulses_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  lengths = libspectrum_new( libspectrum_dword, count );

  /* Copy the data across */
  for( i = 0; i < count; i++ ) {
    lengths[i] = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  }
  libspectrum_tape_block_set_pulse_lengths( block, lengths );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pure_data( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  size_t length; libspectrum_byte *data;
  libspectrum_byte bits_in_last_byte;
  libspectrum_error error;

  /* Check there's enough left in the buffer for all the metadata */
  if( end - (*ptr) < 10 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_pure_data: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PURE_DATA );

  /* Get the metadata */
  libspectrum_tape_block_set_bit0_length( block,
					  (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;
  libspectrum_tape_block_set_bit1_length( block,
					  (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  bits_in_last_byte = **ptr;
  (*ptr)++;

  libspectrum_set_pause_ms( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  /* And the actual data */
  error = tzx_read_data( ptr, end, &length, 3, &data );
  if( error ) { libspectrum_free( block ); return error; }

  if( bits_in_last_byte == 0 && length > 1 ) {
    bits_in_last_byte = 8;
    length -= 1;
  }

  if( bits_in_last_byte > 8 ) {
    bits_in_last_byte = 8;
  }

  libspectrum_tape_block_set_bits_in_last_byte( block, bits_in_last_byte );
  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_raw_data (libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end)
{
  libspectrum_tape_block* block;
  size_t length; libspectrum_byte *data;
  libspectrum_byte bits_in_last_byte;
  libspectrum_error error;

  /* Check there's enough left in the buffer for all the metadata */
  if (end - (*ptr) < 8) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_raw_data: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );

  /* Get the metadata */
  libspectrum_tape_block_set_bit_length( block,
					 (*ptr)[0] + (*ptr)[1] * 0x100 );
  libspectrum_set_pause_ms( block, (*ptr)[2] + (*ptr)[3] * 0x100 );

  bits_in_last_byte = (*ptr)[4];
  (*ptr) += 5;

  /* And the actual data */
  error = tzx_read_data( ptr, end, &length, 3, &data );
  if( error ) { libspectrum_free( block ); return error; }

  if( bits_in_last_byte == 0 && length >= 1 ) {
    bits_in_last_byte = 8;
    length -= 1;
  }

  if( bits_in_last_byte > 8 ) {
    bits_in_last_byte = 8;
  }

  libspectrum_tape_block_set_bits_in_last_byte( block, bits_in_last_byte );
  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_generalised_data( libspectrum_tape *tape,
			   const libspectrum_byte **ptr,
			   const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  libspectrum_dword length, symbol_count, data_count;
  libspectrum_word symbol_count2;
  libspectrum_error error;
  libspectrum_tape_generalised_data_symbol_table *table;
  libspectrum_byte *symbols, *data;
  libspectrum_word *repeats;

  const libspectrum_byte *blockend, *ptr2;
  size_t i, data_size;
  int bits_per_symbol;

  /* Check the length exists */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: not enough data in buffer", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  length = libspectrum_read_dword( ptr );

  blockend = *ptr + length;

  /* Sanity check */
  if( length < 14 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: length less than minimum", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check this much data exists */
  if( end - (*ptr) < length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: not enough data in buffer", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA );

  libspectrum_tape_block_zero( block );

  libspectrum_set_pause_ms( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  error = libspectrum_tape_block_read_symbol_table_parameters( block, 1, ptr );
  if( error ) { libspectrum_tape_block_free( block ); return error; }

  error = libspectrum_tape_block_read_symbol_table_parameters( block, 0, ptr );
  if( error ) { libspectrum_tape_block_free( block ); return error; }

  length -= 14;

  ptr2 = *ptr;

  table = libspectrum_tape_block_pilot_table( block );
  error = libspectrum_tape_block_read_symbol_table( table, ptr, length );
  if( error ) { libspectrum_tape_block_free( block ); return error; }

  length -= ptr2 - *ptr;

  symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );

  if( length < 3 * symbol_count ) {
    libspectrum_tape_block_free( block );
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: not enough data in buffer", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  symbols = libspectrum_new( libspectrum_byte, symbol_count );
  repeats = libspectrum_new( libspectrum_word, symbol_count );

  for( i = 0; i < symbol_count; i++ ) {
    symbols[ i ] = **ptr; (*ptr)++;
    repeats[ i ] = (*ptr)[0] + 0x100 * (*ptr)[1]; (*ptr) += 2;
  }

  libspectrum_tape_block_set_pilot_symbols( block, symbols );
  libspectrum_tape_block_set_pilot_repeats( block, repeats );

  length -= 3 * symbol_count;

  ptr2 = *ptr;

  table = libspectrum_tape_block_data_table( block );
  libspectrum_tape_block_read_symbol_table( table, ptr, length );

  length -= ptr2 - *ptr;

  symbol_count2 = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );

  bits_per_symbol = ceil( log( symbol_count2 ) / M_LN2 );

  libspectrum_tape_block_set_bits_per_data_symbol( block, bits_per_symbol );

  symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );

  data_count = ( ( bits_per_symbol * symbol_count ) + 7 ) / 8;
  data_size = data_count * sizeof( *data );

  data = libspectrum_new( libspectrum_byte, data_size );

  if( end - (*ptr) < data_size ) {
    libspectrum_free( data );
    libspectrum_tape_block_free( block );
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: data extends beyond end of block", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( data, *ptr, data_count * sizeof( *data ) );
  *ptr += data_count;

  libspectrum_tape_block_set_data( block, data );

  /* Sanity check */
  if( *ptr != blockend ) {
    libspectrum_tape_block_free( block );
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s: sanity check failed", __func__ );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_pause( libspectrum_tape *tape, const libspectrum_byte **ptr,
		const libspectrum_byte *end )
{
  libspectrum_tape_block *block;

  /* Check the pause actually exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_pause: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PAUSE );

  /* Get the pause length */
  libspectrum_set_pause_ms( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  /* TZX format spec says pause is low */
  libspectrum_tape_block_set_level( block, 0 );
  (*ptr) += 2;

  libspectrum_tape_append_block( tape, block );

  /* And return */
  return LIBSPECTRUM_ERROR_NONE;
}
  
static libspectrum_error
tzx_read_group_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  char *name;
  libspectrum_error error;
  
  /* Check the length byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_group_start: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_GROUP_START );

  /* Read in the description of the group */
  error = tzx_read_string( ptr, end, &name );
  if( error ) { libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_text( block, name );
			  
  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_jump( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  int offset;
  
  /* Check the offset exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_jump: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_JUMP );

  /* Get the offset */
  offset = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  if( offset >= 32768 ) offset -= 65536;
  libspectrum_tape_block_set_offset( block, offset);

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_loop_start( libspectrum_tape *tape, const libspectrum_byte **ptr,
		     const libspectrum_byte *end )
{
  libspectrum_tape_block *block;
  
  /* Check the count exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_loop_start: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_LOOP_START );

  /* Get the repeat count */
  libspectrum_tape_block_set_count( block, (*ptr)[0] + (*ptr)[1] * 0x100 );
  (*ptr) += 2;

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_select( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_error error;

  size_t length, count, i, j;
  int *offsets;
  char **descriptions;

  /* Check there's enough left in the buffer for the length and the count
     byte */
  if( end - (*ptr) < 3 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_select: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length, and check we've got that many bytes in the buffer */
  length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  if( end - (*ptr) < (ptrdiff_t)length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_select: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_SELECT );

  /* Get the number of selections */
  count = **ptr; (*ptr)++;
  libspectrum_tape_block_set_count( block, count );

  /* Allocate memory */
  offsets = libspectrum_new( int, count );
  libspectrum_tape_block_set_offsets( block, offsets );

  descriptions = libspectrum_new( char *, count );
  libspectrum_tape_block_set_texts( block, descriptions );

  /* Read in the data */
  for( i = 0; i < count; i++ ) {

    /* Check we've got the offset and a length byte */
    if( end - (*ptr) < 3 ) {
      for( j = 0; j < i; j++ ) libspectrum_free( descriptions[j] );
      libspectrum_free( descriptions ); libspectrum_free( offsets ); libspectrum_free( block );
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "tzx_read_select: not enough data in buffer" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get the offset */
    offsets[i] = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

    /* Get the description of this selection */
    error = tzx_read_string( ptr, end, &descriptions[i] );
    if( error ) {
      for( j = 0; j < i; j++ ) libspectrum_free( descriptions[j] );
      libspectrum_free( descriptions ); libspectrum_free( offsets ); libspectrum_free( block );
      return error;
    }

  }

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_stop( libspectrum_tape *tape, const libspectrum_byte **ptr,
	       const libspectrum_byte *end )
{
  libspectrum_tape_block *block;

  /* Check the length field exists */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_stop: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* But then just skip over it, as I don't care what it is */
  (*ptr) += 4;

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_STOP48 );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}  

static libspectrum_error
tzx_read_set_signal_level( libspectrum_tape *tape, const libspectrum_byte **ptr,
                           const libspectrum_byte *end )
{
  libspectrum_tape_block *block;

  /* Check the length field exists and signal level is available */
  if( end - (*ptr) < 5 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
                       "tzx_read_set_signal_level: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* But then just skip over it, as I don't care what it is */
  (*ptr) += 4;

  block =
    libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL );

  libspectrum_tape_block_set_level( block, !!(**ptr) ); (*ptr)++;

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}  

static libspectrum_error
tzx_read_comment( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  char *text;
  libspectrum_error error;

  /* Check the length byte exists */
  if( (*ptr) == end ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_comment: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_COMMENT );

  /* Get the actual comment */
  error = tzx_read_string( ptr, end, &text );
  if( error ) { libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_text( block, text );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_message( libspectrum_tape *tape, const libspectrum_byte **ptr,
		  const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  char *text;
  libspectrum_error error;

  /* Check the time and length byte exists */
  if( end - (*ptr) < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_message: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_MESSAGE );

  /* Get the time in seconds */
  libspectrum_set_pause_ms( block, (**ptr) * 1000 ); (*ptr)++;

  /* Get the message itself */
  error = tzx_read_string( ptr, end, &text );
  if( error ) { libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_text( block, text );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_archive_info( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_error error;

  size_t i, count;
  int *ids;
  char **strings;

  /* Check there's enough left in the buffer for the length and the count
     byte */
  if( end - (*ptr) < 3 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "tzx_read_archive_info: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO );

  /* Skip the length, as I don't care about it */
  (*ptr) += 2;

  /* Get the number of string */
  count = **ptr; (*ptr)++;
  libspectrum_tape_block_set_count( block, count );

  /* Allocate memory */
  ids = libspectrum_new( int, count );
  libspectrum_tape_block_set_ids( block, ids );

  strings = libspectrum_new( char *, count );
  libspectrum_tape_block_set_texts( block, strings );

  for( i = 0; i < count; i++ ) {

    /* Must be ID byte and length byte */
    if( end - (*ptr) < 2 ) {
      size_t j;
      for( j=0; j<i; j++ ) libspectrum_free( strings[j] );
      libspectrum_free( strings ); libspectrum_free( ids ); libspectrum_free( block );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "tzx_read_archive_info: not enough data in buffer"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get the ID byte */
    ids[i] = **ptr; (*ptr)++;

    /* Read in the string itself */
    error = tzx_read_string( ptr, end, &strings[i] );
    if( error ) {
      size_t j;
      for( j = 0; j < i; j++ ) libspectrum_free( strings[j] );
      libspectrum_free( strings ); libspectrum_free( ids ); libspectrum_free( block );
      return error;
    }

  }

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_hardware( libspectrum_tape *tape, const libspectrum_byte **ptr,
		   const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  size_t i, count;
  int *types, *ids, *values;

  /* Check there's enough left in the buffer for the count byte */
  if( (*ptr) == end ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_hardware: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_HARDWARE );

  /* Get the number of string */
  count = **ptr; (*ptr)++;
  libspectrum_tape_block_set_count( block, count );

  /* Check there's enough data in the buffer for all the data */
  if( end - (*ptr) < 3 * (ptrdiff_t)count ) {
    libspectrum_free( block );
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_hardware: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory */
  types = libspectrum_new( int, count );
  libspectrum_tape_block_set_types( block, types );

  ids = libspectrum_new( int, count );
  libspectrum_tape_block_set_ids( block, ids );

  values = libspectrum_new( int, count );
  libspectrum_tape_block_set_values( block, values );

  /* Actually read in all the data */
  for( i = 0; i < count; i++ ) {
    types[i]  = **ptr; (*ptr)++;
    ids[i]    = **ptr; (*ptr)++;
    values[i] = **ptr; (*ptr)++;
  }

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_read_custom( libspectrum_tape *tape, const libspectrum_byte **ptr,
		 const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  char *description;
  libspectrum_byte* data; size_t length;
  libspectrum_error error;

  /* Check the description (16) and length bytes (4) exist */
  if( end - (*ptr) < 20 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_custom: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_CUSTOM );

  /* Get the description */
  description = libspectrum_new( char, 17 );
  memcpy( description, *ptr, 16 ); (*ptr) += 16; description[16] = '\0';
  libspectrum_tape_block_set_text( block, description );

  /* Read in the data */
  error = tzx_read_data( ptr, end, &length, 4, &data );
  if( error ) { libspectrum_free( description ); libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_data_length( block, length );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Concatenation block: just skip it entirely as it serves no useful
   purpose */
static libspectrum_error
tzx_read_concat( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t signature_length = strlen( libspectrum_tzx_signature );

  /* Check there's enough data left; the -1 here is because we've already
     read the first byte of the signature as the block ID */
  if( end - (*ptr) < (ptrdiff_t)signature_length + 2 - 1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_concat: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Skip the data */
  *ptr += signature_length + 2 - 1;

  return LIBSPECTRUM_ERROR_NONE;
}
  
static void
tzx_read_empty_block( libspectrum_tape *tape, libspectrum_tape_type id )
{
  libspectrum_tape_block *block;
  block = libspectrum_tape_block_alloc( id );
  libspectrum_tape_append_block( tape, block );
}  

static libspectrum_error
tzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t *length, int bytes, libspectrum_byte **data )
{
  int i; libspectrum_dword multiplier = 0x01;
  size_t padding;

  if( bytes < 0 ) {
    bytes = -bytes; padding = 1;
  } else {
    padding = 0;
  }

  (*length) = 0;
  for( i=0; i<bytes; i++, multiplier *= 0x100 ) {
    *length += **ptr * multiplier; (*ptr)++;
  }

  /* Have we got enough bytes left in buffer? */
  if( ( end - (*ptr) ) < (ptrdiff_t)(*length) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_data: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data; the check for *length is to avoid
     the implementation-defined behaviour of malloc( 0 ) */
  if( *length || padding ) {
    *data = libspectrum_new( libspectrum_byte, *length + padding );
    /* Copy the block data across, and move along */
    memcpy( *data, *ptr, *length ); *ptr += *length;
  } else {
    *data = NULL;
  }

  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 char **dest )
{
  size_t length;
  libspectrum_error error;
  char *ptr2;

  error = tzx_read_data( ptr, end, &length, -1, (libspectrum_byte**)dest );
  if( error ) return error;
  
  /* Null terminate the string */
  (*dest)[length] = '\0';

  /* Translate line endings */
  for( ptr2 = (*dest); *ptr2; ptr2++ ) if( *ptr2 == '\r' ) *ptr2 = '\n';

  return LIBSPECTRUM_ERROR_NONE;
}
