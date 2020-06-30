/* pzx_read.c: Routines for reading .pzx files
   Copyright (c) 2001, 2002 Philip Kendall, Darren Salt
   Copyright (c) 2011-2015 Fredrick Meunier

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
#include <stdio.h>
#include <string.h>

#include "internals.h"

/* Used for passing internal data around */

typedef struct pzx_context {

  libspectrum_word version;

} pzx_context;

/* Constants etc for each block type */

#define PZX_HEADER "PZXT"
struct info_t {

  const char *id;
  int archive_info_id;

};

/* Needs to be in strcmp  order */
static struct info_t info_ids[] = {

  { "Author",       0x02 },
  { "Comment",      0xff },
  { "Language",     0x04 },
  { "Origin",       0x08 },
  { "Price",        0x06 },
  { "Protection",   0x07 },
  { "Publisher",    0x01 },
  { "Type",         0x05 },
  { "Year",         0x03 },

};

#define PZX_PULSE  "PULS"

#define PZX_DATA   "DATA"

#define PZX_PAUSE  "PAUS"

#define PZX_BROWSE "BRWS"

#define PZX_STOP   "STOP"
static const libspectrum_byte PZXF_STOP48 = 1;

/* TODO: an extension to be similar to the TZX Custom Block Picture type */
#define PZX_INLAY  "inly"

static const char * const signature = PZX_HEADER;
static const size_t signature_length = 4;

static libspectrum_error
read_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
            const libspectrum_byte *end, pzx_context *ctx );

typedef libspectrum_error (*read_block_fn)( libspectrum_tape *tape,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length,
                                            pzx_context *ctx );

static libspectrum_error
pzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t length, libspectrum_byte **data );

static libspectrum_error
pzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 char **dest );

static int
info_t_compar(const void *a, const void *b)
{
  const char *key = a;
  const struct info_t *test = b;
  return strcmp( key, test->id );
}

static int
get_id_byte( char *info_tag ) {
  struct info_t *info =
    (struct info_t*)bsearch( info_tag, info_ids, ARRAY_SIZE( info_ids ),
                             sizeof( struct info_t ), info_t_compar );
  return info == NULL ? -1 : info->archive_info_id;
}

static libspectrum_error
read_pzxt_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  libspectrum_error error;

  size_t i = 0;
  size_t count = 0;
  int id;
  int *ids = NULL;
  char *info_tag = NULL;
  char *string;
  char **strings = NULL;
  const libspectrum_byte *block_end = *buffer + data_length;

  if( data_length < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "read_pzxt_block: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  ctx->version = (**buffer) << 8; (*buffer)++;
  ctx->version |= **buffer; (*buffer)++;

  if( ctx->version < 0x0100 || ctx->version >= 0x0200 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_pzxt_block: only version 1 pzx files are "
                             "supported" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( *buffer < block_end ) {
    ids = libspectrum_new( int, 1 );
    strings = libspectrum_new( char *, 1 );
    count = 1;
    i = 0;

    ids[0] = 0x00;

    /* Read in the title string itself */
    error = pzx_read_string( buffer, block_end, &strings[0] );
    if( error ) {
      libspectrum_free( strings[0] );
      return error;
    }
  }

  while( *buffer < block_end ) {
    error = pzx_read_string( buffer, block_end, &info_tag );
    if( error ) {
      size_t j;
      for( j = 0; j < i; j++ ) libspectrum_free( strings[j] );
      libspectrum_free( strings ); libspectrum_free( ids );
      return error;
    }

    /* Get the ID byte */
    id = get_id_byte( info_tag );
    
    /* Read in the string itself */
    error = pzx_read_string( buffer, block_end, &string );
    if( error ) {
      size_t j;
      for( j = 0; j < i; j++ ) libspectrum_free( strings[j] );
      libspectrum_free( strings ); libspectrum_free( ids );
      return error;
    }

    i = count++;
    ids = libspectrum_renew( int, ids, count );
    strings = libspectrum_renew( char *, strings, count );

    if( id == -1 ) {
      size_t new_len = strlen( info_tag ) + strlen( string ) +
                       strlen( ": " ) + 1;
      char *comment = libspectrum_new( char, new_len );
      snprintf( comment, new_len, "%s: %s", info_tag, string );
      libspectrum_free( string );
      ids[i] = 0xff;
      strings[i] = comment;
    } else {
      ids[i] = id;
      strings[i] = string;
    }

    libspectrum_free( info_tag );
  }

  if( count ) {
    libspectrum_tape_block* block =
      libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO );

    libspectrum_tape_block_set_count( block, count );
    libspectrum_tape_block_set_ids( block, ids );
    libspectrum_tape_block_set_texts( block, strings );

    libspectrum_tape_append_block( tape, block );
  }

  return LIBSPECTRUM_ERROR_NONE;
}


