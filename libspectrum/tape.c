/* tape.c: Routines for handling tape files
   Copyright (c) 2001-2018 Philip Kendall, Darren Salt, Fredrick Meunier

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

#include "internals.h"
#include "tape_block.h"

/* The tape type itself */
struct libspectrum_tape {

  /* All the blocks */
  GSList* blocks;

  /* The last block */
  GSList* last_block;

  /* The state of the current block */
  libspectrum_tape_block_state state;

};

/*** Constants ***/

/* The timings for the standard ROM loader */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_PILOT = 2168; /*Pilot*/
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC1 =  667; /*Sync 1*/
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC2 =  735; /*Sync 2*/
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA0 =  855; /*Reset*/
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA1 = 1710; /*Set*/
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_TAIL  =  945; /*Tail*/

/*** Local function prototypes ***/

/* Free the memory used by a block */
static void
block_free( gpointer data, gpointer user_data );

/* Functions to get the next edge */

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state,
          libspectrum_dword *tstates, int *end_of_block, int *flags );
static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block,
              libspectrum_tape_rom_block_state *state );

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state,
            libspectrum_dword *tstates,
	    int *end_of_block, int *flags );
static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block,
                libspectrum_tape_turbo_block_state *state );

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block,
           libspectrum_tape_pure_tone_block_state *state,
           libspectrum_dword *tstates, int *end_of_block );

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block,
             libspectrum_tape_pulses_block_state *state,
             libspectrum_dword *tstates, int *end_of_block );

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state,
		libspectrum_dword *tstates, int *end_of_block, int *flags );

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state,
	       libspectrum_dword *tstates, int *end_of_block,
               int *flags );

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset );

static libspectrum_error
rle_pulse_edge( libspectrum_tape_rle_pulse_block *block,
                libspectrum_tape_rle_pulse_block_state *state,
		libspectrum_dword *tstates, int *end_of_block );

static libspectrum_error
pulse_sequence_edge( libspectrum_tape_pulse_sequence_block *block,
                     libspectrum_tape_pulse_sequence_block_state *state,
                     libspectrum_dword *tstates, int *end_of_block,
                     int *flags );

libspectrum_error
libspectrum_tape_data_block_next_bit( libspectrum_tape_data_block *block,
                                    libspectrum_tape_data_block_state *state );

static libspectrum_error
data_block_edge( libspectrum_tape_data_block *block,
                 libspectrum_tape_data_block_state *state,
                 libspectrum_dword *tstates, int *end_of_block, int *flags );

/*** Function definitions ****/

/* Allocate a list of blocks */
libspectrum_tape*
libspectrum_tape_alloc( void )
{
  libspectrum_tape *tape = libspectrum_new( libspectrum_tape, 1 );
  tape->blocks = NULL;
  tape->last_block = NULL;
  libspectrum_tape_iterator_init( &(tape->state.current_block), tape );
  tape->state.loop_block = NULL;
  return tape;
}

