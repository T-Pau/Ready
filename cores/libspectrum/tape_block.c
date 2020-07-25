/* tape_block.c: one tape block
   Copyright (c) 2003-2016 Philip Kendall

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

#include "tape_block.h"

/* The number of pilot pulses for the standard ROM loader NB: These
   disagree with the .tzx specification (they're 5 and 1 less
   respectively), but are correct. Entering the loop at #04D8 in the
   48K ROM with HL == #0001 will produce the first sync pulse, not a
   pilot pulse, which gives a difference of one in both cases. The
   further difference of 4 in the header count is just a screw-up in
   the .tzx specification AFAICT */
static const size_t LIBSPECTRUM_TAPE_PILOTS_HEADER = 0x1f7f;
static const size_t LIBSPECTRUM_TAPE_PILOTS_DATA   = 0x0c97;

/* Functions to initialise block types */

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state );
static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state );
static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state );
static void
raw_data_init( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state );
static libspectrum_error
generalised_data_init( libspectrum_tape_generalised_data_block *block,
                       libspectrum_tape_generalised_data_block_state *state );
static libspectrum_error
data_block_init( libspectrum_tape_data_block *block,
                 libspectrum_tape_data_block_state *state );

libspectrum_tape_block*
libspectrum_tape_block_alloc( libspectrum_tape_type type )
{
  libspectrum_tape_block *block = libspectrum_new( libspectrum_tape_block, 1 );
  libspectrum_tape_block_set_type( block, type );
  return block;
}

static void
free_symbol_table( libspectrum_tape_generalised_data_symbol_table *table )
{
  size_t i;

  if( table->symbols ) {
    for( i = 0; i < table->symbols_in_table; i++ )
      libspectrum_free( table->symbols[ i ].lengths );

    libspectrum_free( table->symbols );
  }
}