static libspectrum_error
read_data_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  const libspectrum_byte *block_end = *buffer + data_length;
  libspectrum_tape_block* block;
  libspectrum_byte *data;

  libspectrum_error error;
  libspectrum_dword count;
  int initial_level;
  size_t count_bytes;
  size_t bits_in_last_byte;
  libspectrum_word tail;
  libspectrum_byte p0_count;
  libspectrum_byte p1_count;
  libspectrum_word *p0_pulses;
  libspectrum_word *p1_pulses;

  /* Check there's enough left in the buffer for all the metadata */
  if( data_length < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "read_data_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the metadata */
  count = libspectrum_read_dword( buffer );
  initial_level = !!(count & 0x80000000);
  count &= 0x7fffffff;
  count_bytes = libspectrum_bits_to_bytes( count );
  bits_in_last_byte =
    count % LIBSPECTRUM_BITS_IN_BYTE ?
      count % LIBSPECTRUM_BITS_IN_BYTE : LIBSPECTRUM_BITS_IN_BYTE;
  tail = libspectrum_read_word( buffer );
  p0_count = **buffer; (*buffer)++;
  p1_count = **buffer; (*buffer)++;

  /* need to confirm that we have enough length left for the pulse definitions
   */
  if( data_length < 8 + 2*(p0_count + p1_count) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "read_data_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  error = pzx_read_data( buffer, block_end,
                         p0_count * sizeof( libspectrum_word ),
                         (libspectrum_byte**)&p0_pulses );
  if( error ) return error;

  error = pzx_read_data( buffer, block_end,
                         p1_count * sizeof( libspectrum_word ),
                         (libspectrum_byte**)&p1_pulses );
  if( error ) { libspectrum_free( p0_pulses ); return error; }

  /* And the actual data */
  error = pzx_read_data( buffer, block_end, count_bytes, &data );
  if( error ) {
    libspectrum_free( p0_pulses );
    libspectrum_free( p1_pulses );
    return error;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_DATA_BLOCK );

  libspectrum_tape_block_set_count( block, count );
  libspectrum_tape_block_set_tail_length( block, tail );
  libspectrum_tape_block_set_level( block, initial_level );
  libspectrum_tape_block_set_bit0_pulse_count( block, p0_count );
  libspectrum_tape_block_set_bit0_pulses( block, p0_pulses );
  libspectrum_tape_block_set_bit1_pulse_count( block, p1_count );
  libspectrum_tape_block_set_bit1_pulses( block, p1_pulses );
  libspectrum_tape_block_set_data_length( block, count_bytes );
  libspectrum_tape_block_set_bits_in_last_byte( block, bits_in_last_byte );
  libspectrum_tape_block_set_data( block, data );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_next_pulse( const libspectrum_byte **buffer, const libspectrum_byte *end,
                 size_t *pulse_repeats, libspectrum_dword *length )
{
  /* While we have at least one int 16 left try to extract the next pulse */
  if( ( end - (*buffer) ) < (ptrdiff_t)2 ) goto pzx_corrupt;

  *pulse_repeats = 1;
  *length = libspectrum_read_word( buffer );
  if( *length > 0x8000 ) {
    if( ( end - (*buffer) ) < (ptrdiff_t)2 ) goto pzx_corrupt;
    *pulse_repeats = *length & 0x7fff;
    *length = libspectrum_read_word( buffer );
  }
  if( *length >= 0x8000 ) {
    if( ( end - (*buffer) ) < (ptrdiff_t)2 ) goto pzx_corrupt;
    *length &= 0x7fff;
    *length <<= 16;
    *length |= libspectrum_read_word( buffer );
  }

  /* And return */
  return LIBSPECTRUM_ERROR_NONE;

pzx_corrupt:
  libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
                           "read_next_pulse: not enough data in buffer" );
  return LIBSPECTRUM_ERROR_CORRUPT;
}