/* Free the memory used by a list of blocks, but not the object itself */
libspectrum_error
libspectrum_tape_clear( libspectrum_tape *tape )
{
  g_slist_foreach( tape->blocks, block_free, NULL );
  g_slist_free( tape->blocks );
  tape->blocks = NULL;
  libspectrum_tape_iterator_init( &(tape->state.current_block), tape );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free up a list of blocks */
libspectrum_error
libspectrum_tape_free( libspectrum_tape *tape )
{
  libspectrum_error error;

  error = libspectrum_tape_clear( tape );
  if( error ) return error;

  libspectrum_free( tape );
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Free the memory used by one block */
static void
block_free( gpointer data, gpointer user_data GCC_UNUSED )
{
  libspectrum_tape_block_free( data );
}

/* Read in a tape file, optionally guessing what sort of file it is */
libspectrum_error
libspectrum_tape_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		       size_t length, libspectrum_id_t type,
		       const char *filename )
{
  libspectrum_id_t raw_type;
  libspectrum_class_t class;
  libspectrum_byte *new_buffer;
  libspectrum_error error;

  /* If we don't know what sort of file this is, make a best guess */
  if( type == LIBSPECTRUM_ID_UNKNOWN ) {
    error = libspectrum_identify_file( &type, filename, buffer, length );
    if( error ) return error;

    /* If we still can't identify it, give up */
    if( type == LIBSPECTRUM_ID_UNKNOWN ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"libspectrum_tape_read: couldn't identify file"
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  /* Find out if this file needs decompression */
  new_buffer = NULL;

  error = libspectrum_identify_file_raw( &raw_type, filename, buffer, length );
  if( error ) return error;

  error = libspectrum_identify_class( &class, raw_type );
  if( error ) return error;

  if( class == LIBSPECTRUM_CLASS_COMPRESSED ) {

    size_t new_length;

    error = libspectrum_uncompress_file( &new_buffer, &new_length, NULL,
					 raw_type, buffer, length, NULL );
    if( error ) return error;
    buffer = new_buffer; length = new_length;
  }

  switch( type ) {

  case LIBSPECTRUM_ID_TAPE_TAP:
  case LIBSPECTRUM_ID_TAPE_SPC:
  case LIBSPECTRUM_ID_TAPE_STA:
  case LIBSPECTRUM_ID_TAPE_LTP:
    error = internal_tap_read( tape, buffer, length, type ); break;

  case LIBSPECTRUM_ID_TAPE_TZX:
    error = internal_tzx_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_WARAJEVO:
    error = internal_warajevo_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_Z80EM:
    error = libspectrum_z80em_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_CSW:
    error = libspectrum_csw_read( tape, buffer, length ); break;

  case LIBSPECTRUM_ID_TAPE_WAV:
#ifdef HAVE_LIB_AUDIOFILE
    error = libspectrum_wav_read( tape, filename ); break;
#else     /* #ifdef HAVE_LIB_AUDIOFILE */
    error = LIBSPECTRUM_ERROR_LOGIC;
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_read: format not supported without libaudiofile"
    );
    break;
#endif    /* #ifdef HAVE_LIB_AUDIOFILE */

  case LIBSPECTRUM_ID_TAPE_PZX:
    error = internal_pzx_read( tape, buffer, length ); break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "libspectrum_tape_read: not a tape file" );
    libspectrum_free( new_buffer );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_free( new_buffer );
  return error;
}

libspectrum_error
libspectrum_tape_write( libspectrum_byte **buffer, size_t *length,
			libspectrum_tape *tape, libspectrum_id_t type )
{
  libspectrum_byte *ptr = *buffer;
  libspectrum_buffer *new_buffer;
  libspectrum_class_t class;
  libspectrum_error error;

  error = libspectrum_identify_class( &class, type );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_TAPE ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "libspectrum_tape_write: not a tape format" );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  /* Allow for uninitialised buffer on entry */
  if( !*length ) *buffer = NULL;

  new_buffer = libspectrum_buffer_alloc();

  switch( type ) {

  case LIBSPECTRUM_ID_TAPE_TAP:
  case LIBSPECTRUM_ID_TAPE_SPC:
  case LIBSPECTRUM_ID_TAPE_STA:
  case LIBSPECTRUM_ID_TAPE_LTP:
    error = internal_tap_write( new_buffer, tape, type );
    break;

  case LIBSPECTRUM_ID_TAPE_TZX:
    error = internal_tzx_write( new_buffer, tape );
    break;

  case LIBSPECTRUM_ID_TAPE_CSW:
    error = libspectrum_csw_write( new_buffer, tape );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_tape_write: format not supported" );
    error = LIBSPECTRUM_ERROR_UNKNOWN;
    break;

  }

  libspectrum_buffer_append( buffer, length, &ptr, new_buffer );
  libspectrum_buffer_free( new_buffer );

  return error;
}

/* Does this tape structure actually contain a tape? */
int
libspectrum_tape_present( const libspectrum_tape *tape )
{
  return tape->blocks != NULL;
}

/* Some flags which may be set after calling libspectrum_tape_get_next_edge */
const int LIBSPECTRUM_TAPE_FLAGS_BLOCK      = 1 << 0; /* End of block */
const int LIBSPECTRUM_TAPE_FLAGS_STOP       = 1 << 1; /* Stop tape */
const int LIBSPECTRUM_TAPE_FLAGS_STOP48     = 1 << 2; /* Stop tape if in
						     48K mode */
const int LIBSPECTRUM_TAPE_FLAGS_NO_EDGE    = 1 << 3; /* Not an edge really */
const int LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW  = 1 << 4; /* Set level low */
const int LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH = 1 << 5; /* Set level high */
const int LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT = 1 << 6;/* Short edge; used for
                                                          loader acceleration */
const int LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG = 1 << 7; /* Long edge; used for
                                                          loader acceleration */
const int LIBSPECTRUM_TAPE_FLAGS_TAPE       = 1 << 8; /* End of tape */

libspectrum_error
libspectrum_tape_get_next_edge_internal( libspectrum_dword *tstates,
                                         int *flags,
                                         libspectrum_tape *tape,
                                         libspectrum_tape_block_state *it )
{
  int error;

  libspectrum_tape_block *block =
    libspectrum_tape_iterator_current( it->current_block );

  /* Has this edge ended the block? */
  int end_of_block = 0;

  /* After getting a new block, do we want to advance to the next one? */
  int no_advance = 0;

  /* Assume no special flags by default */
  *flags = 0;

  if( block ) {
    switch( block->type ) {
    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = rom_edge( &(block->types.rom), &(it->block_state.rom), tstates,
                        &end_of_block, flags );
      if( error ) return error;
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = turbo_edge( &(block->types.turbo), &(it->block_state.turbo), tstates,
                          &end_of_block, flags );
      if( error ) return error;
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tone_edge( &(block->types.pure_tone), &(it->block_state.pure_tone),
                         tstates, &end_of_block );
      if( error ) return error;
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = pulses_edge( &(block->types.pulses), &(it->block_state.pulses),
                           tstates, &end_of_block );
      if( error ) return error;
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = pure_data_edge( &(block->types.pure_data),
                              &(it->block_state.pure_data), tstates,
			      &end_of_block, flags );
      if( error ) return error;
      break;
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
      error = raw_data_edge( &(block->types.raw_data), &(it->block_state.raw_data),
                             tstates, &end_of_block, flags );
      if( error ) return error;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
      error = generalised_data_edge( &(block->types.generalised_data),
                                     &(it->block_state.generalised_data),
                                     tstates, &end_of_block, flags );
      if( error ) return error;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      *tstates = block->types.pause.length_tstates; end_of_block = 1;
      /* If the pause isn't a "don't care" level then set the appropriate pulse
         level */
      if( block->types.pause.level != -1 &&
          block->types.pause.length_tstates ) {
        *flags |= block->types.pause.level ? LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH :
                                             LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW;
      }
      /* 0 ms pause => stop tape */
      if( *tstates == 0 ) { *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = jump_blocks( tape, block->types.jump.offset );
      if( error ) return error;
      *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE; end_of_block = 1;
      no_advance = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      if( it->current_block->next && block->types.loop_start.count ) {
        it->loop_block = it->current_block->next;
        it->loop_count = block->types.loop_start.count;
      }
      *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE; end_of_block = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      if( it->loop_block ) {
        if( --(it->loop_count) ) {
          it->current_block = it->loop_block;
          no_advance = 1;
        } else {
          it->loop_block = NULL;
        }
      }
      *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE; end_of_block = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP48; end_of_block = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
      *tstates = 0; end_of_block = 1;
      /* Inverted as the following block will flip the level before recording
         the edge */
      *flags |= block->types.set_signal_level.level ?
          LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW : LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH;
      break;

    /* For blocks which contain no Spectrum-readable data, return zero
       tstates and set end of block set so we instantly get the next block */
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START: 
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE; end_of_block = 1;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
      error = rle_pulse_edge( &(block->types.rle_pulse),
                              &(it->block_state.rle_pulse), tstates, &end_of_block);
      if( error ) return error;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
      error = pulse_sequence_edge( &(block->types.pulse_sequence),
                                   &(it->block_state.pulse_sequence), tstates,
                                   &end_of_block, flags );
      if( error ) return error;
      break;

    case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
      error = data_block_edge( &(block->types.data_block),
                               &(it->block_state.data_block), tstates,
                               &end_of_block, flags );
      if( error ) return error;
      break;

    default:
      *tstates = 0;
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
        "libspectrum_tape_get_next_edge: unknown block type 0x%02x",
        block->type
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  } else {
    *tstates = 0;
    end_of_block = 1;
  }

  /* If that ended the block, move onto the next block */
  if( end_of_block ) {

    *flags |= LIBSPECTRUM_TAPE_FLAGS_BLOCK;

    /* Advance to the next block, unless we've been told not to */
    if( !no_advance ) {

      libspectrum_tape_iterator_next( &(it->current_block) );

      /* If we've just hit the end of the tape, stop the tape (and
	 then `rewind' to the start) */
      if( libspectrum_tape_iterator_current( it->current_block ) == NULL ) {
	*flags |= LIBSPECTRUM_TAPE_FLAGS_STOP;
        *flags |= LIBSPECTRUM_TAPE_FLAGS_TAPE;
        /* Need to have an edge at the end of the tape to terminate the last
           pulse so clear the NO_EDGE flag if it has been set */
        *flags &= ~LIBSPECTRUM_TAPE_FLAGS_NO_EDGE;
        libspectrum_tape_iterator_init( &(it->current_block), tape );
      }
    }

    /* Initialise the new block */
    error = libspectrum_tape_block_init(
                      libspectrum_tape_iterator_current( it->current_block ),
                      it );
    if( error ) return error;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* The main function: called with a tape object and returns the number of
   t-states until the next edge, and a marker if this was the last edge
   on the tape */
libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_dword *tstates, int *flags,
	                        libspectrum_tape *tape )
{
  return libspectrum_tape_get_next_edge_internal( tstates, flags, tape,
                                                  &(tape->state) );
}

/* TZX pauses should have no edge if there is no duration, from the spec:
   A 'Pause' block of zero duration is completely ignored, so the 'current pulse
   level' will NOT change in this case. This also applies to 'Data' blocks that
   have some pause duration included in them. */
static void
do_tail_pause( libspectrum_dword *tstates,
               int *end_of_block, int *flags )
{
  *end_of_block = 1;
  if( *tstates == 0 ) {
    /* The tail pause is optional - if there is no tail, there is no edge */
    *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE;
  }
}

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block,
          libspectrum_tape_rom_block_state *state,
          libspectrum_dword *tstates,
	  int *end_of_block, int *flags )
{
  int error;

  switch( state->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* The next edge occurs in one pilot edge timing */
    *tstates = LIBSPECTRUM_TAPE_TIMING_PILOT;
    /* If that was the last pilot edge, change state */
    if( --(state->edge_count) == 0 )
      state->state = LIBSPECTRUM_TAPE_STATE_SYNC1;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC1;
    /* Followed by the second sync pulse */
    state->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC2;
    /* Followed by the first bit of data */
    error = rom_next_bit( block, state ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == LIBSPECTRUM_TAPE_TIMING_DATA0 ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the second edge */
    state->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == LIBSPECTRUM_TAPE_TIMING_DATA0 ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the next bit of data (or the end of data) */
    error = rom_next_bit( block, state ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause_tstates;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "rom_edge: unknown state %d", state->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block,
              libspectrum_tape_rom_block_state *state )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(state->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(state->bytes_through_block) == block->length ) {
      state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    state->current_byte = block->data[ state->bytes_through_block ];
    state->bits_through_byte = 0;
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = state->current_byte & 0x80;
  state->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  state->bit_tstates = ( next_bit ? LIBSPECTRUM_TAPE_TIMING_DATA1
			          : LIBSPECTRUM_TAPE_TIMING_DATA0 );
  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block,
            libspectrum_tape_turbo_block_state *state,
            libspectrum_dword *tstates, int *end_of_block, int *flags )
{
  int error;

  switch( state->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* Check we actually have some edges */
    if( state->edge_count-- != 0 ) {
      *tstates = block->pilot_length;
      break;
    }
    /* Fall through */

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = block->sync1_length;
    /* Followed by the second sync pulse */
    state->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = block->sync2_length;
    /* Followed by the first bit of data */
    error = turbo_next_bit( block, state ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == block->bit0_length ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the second edge */
    state->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == block->bit0_length ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the next bit of data (or the end of data) */
    error = turbo_next_bit( block, state ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause_tstates;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "turbo_edge: unknown state %d", state->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block,
                libspectrum_tape_turbo_block_state *state )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(state->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(state->bytes_through_block) == block->length ) {
      state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    state->current_byte = block->data[ state->bytes_through_block ];

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( state->bytes_through_block == block->length-1 ) {
      state->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      state->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = state->current_byte & 0x80;
  state->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  state->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block,
           libspectrum_tape_pure_tone_block_state *state,
           libspectrum_dword *tstates, int *end_of_block )
{
  /* The next edge occurs in one pilot edge timing */
  *tstates = block->length;
  /* If that was the last edge, the block is finished */
  if( --(state->edge_count) == 0 ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block,
             libspectrum_tape_pulses_block_state *state,
             libspectrum_dword *tstates, int *end_of_block )
{
  /* Get the length of this edge */
  *tstates = block->lengths[ state->edge_count ];
  /* Was that the last edge? */
  if( ++(state->edge_count) == block->count ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
                libspectrum_tape_pure_data_block_state *state,
		libspectrum_dword *tstates, int *end_of_block, int *flags )
{
  int error;

  switch( state->state ) {

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == block->bit0_length ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the second edge */
    state->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = state->bit_tstates;
    *flags |= ( state->bit_tstates == block->bit0_length ) ? LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT : LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG;
    /* Followed by the next bit of data (or the end of data) */
    error = libspectrum_tape_pure_data_next_bit( block, state );
    if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause_tstates;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "pure_data_edge: unknown state %d",
			     state->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_pure_data_next_bit( libspectrum_tape_pure_data_block *block,
                             libspectrum_tape_pure_data_block_state *state )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(state->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(state->bytes_through_block) == block->length ) {
      state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    state->current_byte = block->data[ state->bytes_through_block ];

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( state->bytes_through_block == block->length-1 ) {
      state->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      state->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = state->current_byte & 0x80;
  state->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  state->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
               libspectrum_tape_raw_data_block_state *state,
	       libspectrum_dword *tstates, int *end_of_block,
               int *flags )
{
  switch (state->state) {
  case LIBSPECTRUM_TAPE_STATE_DATA1:
    *tstates = state->bit_tstates;
    libspectrum_tape_raw_data_next_bit( block, state );
    /* Backwards as last bit is the state of the next bit as it has already been
       updated*/
    *flags |= state->last_bit ? LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW :
                                LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause_tstates;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "raw_edge: unknown state %d", state->state );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_tape_raw_data_next_bit( libspectrum_tape_raw_data_block *block,
                                    libspectrum_tape_raw_data_block_state *state )
{
  int length = 0;

  if( state->bytes_through_block == block->length ) {
    state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
    return;
  }

  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  /* Step through the data until we find an edge */
  do {
    length++;
    if( ++(state->bits_through_byte) == 8 ) {
      if( ++(state->bytes_through_block) == block->length - 1 ) {
	state->bits_through_byte = 8 - block->bits_in_last_byte;
      } else {
	state->bits_through_byte = 0;
      }
      if( state->bytes_through_block == block->length )
	break;
    }
  } while( ( block->data[state->bytes_through_block] <<
             state->bits_through_byte & 0x80 ) != state->last_bit) ;

  state->bit_tstates = length * block->bit_length;
  state->last_bit ^= 0x80;
}

static libspectrum_byte
get_generalised_data_bit( libspectrum_tape_generalised_data_block *block,
                      libspectrum_tape_generalised_data_block_state *state )
{
  libspectrum_byte r = state->current_byte & 0x80 ? 1 : 0;
  state->current_byte <<= 1;

  if( ++state->bits_through_byte == 8 ) {
    state->bits_through_byte = 0;
    state->bytes_through_stream++;
    state->current_byte = block->data[ state->bytes_through_stream ];
  }
  
  return r;
}

libspectrum_byte
get_generalised_data_symbol( libspectrum_tape_generalised_data_block *block,
                      libspectrum_tape_generalised_data_block_state *state )
{
  libspectrum_byte symbol;
  size_t i;
    
  for( i = 0, symbol = 0;
       i < block->bits_per_data_symbol;
       i++ ) {
    symbol <<= 1;
    symbol |= get_generalised_data_bit( block, state );
  }
    
  return symbol;
}

static void
set_tstates_and_flags( libspectrum_tape_generalised_data_symbol *symbol,
		       libspectrum_byte edge, libspectrum_dword *tstates,
		       int *flags )
{
  *tstates = symbol->lengths[ edge ];

  if( !edge ) {
    switch( symbol->edge_type ) {
    case LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_EDGE:
      break;
    case LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_NO_EDGE:
      *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE;
      break;
    case LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_LOW:
      *flags |= LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW;
      break;
    case LIBSPECTRUM_TAPE_GENERALISED_DATA_SYMBOL_HIGH:
      *flags |= LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH;
      break;
    }
  }
}

libspectrum_error
generalised_data_edge( libspectrum_tape_generalised_data_block *block,
                       libspectrum_tape_generalised_data_block_state *state,
		       libspectrum_dword *tstates, int *end_of_block,
		       int *flags )
{
  libspectrum_tape_generalised_data_symbol_table *table;
  libspectrum_tape_generalised_data_symbol *symbol;
  size_t current_symbol;

  switch( state->state ) {
  case LIBSPECTRUM_TAPE_STATE_PILOT:
    table = &( block->pilot_table );
    current_symbol = block->pilot_symbols[ state->run ];
    symbol = &( table->symbols[ current_symbol ] );

    set_tstates_and_flags( symbol, state->edges_through_symbol, tstates,
			   flags );

    state->edges_through_symbol++;
    if( state->edges_through_symbol == table->max_pulses    ||
	symbol->lengths[ state->edges_through_symbol ] == 0    ) {
      state->edges_through_symbol = 0;
      if( ++state->symbols_through_run == block->pilot_repeats[ state->run ] ) {
	state->symbols_through_run = 0;
	if( ++state->run == table->symbols_in_block ) {
	  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;
	  state->bits_through_byte = 0;
	  state->bytes_through_stream = 0;
	  state->symbols_through_stream = 0;
	  state->current_byte = block->data[ 0 ];
	  state->current_symbol = get_generalised_data_symbol( block, state );
	}
      }
    }
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    table = &( block->data_table );
    symbol = &( table->symbols[ state->current_symbol ] );

    set_tstates_and_flags( symbol, state->edges_through_symbol, tstates,
			   flags );

    state->edges_through_symbol++;
    if( state->edges_through_symbol == table->max_pulses    ||
	symbol->lengths[ state->edges_through_symbol ] == 0    ) {
      if( ++state->symbols_through_stream == table->symbols_in_block ) {
	state->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      } else {
	state->edges_through_symbol = 0;
	state->current_symbol = get_generalised_data_symbol( block, state );
      }
    }
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause_tstates;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC, "%s: unknown state %d",
			     __func__, state->state );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset )
{
  gint current_position; GSList *new_block;

  current_position =
    g_slist_position( tape->blocks, tape->state.current_block );
  if( current_position == -1 ) return LIBSPECTRUM_ERROR_LOGIC;

  new_block = g_slist_nth( tape->blocks, current_position + offset );
  if( new_block == NULL ) return LIBSPECTRUM_ERROR_CORRUPT;

  tape->state.current_block = new_block;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Extra, non-TZX, blocks which can be handled as if TZX */

static libspectrum_error
rle_pulse_edge( libspectrum_tape_rle_pulse_block *block,
                libspectrum_tape_rle_pulse_block_state *state,
		libspectrum_dword *tstates, int *end_of_block )
{
  if( block->data[state->index] ) {

    *tstates = block->scale * block->data[ state->index++ ];

  } else {

    if( state->index + 5 > block->length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			       "rle_pulse_edge: file is truncated\n" );
      return LIBSPECTRUM_ERROR_LOGIC;
    }

    *tstates = block->scale * ( block->data[ state->index + 1 ]       |
			        block->data[ state->index + 2 ] << 8  |
			        block->data[ state->index + 3 ] << 16 |
			        block->data[ state->index + 4 ] << 24   );
    state->index += 5;

  }

  if( state->index == block->length ) *end_of_block = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pulse_sequence_edge( libspectrum_tape_pulse_sequence_block *block,
                     libspectrum_tape_pulse_sequence_block_state *state,
                     libspectrum_dword *tstates, int *end_of_block, int *flags )
{
  int new_level = state->level;
  /* Get the length of this edge */
  *tstates = 0;
  /* Skip past any 0 blocks until we find a non 0 block or reach the end of the
     block, keeping track of the current mic level */
  while( !( *tstates || *end_of_block ) ) {
    *tstates = block->lengths[ state->index ];
    new_level = !new_level;
    /* Was that the last repeat of this pulse block? */
    if( ++(state->pulse_count) == block->pulse_repeats[ state->index ] ) {
      state->index++;
      /* Was that the last block available? */
      if( state->index >= block->count ) {
        /* Next block */
        (*end_of_block) = 1;
      } else {
        /* Next pulse block */
        state->pulse_count = 0;
      }
    }
  }

  if( new_level != state->level ) {
    *flags |= new_level ? LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH :
                          LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW;
    state->level = new_level;
  } else if( !(*tstates) ) {
    /* If the net effect of this edge was 0 tstates and no level change, it was
       much ado about nothing */
    *flags |= LIBSPECTRUM_TAPE_FLAGS_NO_EDGE;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_data_block_next_bit( libspectrum_tape_data_block *block,
                                      libspectrum_tape_data_block_state *state )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(state->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the tail at the end */
    if( ++(state->bytes_through_block) == block->length ) {
      state->state = LIBSPECTRUM_TAPE_STATE_TAIL;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    state->current_byte = block->data[ state->bytes_through_block ];

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( state->bytes_through_block == block->length-1 ) {
      state->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      state->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = state->current_byte & 0x80;
  state->current_byte <<= 1;

  /* And set state for another data bit */
  state->bit_pulses = ( next_bit ? block->bit1_pulses : block->bit0_pulses );
  state->pulse_count = ( next_bit ? block->bit1_pulse_count :
                                    block->bit0_pulse_count );
  state->bit_flags = ( next_bit ? state->bit1_flags : state->bit0_flags );
  state->index = 0;
  state->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
data_block_edge( libspectrum_tape_data_block *block,
                 libspectrum_tape_data_block_state *state,
		 libspectrum_dword *tstates, int *end_of_block, int *flags )
{
  int error;

  switch( state->state ) {

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The next pulse for a bit of data */
    *tstates = state->bit_pulses[ state->index ];
    *flags |= state->bit_flags;
    if( ++(state->index) == state->pulse_count ) {
      /* Followed by the next bit of data (or the end of data) */
      error = libspectrum_tape_data_block_next_bit( block, state );
      if( error ) return error;
    }
    break;

  case LIBSPECTRUM_TAPE_STATE_TAIL:
    /* The pulse at the end of the block */
    *tstates = block->tail_length;
    do_tail_pause( tstates, end_of_block, flags );
    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "data_block_edge: unknown state %d",
			     state->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  if( !(*flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE )) {
    *flags |= state->level ? LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH :
                             LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW;
    state->level = !state->level;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Get the current block */
libspectrum_tape_block*
libspectrum_tape_current_block( libspectrum_tape *tape )
{
  return libspectrum_tape_iterator_current( tape->state.current_block );
}

/* Peek at the next block on the tape */
libspectrum_tape_block*
libspectrum_tape_peek_next_block( libspectrum_tape *tape )
{
  libspectrum_tape_block *block;

  if( !tape->state.current_block ) return NULL;

  block = libspectrum_tape_iterator_current( tape->state.current_block->next );
  return block ? block : tape->blocks->data;
}

/* Peek at the last block on the tape */
libspectrum_tape_block WIN32_DLL *
libspectrum_tape_peek_last_block( libspectrum_tape *tape )
{
  return tape->last_block ? tape->last_block->data : NULL;
}

/* Cause the next block on the tape to be active, initialise it
   and return it */
libspectrum_tape_block*
libspectrum_tape_select_next_block( libspectrum_tape *tape )
{
  libspectrum_tape_block *block;

  if( !tape->state.current_block ) return NULL;

  block = libspectrum_tape_iterator_next( &(tape->state.current_block) );

  if( !block )
    block = libspectrum_tape_iterator_init( &(tape->state.current_block), tape );

  if( libspectrum_tape_block_init( block, &(tape->state) ) )
    return NULL;

  return block;
}
  
/* Get the position on the tape of the current block */
libspectrum_error
libspectrum_tape_position( int *n, libspectrum_tape *tape )
{
  *n = g_slist_position( tape->blocks, tape->state.current_block );

  if( *n == -1 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_position: current block is not in tape!"
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Select the nth block on the tape */
libspectrum_error
libspectrum_tape_nth_block( libspectrum_tape *tape, int n )
{
  GSList *new_block;
  libspectrum_error error;

  new_block = g_slist_nth( tape->blocks, n );
  if( !new_block ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_tape_nth_block: tape does not have block %d", n
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  tape->state.current_block = new_block;

  error = libspectrum_tape_block_init( tape->state.current_block->data,
                                       &(tape->state) );
  if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_tape_append_block( libspectrum_tape *tape,
			       libspectrum_tape_block *block )
{
  if( tape->blocks == NULL ) {
    tape->blocks = g_slist_append( tape->blocks, (gpointer)block );
    tape->last_block = tape->blocks;
  } else {
    tape->last_block =
      g_slist_append( tape->last_block, (gpointer)block )->next;
  }

  /* If we previously didn't have a tape loaded ( implied by
     tape->current_block == NULL ), set up so that we point to the
     start of the tape */
  if( !tape->state.current_block ) {
    tape->state.current_block = tape->blocks;
    libspectrum_tape_block_init( tape->blocks->data, &(tape->state) );
  }
}

void
libspectrum_tape_remove_block( libspectrum_tape *tape,
			       libspectrum_tape_iterator it )
{
  if( it->data ) libspectrum_tape_block_free( it->data );
  tape->blocks = g_slist_delete_link( tape->blocks, it );
  tape->last_block = g_slist_last( tape->blocks );
}

libspectrum_error
libspectrum_tape_insert_block( libspectrum_tape *tape,
			       libspectrum_tape_block *block,
			       size_t position )
{
  tape->blocks = g_slist_insert( tape->blocks, block, position );
  tape->last_block = g_slist_last( tape->blocks );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_block_description( char *buffer, size_t length,
	                            libspectrum_tape_block *block )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    strncpy( buffer, "Standard Speed Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    strncpy( buffer, "Turbo Speed Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    strncpy( buffer, "Pure Tone", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    strncpy( buffer, "List of Pulses", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    strncpy( buffer, "Pure Data", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    strncpy( buffer, "Raw Data", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_GENERALISED_DATA:
    strncpy( buffer, "Generalised Data", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    strncpy( buffer, "Pause", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    strncpy( buffer, "Group Start", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    strncpy( buffer, "Group End", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    strncpy( buffer, "Jump", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    strncpy( buffer, "Loop Start Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    strncpy( buffer, "Loop End", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    strncpy( buffer, "Select", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    strncpy( buffer, "Stop Tape If In 48K Mode", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SET_SIGNAL_LEVEL:
    strncpy( buffer, "Set Signal Level", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    strncpy( buffer, "Comment", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    strncpy( buffer, "Message", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    strncpy( buffer, "Archive Info", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    strncpy( buffer, "Hardware Information", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    strncpy( buffer, "Custom Info", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE:
    strncpy( buffer, "RLE Pulse", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE:
    strncpy( buffer, "Pulse Sequence", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK:
    strncpy( buffer, "Data Block", length );
    break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "libspectrum_tape_block_description: unknown block type 0x%02x",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  buffer[ length-1 ] = '\0';
  return LIBSPECTRUM_ERROR_NONE;
}

/* Given a tape file, attempt to guess which sort of hardware it should run
   on by looking for a hardware block (0x33).

   Deliberately not mentioned in libspectrum.h(.in) as I'm really not
   sure this function is actually useful.
*/
libspectrum_error
libspectrum_tape_guess_hardware( libspectrum_machine *machine,
				 const libspectrum_tape *tape )
{
  GSList *ptr; int score, current_score; size_t i;

  *machine = LIBSPECTRUM_MACHINE_UNKNOWN; current_score = 0;

  if( !libspectrum_tape_present( tape ) ) return LIBSPECTRUM_ERROR_NONE;

  for( ptr = tape->blocks; ptr; ptr = ptr->next ) {

    libspectrum_tape_block *block = (libspectrum_tape_block*)ptr->data;
    libspectrum_tape_hardware_block *hardware;

    if( block->type != LIBSPECTRUM_TAPE_BLOCK_HARDWARE ) continue;

    hardware = &( block->types.hardware );

    for( i=0; i<hardware->count; i++ ) {

      /* Only interested in which computer types this tape runs on */
      if( hardware->types[i] != 0 ) continue;

      /* Skip if the tape doesn't run on this hardware */
      if( hardware->values[i] == 3 ) continue;

      /* If the tape uses the special hardware, choose that preferentially.
	 If it doesn't (or is unknown), it's a possibility */
      if( hardware->values[i] == 1 ) { score = 2; } else { score = 1; }

      if( score <= current_score ) continue;

      switch( hardware->ids[i] ) {

      case 0: /* 16K Spectrum */
	*machine = LIBSPECTRUM_MACHINE_16; current_score = score;
	break;

      case 1: /* 48K Spectrum */
      case 2: /* 48K Issue 1 Spectrum */
	*machine = LIBSPECTRUM_MACHINE_48; current_score = score;
	break;

      case 3: /* 128K Spectrum */
	*machine = LIBSPECTRUM_MACHINE_128; current_score = score;
	break;

      case 4: /* +2 */
	*machine = LIBSPECTRUM_MACHINE_PLUS2; current_score = score;
	break;

      case 5: /* +2A and +3. Gee, thanks to whoever designed the TZX format
		 for distinguishing those :-( */
	*machine = LIBSPECTRUM_MACHINE_PLUS3; current_score = score;
	break;

      case 6: /* TC2048 */
	*machine = LIBSPECTRUM_MACHINE_TC2048; current_score = score;
	break;

      }
    }
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

/*
 * Tape iterator functions
 */

libspectrum_tape_block*
libspectrum_tape_iterator_init( libspectrum_tape_iterator *iterator,
				libspectrum_tape *tape )
{
  *iterator = tape->blocks;
  return libspectrum_tape_iterator_current( *iterator );
}

libspectrum_tape_block*
libspectrum_tape_block_internal_init(
                                libspectrum_tape_block_state *it,
				libspectrum_tape *tape )
{
  if( !tape || !tape->blocks )
    return NULL;

  it->current_block = tape->blocks;

  if( libspectrum_tape_block_init( it->current_block->data,
                                   it ) )
    return NULL;

  return libspectrum_tape_iterator_current( it->current_block );
}

libspectrum_tape_block*
libspectrum_tape_iterator_current( libspectrum_tape_iterator iterator )
{
  return iterator ? iterator->data : NULL;
}

libspectrum_tape_block*
libspectrum_tape_iterator_next( libspectrum_tape_iterator *iterator )
{
  if( iterator && *iterator ) {
    *iterator = (*iterator)->next;
    return libspectrum_tape_iterator_current( *iterator );
  }
  return NULL;
}

libspectrum_tape_block*
libspectrum_tape_iterator_peek_next( libspectrum_tape_iterator iterator )
{
  if( iterator ) {
    return libspectrum_tape_iterator_current( iterator->next );
  }
  return NULL;
}

libspectrum_tape_state_type
libspectrum_tape_state( libspectrum_tape *tape )
{
  libspectrum_tape_block *block =
    libspectrum_tape_iterator_current( tape->state.current_block );
  switch( block->type ) {

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA: return tape->state.block_state.pure_data.state;
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA: return tape->state.block_state.raw_data.state;
    case LIBSPECTRUM_TAPE_BLOCK_ROM: return tape->state.block_state.rom.state;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO: return tape->state.block_state.turbo.state;

    default:
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_INVALID,
        "invalid current block type 0x%02x in tape given to %s", block->type, __func__
      );
      return LIBSPECTRUM_TAPE_STATE_INVALID;
  }
}

libspectrum_error
libspectrum_tape_set_state( libspectrum_tape *tape, libspectrum_tape_state_type state )
{
  libspectrum_tape_block *block =
    libspectrum_tape_iterator_current( tape->state.current_block );
  switch( block->type ) {

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA: tape->state.block_state.pure_data.state = state; break;
    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA: tape->state.block_state.raw_data.state = state; break;
    case LIBSPECTRUM_TAPE_BLOCK_ROM: tape->state.block_state.rom.state = state; break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO: tape->state.block_state.turbo.state = state; break;

    default:
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_INVALID,
        "invalid current block type 0x%2x in tape given to %s", block->type, __func__
      );
      return LIBSPECTRUM_ERROR_INVALID;
  }

  return LIBSPECTRUM_ERROR_NONE;
}
