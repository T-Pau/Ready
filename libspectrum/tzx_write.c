/* tzx_write.c: Routines for writing .tzx files
   Copyright (c) 2001-2007 Philip Kendall, Fredrick Meunier

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

#include <stdio.h>
#include <string.h>

#include "tape_block.h"
#include "internals.h"

/*** Local function prototypes ***/

static void
tzx_write_rom( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_turbo( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
add_pure_tone_block( libspectrum_buffer *buffer, libspectrum_dword pulse_length,
                     size_t count );
static void
tzx_write_pure_tone( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_data( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_raw_data( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static libspectrum_error
tzx_write_generalised_data( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_pulses( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_pause( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_group_start( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_jump( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_loop_start( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_select( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_stop( libspectrum_buffer* buffer );
static void
add_set_signal_level_block( libspectrum_buffer* buffer, int level );
static void
tzx_write_set_signal_level( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_comment( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_message( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_archive_info( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_hardware( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_custom( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static libspectrum_error
tzx_write_rle( libspectrum_tape_block *block, libspectrum_buffer* buffer,
               libspectrum_tape *tape,
               libspectrum_tape_iterator iterator );
static void
add_pulses_block( size_t pulse_count, libspectrum_dword *lengths,
                  libspectrum_tape_block *block, libspectrum_buffer* buffer );
static void
tzx_write_pulse_sequence( libspectrum_tape_block *block, libspectrum_buffer* buffer );
static libspectrum_error
tzx_write_data_block( libspectrum_tape_block *block, libspectrum_buffer* buffer );

static void
tzx_write_empty_block( libspectrum_buffer* buffer, libspectrum_tape_type id );

static void
tzx_write_bytes( libspectrum_buffer* buffer, size_t length, size_t length_bytes,
                 libspectrum_byte *data );
static void
tzx_write_string( libspectrum_buffer* buffer, char *string );

/*** Function definitions ***/

/* The main write function */

libspectrum_error
internal_tzx_write( libspectrum_buffer* buffer, libspectrum_tape *tape )
{
  libspectrum_error error;
  libspectrum_tape_iterator iterator;
  libspectrum_tape_block *block;

  size_t signature_length = strlen( libspectrum_tzx_signature );

  /* First, write the .tzx signature and the version numbers */
  libspectrum_buffer_write( buffer, libspectrum_tzx_signature, signature_length );

  libspectrum_buffer_write_byte( buffer, 1  ); /* Major version number */
  libspectrum_buffer_write_byte( buffer, 20 ); /* Minor version number */

  for( block = libspectrum_tape_iterator_init( &iterator, tape );
       block;
       block = libspectrum_tape_iterator_next( &iterator )       )
  {
    switch( libspectrum_tape_block_type( block ) ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      tzx_write_rom( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      tzx_write_turbo( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      tzx_write_pure_tone( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      tzx_write_pulses( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      tzx_write_data( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
      tzx_write_raw_data( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
      error = tzx_write_generalised_data( block, buffer );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      tzx_write_pause( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      tzx_write_group_start( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      tzx_write_empty_block( buffer, libspectrum_tape_block_type( block ) );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      tzx_write_jump( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      tzx_write_loop_start( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      tzx_write_empty_block( buffer, libspectrum_tape_block_type( block ) );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      tzx_write_select( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      tzx_write_stop( buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
      tzx_write_set_signal_level( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      tzx_write_comment( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      tzx_write_message( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      tzx_write_archive_info( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      tzx_write_hardware( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      tzx_write_custom( block, buffer );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
      error = tzx_write_rle( block, buffer, tape, iterator );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
      tzx_write_pulse_sequence( block, buffer );
      break;
    case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
      error = tzx_write_data_block( block, buffer );
      if( error != LIBSPECTRUM_ERROR_NONE ) { return error; }
      break;

    default:
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
	"libspectrum_tzx_write: unknown block type 0x%02x",
	libspectrum_tape_block_type( block )
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
tzx_write_rom( libspectrum_tape_block *block, libspectrum_buffer* buffer )
{
  /* Write the ID byte and the pause */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_ROM );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pause( block ) );

  /* Copy the data across */
  tzx_write_bytes( buffer, libspectrum_tape_block_data_length( block ), 2,
                   libspectrum_tape_block_data( block ) );
}

static void
tzx_write_turbo( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  /* Write the ID byte and the metadata */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_TURBO );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pilot_length( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_sync1_length( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_sync2_length( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_bit0_length ( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_bit1_length ( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pilot_pulses( block ) );
  libspectrum_buffer_write_byte( buffer,
                     libspectrum_tape_block_bits_in_last_byte( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pause       ( block ) );

  tzx_write_bytes( buffer, libspectrum_tape_block_data_length( block ), 3,
                   libspectrum_tape_block_data( block ) );
}

static void
add_pure_tone_block( libspectrum_buffer *buffer, libspectrum_dword pulse_length,
                     size_t count )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_PURE_TONE );
  libspectrum_buffer_write_word( buffer, pulse_length );
  libspectrum_buffer_write_word( buffer, count );
}

static void
tzx_write_pure_tone( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  add_pure_tone_block( buffer, libspectrum_tape_block_pulse_length( block ),
                       libspectrum_tape_block_count( block ) );
}

static void
tzx_write_pulses( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  size_t i;
  size_t count = libspectrum_tape_block_count( block );

  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_PULSES );
  libspectrum_buffer_write_byte( buffer, count );
  for( i = 0; i < count; i++ )
    libspectrum_buffer_write_word(
                            buffer,
			    libspectrum_tape_block_pulse_lengths( block, i ) );
}

static void
tzx_write_data( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  /* Write the ID byte and the metadata */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_PURE_DATA );
  libspectrum_buffer_write_word( buffer,
                                 libspectrum_tape_block_bit0_length( block ) );
  libspectrum_buffer_write_word( buffer,
                                 libspectrum_tape_block_bit1_length( block ) );
  libspectrum_buffer_write_byte( buffer,
                     libspectrum_tape_block_bits_in_last_byte( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pause( block ) );

  tzx_write_bytes( buffer, libspectrum_tape_block_data_length( block ), 3,
                   libspectrum_tape_block_data( block ) );
}

static void
tzx_write_raw_data( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  /* Write the ID byte and the metadata */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );
  libspectrum_buffer_write_word( buffer,
                                 libspectrum_tape_block_bit_length( block ) );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_pause( block ) );
  libspectrum_buffer_write_byte( buffer,
                     libspectrum_tape_block_bits_in_last_byte( block ) );

  tzx_write_bytes( buffer, libspectrum_tape_block_data_length( block ), 3,
                   libspectrum_tape_block_data( block ) );
}

static size_t
generalised_data_length( libspectrum_tape_generalised_data_symbol_table *pilot,
                         libspectrum_tape_generalised_data_symbol_table *data,
                         size_t data_bits_per_symbol )
{
  size_t data_length = 14;	/* Minimum if no tables or symbols present */

  if( pilot->symbols_in_block ) {

    data_length += ( 2 * pilot->max_pulses + 1 ) * pilot->symbols_in_table;
    data_length += 3 * pilot->symbols_in_block;

  }

  if( data->symbols_in_block ) {

    data_length += ( 2 * data->max_pulses + 1 ) * data->symbols_in_table;
    data_length +=
      libspectrum_bits_to_bytes( data_bits_per_symbol * data->symbols_in_block );

  }

  return data_length;
}

static libspectrum_error
serialise_generalised_data_table( libspectrum_buffer *buffer,
                                  libspectrum_tape_generalised_data_symbol_table *table )
{
  libspectrum_dword symbols_in_block;
  libspectrum_word symbols_in_table;

  symbols_in_block = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table );

  libspectrum_buffer_write_dword( buffer, symbols_in_block );
  libspectrum_buffer_write_byte( buffer, libspectrum_tape_generalised_data_symbol_table_max_pulses( table ) );

  symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );

  if( symbols_in_block != 0 &&
      ( symbols_in_table == 0 || symbols_in_table > 256 ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID, "%s: invalid number of symbols in table: %d", __func__, symbols_in_table );
    return LIBSPECTRUM_ERROR_INVALID;
  } else if( symbols_in_table == 256 ) {
    symbols_in_table = 0;
  }

  libspectrum_buffer_write_byte( buffer, symbols_in_table );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
serialise_generalised_data_symbols( libspectrum_buffer *buffer, libspectrum_tape_generalised_data_symbol_table *table )
{
  libspectrum_word symbols_in_table = libspectrum_tape_generalised_data_symbol_table_symbols_in_table( table );
  libspectrum_byte max_pulses = libspectrum_tape_generalised_data_symbol_table_max_pulses( table );

  libspectrum_word i;
  libspectrum_byte j;

  if( !libspectrum_tape_generalised_data_symbol_table_symbols_in_block( table ) ) return;

  for( i = 0; i < symbols_in_table; i++ ) {

    libspectrum_tape_generalised_data_symbol *symbol = libspectrum_tape_generalised_data_symbol_table_symbol( table, i );

    libspectrum_buffer_write_byte( buffer, libspectrum_tape_generalised_data_symbol_type( symbol ) );

    for( j = 0; j < max_pulses; j++ )
      libspectrum_buffer_write_word( buffer, libspectrum_tape_generalised_data_symbol_pulse( symbol, j ) );

  }
}

static libspectrum_error
write_generalised_data_block( libspectrum_tape_block *block,
                              libspectrum_buffer *buffer, size_t bits_per_symbol,
                              libspectrum_tape_generalised_data_symbol_table *pilot_table,
                              libspectrum_tape_generalised_data_symbol_table *data_table,
                              libspectrum_word pause_ms )
{
  size_t data_length;
  libspectrum_error error;
  libspectrum_dword pilot_symbol_count, data_symbol_count, i;

  data_length = generalised_data_length( pilot_table, data_table,
                                         bits_per_symbol );

  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA );
  libspectrum_buffer_write_dword( buffer, data_length );

  libspectrum_buffer_write_word( buffer, pause_ms );

  error = serialise_generalised_data_table( buffer, pilot_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = serialise_generalised_data_table( buffer, data_table );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  serialise_generalised_data_symbols( buffer, pilot_table );

  pilot_symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( pilot_table );

  for( i = 0; i < pilot_symbol_count; i++ ) {
    libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_pilot_symbols( block, i ) );
    libspectrum_buffer_write_word( buffer, libspectrum_tape_block_pilot_repeats( block, i ) );
  }

  serialise_generalised_data_symbols( buffer, data_table );

  data_symbol_count = libspectrum_tape_generalised_data_symbol_table_symbols_in_block( data_table );

  data_length =
    libspectrum_bits_to_bytes( bits_per_symbol * data_symbol_count );

  libspectrum_buffer_write( buffer, libspectrum_tape_block_data( block ), data_length );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_generalised_data( libspectrum_tape_block *block,
                            libspectrum_buffer *buffer )
{ 
  size_t bits_per_symbol;
  libspectrum_tape_generalised_data_symbol_table *pilot_table, *data_table;

  pilot_table = libspectrum_tape_block_pilot_table( block );
  data_table = libspectrum_tape_block_data_table( block );

  bits_per_symbol = libspectrum_tape_block_bits_per_data_symbol( block );

  return write_generalised_data_block( block, buffer,
                                       bits_per_symbol, pilot_table, data_table,
                                       libspectrum_tape_block_pause( block ) );
}

static void
tzx_write_pause( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  /* High pause when represented in the TZX format is really a set signal level
     1 and then a pulse as TZX format says that all pauses are low, a don't care
     pause is a pulse too */
  if( libspectrum_tape_block_level( block ) != 0 ) {
    if( libspectrum_tape_block_level( block ) == 1 ) {
      add_set_signal_level_block( buffer, 1 );
    }

    add_pure_tone_block( buffer,
                         libspectrum_tape_block_pause_tstates( block ), 1 );

    return;
  }

  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_PAUSE );
  libspectrum_buffer_write_word( buffer, libspectrum_tape_block_pause( block ) );
}

static void
tzx_write_group_start( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_GROUP_START );
  tzx_write_string( buffer, libspectrum_tape_block_text( block ) );
}

static void
tzx_write_jump( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  int u_offset;

  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_JUMP );

  u_offset = libspectrum_tape_block_offset( block );
  if( u_offset < 0 ) u_offset += 65536;
  libspectrum_buffer_write_word( buffer, u_offset );
}

static void
tzx_write_loop_start( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_LOOP_START );
  libspectrum_buffer_write_word( buffer,
                                   libspectrum_tape_block_count( block ) );
}

static void
tzx_write_select( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  size_t count, total_length, i;

  /* The id byte, the total length (2 bytes), the count byte,
     and ( 2 offset bytes and 1 length byte ) per selection */
  count = libspectrum_tape_block_count( block );
  total_length = 4 + 3 * count;

  for( i = 0; i < count; i++ )
    total_length += strlen( (char*)libspectrum_tape_block_texts( block, i ) );

  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_SELECT );
  libspectrum_buffer_write_word( buffer, total_length );
  libspectrum_buffer_write_byte( buffer, count );

  for( i = 0; i < count; i++ ) {
    libspectrum_buffer_write_word( buffer, libspectrum_tape_block_offsets( block, i ) );
    tzx_write_string( buffer, libspectrum_tape_block_texts( block, i ) );
  }
}

static void
tzx_write_stop( libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_STOP48 );
  libspectrum_buffer_write_dword( buffer, 0 );
}

static void
add_set_signal_level_block( libspectrum_buffer *buffer, int level )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL );
  libspectrum_buffer_write_dword( buffer, 1 );
  libspectrum_buffer_write_byte( buffer, level );
}

static void
tzx_write_set_signal_level( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  add_set_signal_level_block( buffer, libspectrum_tape_block_level( block ) );
}

static void
tzx_write_comment( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_COMMENT );
  tzx_write_string( buffer, libspectrum_tape_block_text( block ) );
}

static void
tzx_write_message( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_MESSAGE );
  libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_pause( block ) );
  tzx_write_string( buffer, libspectrum_tape_block_text( block ) );
}

static void
tzx_write_archive_info( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  size_t i, count, total_length;

  count = libspectrum_tape_block_count( block );

  /* 1 count byte, 2 bytes (ID and length) for every string */
  total_length = 1 + 2 * count;
  /* And then the length of all the strings */
  for( i = 0; i < count; i++ )
    total_length += strlen( (char*)libspectrum_tape_block_texts( block, i ) );

  /* Write out the metadata */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO );
  libspectrum_buffer_write_word( buffer, total_length );
  libspectrum_buffer_write_byte( buffer, count );

  /* And the strings */
  for( i = 0; i < count; i++ ) {
    libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_ids( block, i ) );
    tzx_write_string( buffer, libspectrum_tape_block_texts( block, i ) );
  }
}

static void
tzx_write_hardware( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  size_t i;
  size_t count = libspectrum_tape_block_count( block );

  /* Write out the metadata */
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_HARDWARE );
  libspectrum_buffer_write_byte( buffer, count );

  /* And the info */
  for( i = 0; i < count; i++ ) {
    libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_types( block, i ) );
    libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_ids  ( block, i ) );
    libspectrum_buffer_write_byte( buffer, libspectrum_tape_block_values( block, i ) );
  }
}

static void
tzx_write_custom( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_buffer_write_byte( buffer, LIBSPECTRUM_TAPE_BLOCK_CUSTOM );
  libspectrum_buffer_write( buffer, libspectrum_tape_block_text( block ), 16 );
  tzx_write_bytes( buffer, libspectrum_tape_block_data_length( block ), 4,
		   libspectrum_tape_block_data( block ) );
}

typedef struct {
  short bits_used; /* The bits used in the current byte in progress */
  short level; /* The last level output to this block */
  libspectrum_byte *tape_buffer; /* The buffer we are writing into */
  size_t tape_length; /* size of the buffer allocated so far */
  size_t length; /* size of the buffer used so far */
} rle_write_state;

static rle_write_state rle_state;

/* write a pulse of pulse_length bits into the tape_buffer */
static void
write_pulse( libspectrum_dword pulse_length )
{
  int i;
  size_t target_size = rle_state.length + pulse_length/8;

  if( rle_state.tape_length <= target_size ) {
    rle_state.tape_length = target_size * 2;
    rle_state.tape_buffer = libspectrum_renew( libspectrum_byte,
					       rle_state.tape_buffer,
					       rle_state.tape_length );
  }

  for( i = pulse_length; i > 0; i-- ) {
    if( rle_state.level ) 
      *(rle_state.tape_buffer + rle_state.length) |=
        1 << (7 - rle_state.bits_used);
    rle_state.bits_used++;

    if( rle_state.bits_used == 8 ) {
      rle_state.length++;
      *(rle_state.tape_buffer + rle_state.length) = 0;
      rle_state.bits_used = 0;
    }
  }

  rle_state.level = !rle_state.level;
}

/* Convert RLE block to a TZX DRB as TZX CSW block support is limited :/ */
static libspectrum_error
tzx_write_rle( libspectrum_tape_block *block, libspectrum_buffer *buffer,
               libspectrum_tape *tape,
               libspectrum_tape_iterator iterator )
{
  libspectrum_error error;
  libspectrum_tape_block_state it;
  libspectrum_dword scale = libspectrum_tape_block_scale( block );
  libspectrum_dword pulse_tstates = 0;
  libspectrum_dword balance_tstates = 0;
  int flags = 0;

  libspectrum_tape_block *raw_block = 
    libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );

  libspectrum_tape_block_set_bit_length( raw_block, scale );
  libspectrum_set_pause_ms( raw_block, 0 );

  rle_state.bits_used = 0;
  rle_state.level = 0;
  rle_state.length = 0;
  rle_state.tape_length = 8192;
  rle_state.tape_buffer = libspectrum_new( libspectrum_byte, rle_state.tape_length );

  *rle_state.tape_buffer = 0;

  it.current_block = iterator;
  error = libspectrum_tape_block_init( block, &it );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_free( rle_state.tape_buffer );
    libspectrum_tape_block_free( raw_block );
    return error;
  }

  while( !(flags & LIBSPECTRUM_TAPE_FLAGS_BLOCK) ) {
    libspectrum_dword pulse_length = 0;

    /* Use internal version of this that doesn't bugger up the
       external tape status */
    error = libspectrum_tape_get_next_edge_internal( &pulse_tstates, &flags,
                                                     tape, &it );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_free( rle_state.tape_buffer );
      libspectrum_tape_block_free( raw_block );
      return error;
    }

    balance_tstates += pulse_tstates;

    /* next set of pulses is: balance_tstates / scale; */
    pulse_length = balance_tstates / scale;
    balance_tstates = balance_tstates % scale;

    /* write pulse_length bits of the current level into the buffer */
    write_pulse( pulse_length );
  }

  if( rle_state.length || rle_state.bits_used ) {
    if( rle_state.bits_used ) {
      rle_state.length++;
    } else {
      rle_state.bits_used = 8;
    }

    error = libspectrum_tape_block_set_bits_in_last_byte( raw_block,
                                                          rle_state.bits_used );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_free( rle_state.tape_buffer );
      libspectrum_tape_block_free( raw_block );
      return error;
    }

    error = libspectrum_tape_block_set_data_length( raw_block,
                                                    rle_state.length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_free( rle_state.tape_buffer );
      libspectrum_tape_block_free( raw_block );
      return error;
    }

    error = libspectrum_tape_block_set_data( raw_block, rle_state.tape_buffer );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_free( rle_state.tape_buffer );
      libspectrum_tape_block_free( raw_block );
      return error;
    }

    /* now have tzx_write_raw_data finish the job */
    tzx_write_raw_data( raw_block, buffer );
  } else {
    libspectrum_free( rle_state.tape_buffer );
  }

  return libspectrum_tape_block_free( raw_block );
}

static void
add_pulses_block( size_t pulse_count, libspectrum_dword *lengths,
                  libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_tape_block *pulses = 
              libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PULSES );

  libspectrum_tape_block_set_count( pulses, pulse_count );
  libspectrum_tape_block_set_pulse_lengths( pulses, lengths );

  tzx_write_pulses( pulses, buffer );

  libspectrum_tape_block_free( pulses );
}

/* Use ID 2B to set initial signal level, ID 12 Pure Tone for repeating pulses
   and ID 13 Pulse Sequence for non-repeating */
static void
tzx_write_pulse_sequence( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  size_t count = libspectrum_tape_block_count( block );
  size_t i;
  size_t uncommitted_pulse_count = 0;
  size_t max_pulse_count = 0;
  libspectrum_dword *lengths = NULL;

  add_set_signal_level_block( buffer, 0 );

  for( i = 0; i<count; i++ ) {
    size_t pulse_repeats = libspectrum_tape_block_pulse_repeats( block, i );
    if( pulse_repeats > 1 ) {
      /* Close off any outstanding pulse blocks */
      if( uncommitted_pulse_count > 0 ) {
        add_pulses_block( uncommitted_pulse_count, lengths, block, buffer );
        uncommitted_pulse_count = 0;
        max_pulse_count = 0;
        lengths = NULL;
      }

      add_pure_tone_block( buffer,
                           libspectrum_tape_block_pulse_lengths( block, i ),
                           pulse_repeats );
    } else {
      if( uncommitted_pulse_count == max_pulse_count ) {
        max_pulse_count = uncommitted_pulse_count + 64;
        lengths =
          libspectrum_renew( libspectrum_dword, lengths, max_pulse_count );
      }
      /* Queue up pulse */
      lengths[uncommitted_pulse_count++] =
        libspectrum_tape_block_pulse_lengths( block, i );
    }
  }

  /* Close off any outstanding pulse blocks */
  if( uncommitted_pulse_count > 0 ) {
    add_pulses_block( uncommitted_pulse_count, lengths, block, buffer );
  }
}

/* Use ID 2B to set initial signal level, ID 14 Pure Data Block and a ID 12
   Pure Tone for the tail pulse */
static libspectrum_error
tzx_write_data_block( libspectrum_tape_block *block, libspectrum_buffer *buffer )
{
  libspectrum_error error;
  libspectrum_tape_block *pure_data;
  size_t data_length, i;
  libspectrum_byte *data;
  libspectrum_tape_generalised_data_symbol_table pilot_table, data_table;

  tzx_write_set_signal_level( block, buffer );

  /* Pure data block can only have two identical pulses for bit 0 and bit 1 */
  if( libspectrum_tape_block_bit0_pulse_count( block ) != 2 ||
      ( libspectrum_tape_block_bit0_pulses( block, 0 ) !=
        libspectrum_tape_block_bit0_pulses( block, 1 ) ) ||
      libspectrum_tape_block_bit1_pulse_count( block ) != 2 ||
      ( libspectrum_tape_block_bit1_pulses( block, 0 ) !=
        libspectrum_tape_block_bit1_pulses( block, 1 ) ) ) {
    pilot_table.symbols_in_block = 0;
    pilot_table.max_pulses = 0;
    pilot_table.symbols_in_table = 0;

    data_table.symbols_in_block = libspectrum_tape_block_count( block );
    data_table.max_pulses = MAX( libspectrum_tape_block_bit0_pulse_count( block ),
                                 libspectrum_tape_block_bit1_pulse_count( block ) );
    data_table.symbols_in_table = 2;

    data_table.symbols =
      libspectrum_new( libspectrum_tape_generalised_data_symbol,
                       data_table.symbols_in_table );

    data_table.symbols[0].edge_type =
      LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_EDGE;
    data_table.symbols[0].lengths =
      libspectrum_new( libspectrum_word,
                       libspectrum_tape_block_bit0_pulse_count( block ) );
    for( i = 0; i < libspectrum_tape_block_bit0_pulse_count( block ); i++ ) {
      data_table.symbols[0].lengths[i] =
        libspectrum_tape_block_bit0_pulses( block, i );
    }

    data_table.symbols[1].edge_type =
      LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_EDGE;
    data_table.symbols[1].lengths =
      libspectrum_new( libspectrum_word,
                       libspectrum_tape_block_bit1_pulse_count( block ) );
    for( i = 0; i < libspectrum_tape_block_bit1_pulse_count( block ); i++ ) {
      data_table.symbols[1].lengths[i] =
        libspectrum_tape_block_bit1_pulses( block, i );
    }

    error = write_generalised_data_block( block, buffer, 1, &pilot_table,
                                          &data_table, 0 );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  } else {
    pure_data = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PURE_DATA );

    libspectrum_tape_block_set_bit0_length( pure_data,
                            libspectrum_tape_block_bit0_pulses( block, 0 ) );
    libspectrum_tape_block_set_bit1_length( pure_data,
                            libspectrum_tape_block_bit1_pulses( block, 0 ) );
    libspectrum_tape_block_set_bits_in_last_byte( pure_data, 
                            libspectrum_tape_block_bits_in_last_byte( block ) );
    libspectrum_set_pause_tstates( pure_data, 0 );

    /* And the actual data */
    data_length = libspectrum_tape_block_data_length( block );
    libspectrum_tape_block_set_data_length( pure_data, data_length );
    data = libspectrum_new( libspectrum_byte, data_length );
    memcpy( data, libspectrum_tape_block_data( block ), data_length );
    libspectrum_tape_block_set_data( pure_data, data );

    tzx_write_data( pure_data, buffer );

    libspectrum_tape_block_free( pure_data );
  }

  if( libspectrum_tape_block_tail_length( block ) ) {
    add_pure_tone_block( buffer,
                         libspectrum_tape_block_tail_length( block ), 1 );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
tzx_write_empty_block( libspectrum_buffer *buffer, libspectrum_tape_type id )
{
  libspectrum_buffer_write_byte( buffer, id );
}

static void
tzx_write_bytes( libspectrum_buffer* buffer, size_t length, size_t length_bytes,
                 libspectrum_byte *data )
{
  size_t i, shifter;

  /* Write out the appropriate number of length bytes */
  for( i=0, shifter = length; i<length_bytes; i++, shifter >>= 8 )
    libspectrum_buffer_write_byte( buffer, shifter & 0xff );

  /* And then the actual data */
  libspectrum_buffer_write( buffer, data, length );
}

static void
tzx_write_string( libspectrum_buffer *buffer, char *string )
{
  size_t i, length = strlen( (char*)string );

  length &= 0xff;
  libspectrum_buffer_write_byte( buffer, length );

  /* Fix up line endings as we go */
  for( i=0; i<length; i++ )
    libspectrum_buffer_write_byte( buffer, string[i] == '\x0a' ? '\x0d' : string[i] );
}