static libspectrum_error
read_puls_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  size_t count = 0;
  size_t pulse_repeats;
  libspectrum_dword length;
  libspectrum_tape_block *block;
  libspectrum_error error;
  size_t buffer_sizes = 64;
  size_t *pulse_repeats_buffer =
    libspectrum_new( size_t, buffer_sizes );
  libspectrum_dword *lengths_buffer =
    libspectrum_new( libspectrum_dword, buffer_sizes );
  const libspectrum_byte *block_end = *buffer + data_length;

  while( ( block_end - (*buffer) ) > (ptrdiff_t)0 ) {
    error = read_next_pulse( buffer, block_end, &pulse_repeats, &length );
    if( error ) {
      libspectrum_free( pulse_repeats_buffer );
      libspectrum_free( lengths_buffer );
      return error;
    }
    pulse_repeats_buffer[ count ] = pulse_repeats;
    lengths_buffer[ count ] = length;
    count++;
    if( buffer_sizes == count ) {
      buffer_sizes *= 2;
      pulse_repeats_buffer =
        libspectrum_renew( size_t, pulse_repeats_buffer, buffer_sizes );
      lengths_buffer =
        libspectrum_renew( libspectrum_dword, lengths_buffer, buffer_sizes );
    }
  }

  if( count == 0 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
                           "read_puls_block: no pulses found in pulse block" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( buffer_sizes != count ) {
    pulse_repeats_buffer =
      libspectrum_renew( size_t, pulse_repeats_buffer, count );
    lengths_buffer =
      libspectrum_renew( libspectrum_dword, lengths_buffer, count );
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PULSE_SEQUENCE );

  libspectrum_tape_block_set_count( block, count );
  libspectrum_tape_block_set_pulse_lengths( block, lengths_buffer );
  libspectrum_tape_block_set_pulse_repeats( block, pulse_repeats_buffer );

  libspectrum_tape_append_block( tape, block );

  /* And return */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_paus_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  libspectrum_tape_block *block;
  libspectrum_dword pause_tstates;
  int initial_level;

  /* Check the pause actually exists */
  if( data_length < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "read_paus_block: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PAUSE );

  pause_tstates = libspectrum_read_dword( buffer );
  initial_level = !!(pause_tstates & 0x80000000);
  pause_tstates &= 0x7fffffff;

  /* Set the pause length */
  libspectrum_set_pause_tstates( block, pause_tstates );
  libspectrum_tape_block_set_level( block, initial_level );

  libspectrum_tape_append_block( tape, block );

  /* And return */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_brws_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  libspectrum_tape_block* block;
  char *text;
  libspectrum_error error;

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_COMMENT );

  /* Get the actual comment */
  error = pzx_read_string( buffer, *buffer + data_length, &text );
  if( error ) { libspectrum_free( block ); return error; }
  libspectrum_tape_block_set_text( block, text );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_stop_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
                 const libspectrum_byte *end, size_t data_length,
                 pzx_context *ctx )
{
  libspectrum_tape_block *block;
  libspectrum_word flags;

  if( data_length < 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "tzx_read_stop: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  flags = libspectrum_read_word( buffer );

  if( flags == PZXF_STOP48 ) {
    block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_STOP48 );
  } else {
    /* General stop is a 0 duration pause */
    block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_PAUSE );
    libspectrum_tape_block_set_pause( block, 0 );
  }

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_block( libspectrum_tape *tape GCC_UNUSED,
	    const libspectrum_byte **buffer,
	    const libspectrum_byte *end GCC_UNUSED,
            size_t data_length,
            pzx_context *ctx GCC_UNUSED )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_block_t {

  const char *id;
  read_block_fn function;

};