/* Free the memory used by one block */
libspectrum_error
libspectrum_tape_block_free( libspectrum_tape_block *block )
{
  size_t i;

  switch( block->type ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    libspectrum_free( block->types.rom.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    libspectrum_free( block->types.turbo.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    libspectrum_free( block->types.pulses.lengths );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    libspectrum_free( block->types.pure_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    libspectrum_free( block->types.raw_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    free_symbol_table( &block->types.generalised_data.pilot_table );
    free_symbol_table( &block->types.generalised_data.data_table );
    libspectrum_free( block->types.generalised_data.pilot_symbols );
    libspectrum_free( block->types.generalised_data.pilot_repeats );
    libspectrum_free( block->types.generalised_data.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    libspectrum_free( block->types.group_start.name );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    for( i=0; i<block->types.select.count; i++ ) {
      libspectrum_free( block->types.select.descriptions[i] );
    }
    libspectrum_free( block->types.select.descriptions );
    libspectrum_free( block->types.select.offsets );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    libspectrum_free( block->types.comment.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    libspectrum_free( block->types.message.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    for( i=0; i<block->types.archive_info.count; i++ ) {
      libspectrum_free( block->types.archive_info.strings[i] );
    }
    libspectrum_free( block->types.archive_info.ids );
    libspectrum_free( block->types.archive_info.strings );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    libspectrum_free( block->types.hardware.types  );
    libspectrum_free( block->types.hardware.ids    );
    libspectrum_free( block->types.hardware.values );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    libspectrum_free( block->types.custom.description );
    libspectrum_free( block->types.custom.data );
    break;

  /* Block types not present in .tzx follow here */

  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    libspectrum_free( block->types.rle_pulse.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
    libspectrum_free( block->types.pulse_sequence.lengths );
    libspectrum_free( block->types.pulse_sequence.pulse_repeats );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    libspectrum_free( block->types.data_block.data );
    libspectrum_free( block->types.data_block.bit0_pulses );
    libspectrum_free( block->types.data_block.bit1_pulses );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CONCAT: /* This should never occur */
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "%s: unknown block type %d", __func__,
			     block->type );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  libspectrum_free( block );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_tape_type
libspectrum_tape_block_type( libspectrum_tape_block *block )
{
  return block->type;
}

libspectrum_error
libspectrum_tape_block_set_type( libspectrum_tape_block *block,
				 libspectrum_tape_type type )
{
  block->type = type;
  return LIBSPECTRUM_ERROR_NONE;
}

/* Called when a new block is started to initialise its internal state */
libspectrum_error
libspectrum_tape_block_init( libspectrum_tape_block *block,
                             libspectrum_tape_block_state *state )
{
  if( !block ) return LIBSPECTRUM_ERROR_NONE;

  switch( libspectrum_tape_block_type( block ) ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_init( &(block->types.rom), &(state->block_state.rom) );
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    return turbo_init( &(block->types.turbo), &(state->block_state.turbo) );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    state->block_state.pure_tone.edge_count = block->types.pure_tone.pulses;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    state->block_state.pulses.edge_count = 0;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    return pure_data_init( &(block->types.pure_data),
                           &(state->block_state.pure_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    raw_data_init( &(block->types.raw_data), &(state->block_state.raw_data) );
    return LIBSPECTRUM_ERROR_NONE;
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    return generalised_data_init( &(block->types.generalised_data),
                                  &(state->block_state.generalised_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    state->block_state.rle_pulse.index = 0;
    return LIBSPECTRUM_ERROR_NONE;
  case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
    state->block_state.pulse_sequence.index = 0;
    state->block_state.pulse_sequence.pulse_count = 0;
    state->block_state.pulse_sequence.level = -1;
    return LIBSPECTRUM_ERROR_NONE;
  case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    return data_block_init( &(block->types.data_block),
                            &(state->block_state.data_block) );

  /* These blocks need no initialisation */
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    return LIBSPECTRUM_ERROR_NONE;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_init_block: unknown block type 0x%02x",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state )
{
  /* Initialise the number of pilot pulses */
  state->edge_count = block->length && block->data[0] & 0x80 ?
                      LIBSPECTRUM_TAPE_PILOTS_DATA           :
                      LIBSPECTRUM_TAPE_PILOTS_HEADER;

  /* And that we're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  state->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state )
{
  /* Initialise the number of pilot pulses */
  state->edge_count = block->pilot_pulses;

  /* And that we're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  state->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  /* Set up the next bit */
  error = libspectrum_tape_pure_data_next_bit( block, state );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static void
raw_data_init( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state )
{
  if( block->data ) {

    /* We're just before the start of the data */
    state->state = LIBSPECTRUM_TAPE_STATE_DATA1;
    state->bytes_through_block = -1; state->bits_through_byte = 7;
    state->last_bit = 0x80 & block->data[0];
    /* Set up the next bit */
    libspectrum_tape_raw_data_next_bit( block, state );

  } else {

    state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;

  }
}

static libspectrum_error
generalised_data_init( libspectrum_tape_generalised_data_block *block,
                       libspectrum_tape_generalised_data_block_state *state )
{
  state->run = 0;
  state->symbols_through_run = 0;
  state->edges_through_symbol = 0;

  state->current_symbol = 0;
  state->symbols_through_stream = 0;

  state->current_byte = 0;
  state->bits_through_byte = 0;
  state->bytes_through_stream = 0;

  if( block->pilot_table.symbols_in_block ) {
    state->state = LIBSPECTRUM_TAPE_STATE_PILOT;
  } else if( block->data_table.symbols_in_block ) {
    state->state = LIBSPECTRUM_TAPE_STATE_DATA1;
    state->current_byte = block->data[ 0 ];
    state->current_symbol = get_generalised_data_symbol( block, state );
  } else {
    state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
data_block_init( libspectrum_tape_data_block *block,
                 libspectrum_tape_data_block_state *state )
{
  libspectrum_error error;

  state->bit0_flags = 0;
  state->bit1_flags = 0;

  /* If both bit0 and bit1 are two matching pulses, then assign them to be
     flagged as short/long pulses respectively */
  if( block->bit0_pulse_count == 2 && block->bit1_pulse_count == 2 &&
      block->bit0_pulses[ 0 ] == block->bit0_pulses[ 1 ] &&
      block->bit1_pulses[ 0 ] == block->bit1_pulses[ 1 ] &&
      block->bit0_pulses[ 0 ] > 0  && block->bit1_pulses[ 0 ] > 0 ) {
    if( block->bit0_pulses[ 0 ] < block->bit1_pulses[ 0 ] ) {
      state->bit0_flags = LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT;
      state->bit1_flags = LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    } else if( block->bit0_pulses[ 0 ] > block->bit1_pulses[ 0 ] ) {
      state->bit0_flags = LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
      state->bit1_flags = LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT;
    }
  }

  if( block->initial_level != -1 ) {
    state->level = block->initial_level;
  }

  /* We're just before the start of the data */
  state->bytes_through_block = -1; state->bits_through_byte = 7;
  /* Set up the next bit */
  error = libspectrum_tape_data_block_next_bit( block, state );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Does this block consist solely of metadata? */
int
libspectrum_tape_block_metadata( libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
  case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
  case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    return 0;

  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
  case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
    return 1;
  }

  /* Should never happen */
  return -1;
}

libspectrum_error
libspectrum_tape_block_read_symbol_table_parameters(
  libspectrum_tape_block *block, int pilot, const libspectrum_byte **ptr )
{
  libspectrum_tape_generalised_data_block *generalised =
    &block->types.generalised_data;

  libspectrum_tape_generalised_data_symbol_table *table =
    pilot ? &generalised->pilot_table : &generalised->data_table;

  table->symbols_in_block = libspectrum_read_dword( ptr );
  table->max_pulses = (*ptr)[0];

  table->symbols_in_table = (*ptr)[1];
  if( !table->symbols_in_table && table->symbols_in_block ) 
    table->symbols_in_table = 256;

  (*ptr) += 2;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_block_read_symbol_table(
  libspectrum_tape_generalised_data_symbol_table *table,
  const libspectrum_byte **ptr, size_t length )
{
  if( table->symbols_in_block ) {
    
    libspectrum_tape_generalised_data_symbol *symbol;
    size_t i, j;

    /* Sanity check */
    if( length < ( 2 * table->max_pulses + 1 ) * table->symbols_in_table ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "%s: not enough data in buffer", __func__ );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }


    table->symbols =
      libspectrum_new( libspectrum_tape_generalised_data_symbol,
                       table->symbols_in_table );

    for( i = 0, symbol = table->symbols;
	 i < table->symbols_in_table;
	 i++, symbol++ ) {
      symbol->edge_type = **ptr; (*ptr)++;
      symbol->lengths = libspectrum_new( libspectrum_word, table->max_pulses );
      for( j = 0; j < table->max_pulses; j++ ) {
	symbol->lengths[ j ] = (*ptr)[0] + (*ptr)[1] * 0x100;
	(*ptr) += 2;
      }
    }

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_tape_block_zero( libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    block->types.generalised_data.pilot_table.symbols = NULL;
    block->types.generalised_data.data_table.symbols = NULL;
    block->types.generalised_data.pilot_symbols = NULL;
    block->types.generalised_data.pilot_repeats = NULL;
    block->types.generalised_data.data = NULL;
    break;
  default:
    break;
  }
}

/* Give the length of a tape block */

#define BITS_SET_ARRAY_SIZE 256
static int bits_set[ BITS_SET_ARRAY_SIZE ];

const int LIBSPECTRUM_BITS_IN_BYTE = 8;

static int
libspectrum_bits_set_n_bits( libspectrum_byte byte, libspectrum_byte bits )
{
  int i;
  int retval = 0;

  if( bits > LIBSPECTRUM_BITS_IN_BYTE) bits = LIBSPECTRUM_BITS_IN_BYTE;

  for( i = 0; i < bits; i++ ) {
    if( byte & 0x80 ) retval++;
    byte <<= 1;
  }

  return retval;
}

void
libspectrum_init_bits_set( void )
{
  int i;

  /* Not big or clever, but easy to understand */
  for( i = 0; i < BITS_SET_ARRAY_SIZE; i++ ) {
    bits_set[ i ] = libspectrum_bits_set_n_bits( (libspectrum_byte)i,
                                                 LIBSPECTRUM_BITS_IN_BYTE );
  }
}

static libspectrum_dword
convert_pulses_to_tstates( libspectrum_dword set_bit_length,
                           libspectrum_dword unset_bit_length,
                           int bits_set_in_block, int bit_block_size )
{
  static const int pulses_in_bit = 2;
  return ( pulses_in_bit * set_bit_length ) * bits_set_in_block +
         ( pulses_in_bit * unset_bit_length ) * ( bit_block_size - bits_set_in_block );
}

static libspectrum_dword
rom_block_length( libspectrum_tape_rom_block *rom )
{
  libspectrum_dword length = rom->pause_tstates;
  size_t i;

  size_t edge_count = rom->length && rom->data[0] & 0x80 ?
                      LIBSPECTRUM_TAPE_PILOTS_DATA           :
                      LIBSPECTRUM_TAPE_PILOTS_HEADER;
  length += LIBSPECTRUM_TAPE_TIMING_PILOT * edge_count;
  length += LIBSPECTRUM_TAPE_TIMING_SYNC1;
  length += LIBSPECTRUM_TAPE_TIMING_SYNC2;

  for( i = 0; i < rom->length; i++ ) {
    libspectrum_byte data = rom->data[ i ];
    length += convert_pulses_to_tstates( LIBSPECTRUM_TAPE_TIMING_DATA1,
                                         LIBSPECTRUM_TAPE_TIMING_DATA0,
                                         bits_set[ data ],
                                         LIBSPECTRUM_BITS_IN_BYTE );
  }

  return length;
}

static libspectrum_dword
turbo_block_length( libspectrum_tape_turbo_block *turbo )
{
  libspectrum_dword length =
    turbo->pilot_pulses * turbo->pilot_length +
    turbo->sync1_length + turbo->sync2_length +
    turbo->pause_tstates;
  size_t i;
  if( turbo->length ) {
    int bits_set_in_last_byte =
      libspectrum_bits_set_n_bits( turbo->data[ turbo->length-1 ],
                                   turbo->bits_in_last_byte );

    for( i = 0; i < turbo->length-1; i++ ) {
      libspectrum_byte data = turbo->data[ i ];
      length += convert_pulses_to_tstates( turbo->bit1_length,
                                           turbo->bit0_length,
                                           bits_set[ data ],
                                           LIBSPECTRUM_BITS_IN_BYTE );
    }

    /* handle bits in last byte correctly */
    length += convert_pulses_to_tstates( turbo->bit1_length,
                                         turbo->bit0_length,
                                         bits_set_in_last_byte,
                                         turbo->bits_in_last_byte );
  }

  return length;
}

static libspectrum_dword
pulses_block_length( libspectrum_tape_pulses_block *pulses )
{
  libspectrum_dword length = 0;
  size_t i;

  for( i = 0; i < pulses->count; i++ ) length += pulses->lengths[ i ];

  return length;
}

static libspectrum_dword
pure_data_block_length( libspectrum_tape_pure_data_block *pure_data )
{
  libspectrum_dword length = pure_data->pause_tstates;
  size_t i;
  if( pure_data->length ) {
    int bits_set_in_last_byte =
      libspectrum_bits_set_n_bits( pure_data->data[ pure_data->length-1 ],
                                   pure_data->bits_in_last_byte );

    for( i = 0; i < pure_data->length-1; i++ ) {
      libspectrum_byte data = pure_data->data[ i ];
      length += convert_pulses_to_tstates( pure_data->bit1_length,
                                           pure_data->bit0_length,
                                           bits_set[ data ],
                                           LIBSPECTRUM_BITS_IN_BYTE );
    }
      
    /* handle bits in last byte correctly */
    length += convert_pulses_to_tstates( pure_data->bit1_length,
                                         pure_data->bit0_length,
                                         bits_set_in_last_byte,
                                         pure_data->bits_in_last_byte );
  }

  return length;
}

static libspectrum_dword
raw_data_block_length( libspectrum_tape_raw_data_block *raw_data )
{
  libspectrum_dword length = raw_data->pause_tstates;

  length += ( LIBSPECTRUM_BITS_IN_BYTE * raw_data->length -
              ( LIBSPECTRUM_BITS_IN_BYTE - raw_data->bits_in_last_byte ) ) *
              raw_data->bit_length;

  return length;
}

static libspectrum_dword
rle_pulse_block_length( libspectrum_tape_rle_pulse_block *rle_pulse )
{
  libspectrum_dword length = 0;
  size_t i;

  for( i = 0; i < rle_pulse->length; i++ ) {
    length += rle_pulse->data[ i ] * rle_pulse->scale;
  }

  return length;
}

static libspectrum_dword
generalised_data_block_length(
                    libspectrum_tape_generalised_data_block *generalised_data )
{
  libspectrum_dword length = 0;
  libspectrum_tape_generalised_data_block_state state;
  libspectrum_dword tstates = 0;
  /* Assume no special flags by default */
  int flags = 0;
  /* Has this edge ended the block? */
  int end_of_block = 0;
  libspectrum_error error = generalised_data_init( generalised_data, &state );

  if( error ) return -1;

  /* just reuse tape iteration for this as it is so nasty */
  while( !end_of_block ) {
    error = generalised_data_edge( generalised_data, &state, &tstates,
                                   &end_of_block, &flags );
    if( error ) return -1;

    length += tstates;
  }

  return length;
}

static libspectrum_dword
pulse_sequence_block_length(
                    libspectrum_tape_pulse_sequence_block *pulses )
{
  libspectrum_dword length = 0;
  size_t i;

  for( i = 0; i < pulses->count; i++ )
    length += pulses->lengths[ i ] * pulses->pulse_repeats[ i ];

  return length;
}

static libspectrum_dword
data_block_length( libspectrum_tape_data_block *data_block )
{
  libspectrum_dword length = 0;
  size_t i;
  if( data_block->count ) {
    int bits_set_in_last_byte =
      libspectrum_bits_set_n_bits( data_block->data[ data_block->length-1 ],
                                   data_block->bits_in_last_byte );
    size_t bit0_length = 0;
    size_t bit1_length = 0;

    for( i = 0; i < data_block->bit0_pulse_count; i++ ) {
      bit0_length += data_block->bit0_pulses[ i ];
    }
    bit0_length /=
      data_block->bit0_pulse_count ? data_block->bit0_pulse_count : 1;

    for( i = 0; i < data_block->bit1_pulse_count; i++ ) {
      bit1_length += data_block->bit1_pulses[ i ];
    }
    bit1_length /=
      data_block->bit1_pulse_count ? data_block->bit1_pulse_count : 1;

    for( i = 0; i < data_block->length-1; i++ ) {
      libspectrum_byte data = data_block->data[ i ];
      length += convert_pulses_to_tstates( bit1_length,
                                           bit0_length,
                                           bits_set[ data ],
                                           LIBSPECTRUM_BITS_IN_BYTE );
    }
      
    /* handle bits in last byte correctly */
    length += convert_pulses_to_tstates( bit1_length,
                                         bit0_length,
                                         bits_set_in_last_byte,
                                         data_block->bits_in_last_byte );
  }

  return length;
}

libspectrum_dword
libspectrum_tape_block_length( libspectrum_tape_block *block )
{
  libspectrum_tape_pure_tone_block *pure_tone;

  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_block_length( &block->types.rom );
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    return turbo_block_length( &block->types.turbo );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    pure_tone = &block->types.pure_tone;
    return pure_tone->pulses * pure_tone->length;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    return pulses_block_length( &block->types.pulses );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    return pure_data_block_length( &block->types.pure_data );
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    return block->types.pause.length_tstates;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    return raw_data_block_length( &block->types.raw_data );
  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    return rle_pulse_block_length( &block->types.rle_pulse );
  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    return generalised_data_block_length( &block->types.generalised_data );
  case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
    return pulse_sequence_block_length( &block->types.pulse_sequence );
  case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    return data_block_length( &block->types.data_block );
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
  case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    return 0;
  default:
    return -1;
  }
}
