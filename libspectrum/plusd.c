/* plusd.c: Routines for handling DISCiPLE/+D snapshots
   Copyright (c) 1998,2003 Philip Kendall
   Copyright (c) 2007,2015 Stuart Brady

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

#include <string.h>

#include "internals.h"

#define PLUSD_HEADER_LENGTH 22

static libspectrum_error
identify_machine( size_t buffer_length, libspectrum_snap *snap );
static libspectrum_error
libspectrum_plusd_read_header( const libspectrum_byte *buffer,
			       size_t buffer_length, libspectrum_snap *snap );
static libspectrum_error
libspectrum_plusd_read_data( const libspectrum_byte *buffer,
			     libspectrum_snap *snap );
static libspectrum_byte
readbyte( libspectrum_snap *snap, libspectrum_word address );
static void
libspectrum_plusd_read_128_data( libspectrum_snap *snap,
				 const libspectrum_byte *buffer );

libspectrum_error
libspectrum_plusd_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
			size_t buffer_length )
{
  int error;

  error = identify_machine( buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_plusd_read_header( buffer, buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  buffer += PLUSD_HEADER_LENGTH;

  error = libspectrum_plusd_read_data( buffer, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
identify_machine( size_t buffer_length, libspectrum_snap *snap )
{
  switch( buffer_length ) {
  case PLUSD_HEADER_LENGTH + 0xc000:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;
  case PLUSD_HEADER_LENGTH + 1 + 0x20000:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "plusd identify_machine: unknown length" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
libspectrum_plusd_read_header( const libspectrum_byte *buffer,
			       size_t buffer_length, libspectrum_snap *snap )
{
  libspectrum_byte i;

  libspectrum_snap_set_iy ( snap, buffer[ 0] + buffer[ 1] * 0x100 );
  libspectrum_snap_set_ix ( snap, buffer[ 2] + buffer[ 3] * 0x100 );
  libspectrum_snap_set_de_( snap, buffer[ 4] + buffer[ 5] * 0x100 );
  libspectrum_snap_set_bc_( snap, buffer[ 6] + buffer[ 7] * 0x100 );
  libspectrum_snap_set_hl_( snap, buffer[ 8] + buffer[ 9] * 0x100 );
  libspectrum_snap_set_f_ ( snap, buffer[10] );
  libspectrum_snap_set_a_ ( snap, buffer[11] );
  libspectrum_snap_set_de ( snap, buffer[12] + buffer[13] * 0x100 );
  libspectrum_snap_set_bc ( snap, buffer[14] + buffer[15] * 0x100 );
  libspectrum_snap_set_hl ( snap, buffer[16] + buffer[17] * 0x100 );
  /* Header offset 18 is 'rubbish' */
  i = buffer[19]; libspectrum_snap_set_i( snap, i );
  libspectrum_snap_set_sp ( snap, buffer[20] + buffer[21] * 0x100 );

  /* Make a guess at the interrupt mode depending on what I was set to */
  libspectrum_snap_set_im( snap, ( i == 0 || i == 63 ) ? 1 : 2 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
libspectrum_plusd_read_data( const libspectrum_byte *buffer,
			     libspectrum_snap *snap )
{
  libspectrum_byte iff;
  libspectrum_word sp;
  int error;

  sp = libspectrum_snap_sp( snap );

  /* We must have 0x4000 <= SP <= 0xfffa so we can rescue the stacked
     registers */
  if( sp < 0x4000 || sp > 0xfffa ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_plusd_read_data: SP invalid (0x%04x)", sp
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_48:

    /* Split the RAM into separate pages */
    error = libspectrum_split_to_48k_pages( snap, buffer );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  case LIBSPECTRUM_MACHINE_128:

    libspectrum_snap_set_out_128_memoryport( snap, buffer[0] );
    buffer++;
    libspectrum_plusd_read_128_data( snap, buffer );

    break;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_plusd_read_data: unknown machine" );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  /* R, IFF, AF and PC are stored on the stack */
  iff = readbyte( snap, sp ) & 0x04;
  libspectrum_snap_set_r   ( snap, readbyte( snap, sp + 1 ) );
  libspectrum_snap_set_iff1( snap, iff );
  libspectrum_snap_set_iff2( snap, iff );
  libspectrum_snap_set_f   ( snap, readbyte( snap, sp + 2 ) );
  libspectrum_snap_set_a   ( snap, readbyte( snap, sp + 3 ) );
  libspectrum_snap_set_pc  ( snap, readbyte( snap, sp + 4 ) +
				   readbyte( snap, sp + 5 ) * 0x100 );

  /* Store SP + 6 to account for those unstacked values */
  libspectrum_snap_set_sp( snap, sp + 6 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_byte
readbyte( libspectrum_snap *snap, libspectrum_word address )
{
  int page;

  switch( address >> 14 ) {

  case 1:
    page = 5;
    break;

  case 2:
    page = 2;
    break;

  case 3:
    page = libspectrum_snap_out_128_memoryport( snap ) & 0x07;
    break;

  default:
    return 0;

  }

  return libspectrum_snap_pages( snap, page )[ address & 0x3fff ];
}

static void
libspectrum_plusd_read_128_data( libspectrum_snap *snap,
				 const libspectrum_byte *buffer )
{
  int i;

  for( i=0; i<8; i++ ) {

    libspectrum_byte *ram;

    ram = libspectrum_new( libspectrum_byte, 0x4000 );
    libspectrum_snap_set_pages( snap, i, ram );

    memcpy( ram, buffer, 0x4000 );
    buffer += 0x4000;

  }
}