static struct read_block_t read_blocks[] = {

  { PZX_HEADER,     read_pzxt_block },
  { PZX_PULSE,      read_puls_block },
  { PZX_DATA,       read_data_block },
  { PZX_PAUSE,      read_paus_block },
  { PZX_BROWSE,     read_brws_block },
  { PZX_STOP,       read_stop_block },
  { PZX_INLAY,      skip_block      },

};

static libspectrum_error
read_block_header( char *id, libspectrum_dword *data_length, 
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end )
{
  if( end - *buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "read_block_header: not enough data for block header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;
  *data_length = libspectrum_read_dword( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_block( libspectrum_tape *tape, const libspectrum_byte **buffer,
            const libspectrum_byte *end, pzx_context *ctx )
{
  char id[5];
  libspectrum_dword data_length;
  libspectrum_error error;
  size_t i; int done;

  error = read_block_header( id, &data_length, buffer, end );
  if( error ) return error;

  if( end - *buffer < data_length ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "read_block: block length goes beyond end of file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  done = 0;

  for( i = 0; !done && i < ARRAY_SIZE( read_blocks ); i++ ) {

    if( !memcmp( id, read_blocks[i].id, 4 ) ) {
      error = read_blocks[i].function( tape, buffer, end, data_length, ctx );
      if( error ) return error;
      done = 1;
    }

  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_block: unknown block id '%s'", id );
    *buffer += data_length;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* The main load function */

libspectrum_error
internal_pzx_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
                   size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *end = buffer + length;
  pzx_context *ctx;

  if( end - buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "internal_pzx_read: not enough data for PZX header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( buffer, signature, signature_length ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_SIGNATURE,
      "internal_pzx_read: wrong signature"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  ctx = libspectrum_new( pzx_context, 1 );
  ctx->version = 0;

  while( buffer < end ) {
    error = read_block( tape, &buffer, end, ctx );
    if( error ) {
      libspectrum_free( ctx );
      return error;
    }
  }

  libspectrum_free( ctx );
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pzx_read_data( const libspectrum_byte **ptr, const libspectrum_byte *end,
	       size_t length, libspectrum_byte **data )
{
  /* Have we got enough bytes left in buffer? */
  if( ( end - (*ptr) ) < (ptrdiff_t)(length) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "pzx_read_data: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data; the check for *length is to avoid
     the implementation-defined behaviour of malloc( 0 ) */
  if( length ) {
    *data = libspectrum_new( libspectrum_byte, length );
    /* Copy the block data across, and move along */
    memcpy( *data, *ptr, length ); *ptr += length;
  } else {
    *data = NULL;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pzx_read_string( const libspectrum_byte **ptr, const libspectrum_byte *end,
		 char **dest )
{
  size_t length = 0;
  char *ptr2;
  size_t buffer_size = 64;
  char *buffer = libspectrum_new( char, buffer_size );

  while( **ptr != '\0' && *ptr < end ) {
    if( length == buffer_size ) {
      buffer_size *= 2;
      buffer = libspectrum_renew( char, buffer, buffer_size );
    }
    *(buffer + length++) = **ptr; (*ptr)++;
  }

  /* Advance past the null terminator discarding any garbage */
  *ptr = end;

  *dest = libspectrum_new( char, (length + 1) );

  strncpy( *dest, buffer, length );

  /* Null terminate the string */
  (*dest)[length] = '\0';

  /* Translate line endings */
  for( ptr2 = (*dest); *ptr2; ptr2++ ) if( *ptr2 == '\r' ) *ptr2 = '\n';

  libspectrum_free( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}
