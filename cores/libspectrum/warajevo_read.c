/* warajevo_read.c: Routines for reading Warajevo .tap files
   Copyright (c) 2001, 2002 Philip Kendall, Darren Salt
   Copyright (c) 2003-2015 Fredrick Meunier

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

static const libspectrum_word RECORD_SAMPLES = 0xfffe;
static const libspectrum_word COMPRESSED_BLOCK = 0xffff;

enum raw_data_frequency {
  HZ15000 = 0,
  HZ22050,
  HZ30303,
  HZ44100,
};

#ifdef WORDS_BIGENDIAN

typedef struct
{
  unsigned unused    : 3;
  unsigned frequency : 2;
  unsigned bits_used : 3;
} status_bits;

#else                   /* #ifdef WORDS_BIGENDIAN */

typedef struct
{
  unsigned bits_used : 3;
  unsigned frequency : 2;
  unsigned unused    : 3;
} status_bits;

#endif			/* #ifdef WORDS_BIGENDIAN */

typedef union
{
  libspectrum_byte byte;
  status_bits bits;
} status_type;

/* Non-terminating nodes from the size state machine */
typedef enum state_type {
   b = 0, /* start */
   b0,
   b1,
   b01,
   b10,
   b11,
   b110,
   b111,
} state_type;

/* Non-terminating nodes from the offset state machine */
typedef enum vstate_type {
   v = 0, /* start */
   v0,
   v01,
   v00,
   v000,
   v001,
   v0010,
   v0011,
} vstate_type;

typedef struct
{
  int mode; /* 0 = copy byte to output, 1 = building copy size,
               2 = building offset, 3 = command complete */
  state_type state;
  vstate_type vstate;
  int offset;
  unsigned int size;
  libspectrum_byte pattern;
  size_t pattern_depth;
} copy_command;

static copy_command command;

/* The Warajevo .tap file signature (3rd group of 4 bytes) also end of tap
   marker */
static const libspectrum_dword warajevo_signature = 0xffffffff;

/*** Local function prototypes ***/

static libspectrum_dword lsb2dword( const libspectrum_byte *mem );
static libspectrum_word lsb2word( const libspectrum_byte *mem ); 

static libspectrum_error
exec_command( libspectrum_byte *dest, const libspectrum_byte *src,
	      const libspectrum_byte *end, size_t *sp, size_t *pc,
	      size_t *bytes_written, const size_t to_write );

static libspectrum_error
get_next_block( size_t *offset, const libspectrum_byte *buffer,
		const libspectrum_byte *end, libspectrum_tape *tape );

static libspectrum_error
read_rom_block( libspectrum_tape *tape, const libspectrum_byte *ptr,
		const libspectrum_byte *end, size_t offset );

static libspectrum_error
read_raw_data( libspectrum_tape *tape, const libspectrum_byte *ptr,
	       const libspectrum_byte *end, size_t offset );

static libspectrum_error
add_bit_to_copy_command( libspectrum_byte *dest, const libspectrum_byte *src,
			 const libspectrum_byte *end, libspectrum_byte bit,
			 size_t *sp, size_t *bytes_written );

static void
reset_copy_command( void );

static libspectrum_error
decompress_block( libspectrum_byte *dest, const libspectrum_byte *src,
		  const libspectrum_byte *end, size_t signature,
		  size_t length );

/*** Function definitions ***/

static libspectrum_dword
lsb2dword( const libspectrum_byte *mem )
{
  return ( mem[0] <<  0 ) |
         ( mem[1] <<  8 ) |
         ( mem[2] << 16 ) |
         ( mem[3] << 24 );
} 
  
static libspectrum_word
lsb2word( const libspectrum_byte *mem ) 
{
  return ( mem[0] << 0 ) |
         ( mem[1] << 8 );
}

/* The main load function */

