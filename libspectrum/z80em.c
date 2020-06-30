/* z80em.c: Routines for handling Z80Em raw audio files
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2002 Darren Salt
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

libspectrum_error
libspectrum_z80em_read( libspectrum_tape *tape,
			const libspectrum_byte *buffer, size_t length )
{
  libspectrum_tape_block *block;
  libspectrum_tape_rle_pulse_block *z80em_block;

  static const char id[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Raw tape sample";

  if( length < sizeof( id ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_z80em_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( id, buffer, sizeof( id ) ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "libspectrum_z80em_read: wrong signature" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  block = libspectrum_tape_block_alloc( LIBSPECTRUM_TAPE_BLOCK_RLE_PULSE );

  z80em_block = &block->types.rle_pulse;
  z80em_block->scale = 7; /* 1 time unit == 7 clock ticks */

  buffer += sizeof( id );
  length -= sizeof( id );

  /* Claim memory for the data (it's one big lump) */
  z80em_block->length = length;
  z80em_block->data = libspectrum_new( libspectrum_byte, length );

  /* Copy the data across */
  memcpy( z80em_block->data, buffer, length );

  libspectrum_tape_append_block( tape, block );

  return LIBSPECTRUM_ERROR_NONE;
}