libspectrum_error
internal_warajevo_read( libspectrum_tape *tape,
			const libspectrum_byte *buffer, size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *ptr, *end;

  size_t offset;

  ptr = buffer; end = buffer + length;

  /* Must be at least as many bytes as the signature, the initial block pointer
     and source pointer block */
  if( length < 12 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_warajevo_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Now check the signature */
  if( lsb2dword( ptr + 8 ) != warajevo_signature ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_warajevo_read: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Get inital offset */
  offset = lsb2dword( ptr );

  while( offset != warajevo_signature ) {
    error = get_next_block( &offset, ptr, end, tape );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_next_block( size_t *offset, const libspectrum_byte *buffer,
		const libspectrum_byte *end, libspectrum_tape *tape )
{
  int error;
  libspectrum_dword next_block;

  /* Check we have enough data */
  if( end - buffer < *offset || end - buffer - *offset < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_warajevo_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  next_block = lsb2dword( buffer + 4 + *offset );

  /* Check for EOF marker */
  if( next_block == warajevo_signature ) {
    *offset = warajevo_signature;
    return LIBSPECTRUM_ERROR_NONE;
  }

  if( lsb2word( buffer + 8 + *offset ) == RECORD_SAMPLES ) {
    error = read_raw_data( tape, buffer, end, *offset );
  } else {
    error = read_rom_block( tape, buffer, end, *offset );
  }

  if( error ) { libspectrum_tape_free( tape ); return error; }

  /* Advance to the next block */
  *offset = next_block;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Executes a bytes worth of commands */
static libspectrum_error
exec_command( libspectrum_byte *dest, const libspectrum_byte *src,
	      const libspectrum_byte *end GCC_UNUSED, size_t *sp, size_t *pc,
	      size_t *bytes_written, const size_t to_write )
{
  size_t i;
  libspectrum_byte command_byte;
  libspectrum_byte bit;
  libspectrum_error error;

  command_byte = src[(*pc)++];

  for( i = 0; i < 8; i++ ) {
    bit = ( command_byte & ( 0x80 >> i ) ) ? 1 : 0;

    error = add_bit_to_copy_command( dest, src, dest + to_write, bit, sp,
                                     bytes_written );
    if( error ) { return error; }

    if( *bytes_written >= to_write ) break;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
execute_copy_command( libspectrum_byte *dest, const libspectrum_byte *end,
                      size_t *bytes_written )
{
  if( ( (*bytes_written + 1) < command.offset ) ||
      ( dest + *bytes_written - command.offset + 1 + command.size > end ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
                     "execute_copy_command: corrupt compressed block in file" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( dest + *bytes_written,
          dest + *bytes_written - command.offset + 1, command.size );

  *bytes_written += command.size;

  reset_copy_command();

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
add_bit_to_copy_command( libspectrum_byte *dest, const libspectrum_byte *src,
			 const libspectrum_byte *end, libspectrum_byte bit,
			 size_t *sp, size_t *bytes_written )
{
  if( command.mode == 0 ) {
    if( bit == 0 ) {
      dest[(*bytes_written)++] = src[(*sp)++];
    } else {
      command.mode = 1;
      return LIBSPECTRUM_ERROR_NONE;
    }
  }

  if( command.mode == 1 ) {
    switch( command.state ){
    case  b: /* start */
      command.state = bit == 0 ? b0 : b1;
      break;
    case b0:
      if( bit == 0 ) { /* b00 */
        command.size=3;
        command.mode=2;
      } else
        command.state = b01;
      break;
    case b1:
      command.state = bit == 0 ? b10 : b11;
      break;
    case b01:
      if( bit == 0 ) { /* b010 */
        /* no higher byte */
        command.size=2;
        command.mode=3;
        command.offset = src[(*sp)++];
        return execute_copy_command( dest, end, bytes_written );
      } else { /* b011 */
        command.size=10 + src[(*sp)++];
        command.mode=2;
      }
      break;
    case b10:
      if( bit == 0 ) { /* b100 */
        command.size=4;
        command.mode=2;
      } else { /* b101 */
        command.size=5;
        command.mode=2;
      }
      break;
    case b11:
      command.state = bit == 0 ? b110 : b111;
      break;
    case b110:
      if( bit == 0 ) { /* b1100 */
        command.size=6;
        command.mode=2;
      } else { /* b1101 */
        command.size=7;
        command.mode=2;
      }
      break;
    case b111:
      if( bit == 0 ) { /* b1110 */
        command.size=8;
        command.mode=2;
      } else { /* b1111 */
        command.size=9;
        command.mode=2;
      }
      break;
    }
    return LIBSPECTRUM_ERROR_NONE;
  }

  if( command.mode == 2 ) {
    switch( command.vstate ){
    case v: /* start */
      command.offset = src[(*sp)++];
      if( bit == 0 )
        command.vstate = v0;
      else { /* v1 */
        /* no higher byte */
        command.mode=3;
      }
      break;
    case v0:
      command.vstate = bit == 0 ? v00 : v01;
      break;
    case v01: /* v01 + nnnn */
      command.pattern <<= 1;
      command.pattern |= bit;
      if( ++command.pattern_depth == 4 ) {
        command.offset += (command.pattern + 7) << 8;
        command.mode=3;
      }
      break;
    case v00:
      command.vstate = bit == 0 ? v000 : v001;
      break;
    case v000:
      if( bit == 0 ) { /* v0000 */
        command.offset += 1 << 8;
        command.mode=3;
      } else { /* v0001 */
        command.offset += 2 << 8;
        command.mode=3;
      }
      break;
    case v001:
      command.vstate = bit == 0 ? v0010 : v0011;
      break;
    case v0010:
      if( bit == 0 ) { /* v00100 */
        command.offset += 3 << 8;
        command.mode=3;
      } else { /* v00101 */
        command.offset += 4 << 8;
        command.mode=3;
      }
      break;
    case v0011:
      if( bit == 0 ) { /* v00110 */
        command.offset += 5 << 8;
        command.mode=3;
      } else { /* v00111 */
        command.offset += 6 << 8;
        command.mode=3;
      }
      break;
    }
  }

  if( command.mode == 3 ) {
    return execute_copy_command( dest, end, bytes_written );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
reset_copy_command( void )
{
  memset( &command, 0, sizeof( copy_command ) );
}

static libspectrum_error
read_rom_block( libspectrum_tape *tape, const libspectrum_byte *ptr,
		const libspectrum_byte *end, size_t offset )
{
  libspectrum_tape_block *block =
    libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_ROM );
  libspectrum_error error;
  libspectrum_word size;
  libspectrum_word block_size;
  const libspectrum_byte *data; libspectrum_byte *block_data;
  size_t i, length;

  size = lsb2word( ptr + offset + 8 );

  if( size == COMPRESSED_BLOCK ) {
    /* Decompressed length + flag byte and checksum */
    length = lsb2word( ptr + offset + 11 ) + 2;
    block_size = lsb2word( ptr + offset + 13 );
    data = ptr + offset + 17 ;
  } else {
    /* Get the length + flag byte and checksum */
    length = size + 2;
    block_size = size;
    data = ptr + offset + 11;
  }
  libspectrum_tape_block_set_data_length( block, length );

  /* Have we got enough bytes left in buffer? */
  if( end - data < (ptrdiff_t)block_size ) {
    libspectrum_free( block );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "warajevo_read_rom_block: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data */
  block_data = libspectrum_new( libspectrum_byte, length );
  libspectrum_tape_block_set_data( block, block_data );

  /* Add flag */
  block_data[0] = *( ptr + offset + 10 );

  if( size == COMPRESSED_BLOCK ) {

    error = decompress_block( block_data + 1, data, end,
			      lsb2word( ptr + offset + 15 ), length - 2 );
    if( error ) { libspectrum_free( block_data ); libspectrum_free( block ); return error; }
  } else {
    /* Uncompressed block: just copy the data across */
    memcpy( block_data + 1, data, length - 2 );
  }

  /* Calculate checksum */
  block_data[ length - 1 ] = 0;
  for( i = 0; i < length - 1; i++ )
    block_data[ length - 1 ] ^= block_data[i];

  /* Give a 1s pause after each block */
  libspectrum_set_pause_ms( block, 1000 );

  /* Put the block into the block list */
  libspectrum_tape_append_block( tape, block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_raw_data( libspectrum_tape *tape, const libspectrum_byte *ptr,
	       const libspectrum_byte *end, size_t offset )
{
  libspectrum_tape_block *block =
    libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RAW_DATA );
  libspectrum_tape_block *last_block;
  libspectrum_error error;
  libspectrum_word compressed_size, decompressed_size;
  const libspectrum_byte *data = ptr + offset + 17;
  status_type status;
  size_t length, bit_length; libspectrum_byte *block_data;

  decompressed_size = lsb2word( ptr + offset + 11 );
  compressed_size = lsb2word( ptr + offset + 13 );

  length = decompressed_size;
  libspectrum_tape_block_set_data_length( block, length );

  /* Have we got enough bytes left in buffer? */
  if( end - data < (ptrdiff_t)compressed_size ) {
    libspectrum_free( block );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "warajevo_read_raw_data: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data */
  block_data = libspectrum_new( libspectrum_byte, length );
  libspectrum_tape_block_set_data( block, block_data );

  if( compressed_size != decompressed_size ) {

    error = decompress_block( block_data, data, end,
			      lsb2word( ptr + offset + 15 ), length );
    if( error ) {
      libspectrum_free( block_data ); libspectrum_free( block ); return error;
    }
  } else {
    /* Uncompressed block: just copy the data across */
    memcpy( block_data, data, length );
  }

  status.byte = *( ptr + offset + 10 );

  /* Get the metadata */
  switch( status.bits.frequency ) {
  case HZ15000: bit_length = 233; break;
  case HZ22050: bit_length = 158; break;
  case HZ30303: bit_length = 115; break;
  case HZ44100: bit_length =  79; break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "read_raw_data: unknown frequency %d",
			     status.bits.frequency );
    libspectrum_free( block_data ); libspectrum_free( block );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  libspectrum_tape_block_set_bit_length( block, bit_length );

  libspectrum_set_pause_tstates( block, 0 );
  libspectrum_tape_block_set_bits_in_last_byte( block,
						status.bits.bits_used + 1 );

  /* Warajevo TAPs have a relatively short limit on RAW blocks of 64995 bytes
     which is only 11.74 secs at 44100Hz.

     As a result when these blocks are used they often are a sequence of these
     short blocks as opposed to other formats which have a long single block
     corresponding to the sampled section of tape.

     This presents a problem for libspectrum which assumes that a pulse cannot
     span two different blocks, so we will work around this limitation by
     coalesing adjacent compatible RAW blocks to ensure the pulse structure is
     preserved.
     */

  /* Check if the last block was also a raw block of the same sample rate
     and with all 8 bits used in the last byte */
  last_block = libspectrum_tape_peek_last_block( tape );
  if( last_block &&
      libspectrum_tape_block_type( last_block ) ==
        LIBSPECTRUM_TAPE_BLOCK_RAW_DATA && 
      libspectrum_tape_block_bit_length( last_block ) == bit_length &&
      libspectrum_tape_block_bits_in_last_byte( last_block ) == 8 ) {
    /* Combine the two blocks */
    size_t new_length = libspectrum_tape_block_data_length( last_block ) + 
                          length;
    block_data = libspectrum_renew( libspectrum_byte,
				    libspectrum_tape_block_data( last_block ),
				    new_length );

    memcpy( block_data + libspectrum_tape_block_data_length( last_block ),
            libspectrum_tape_block_data( block ), length );
    libspectrum_tape_block_set_data( last_block, block_data );
    libspectrum_tape_block_set_data_length( last_block, new_length );
    libspectrum_tape_block_set_bits_in_last_byte( last_block,
                                                  status.bits.bits_used + 1 );
    libspectrum_tape_block_free( block );
  } else {
    /* Put the block into the block list */
    libspectrum_tape_append_block( tape, block );
  }

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
decompress_block( libspectrum_byte *dest, const libspectrum_byte *src,
		  const libspectrum_byte *end, size_t signature,
		  size_t length )
{
  size_t bytes_written = 0, pc = 0;
  size_t sp = signature + 1;

  libspectrum_error error;

  reset_copy_command();
    
  while( ( pc <= signature ) && ( bytes_written != length ) ) {
    error = exec_command( dest, src, end, &sp, &pc, &bytes_written, length );
    if( error ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}
