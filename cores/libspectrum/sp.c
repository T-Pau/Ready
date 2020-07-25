/* sp.c: Routines for handling .sp snapshots
   Copyright (c) 1998,2003 Philip Kendall

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

static const size_t SP_HEADER_LENGTH = 37;

libspectrum_error
libspectrum_sp_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		     size_t length )
{
  libspectrum_error error;
  libspectrum_word start, memory_length, flags;
  libspectrum_byte *memory;

  /* Length must be at least 48K of RAM plus the 'trailer' */
  if( length < SP_HEADER_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sp_read: not enough bytes for .sp header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check the signature */
  if( buffer[0] != 'S' || buffer[1] != 'P' ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_SIGNATURE,
      "libspectrum_sp_read: 'SP' signature not present"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }
  buffer += 2;

  memory_length = libspectrum_read_word( &buffer );
  start = libspectrum_read_word( &buffer );

  /* Check for overrun of 48K memory */
  if( start + (libspectrum_dword)memory_length > 0x10000 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_sp_read: memory dump extends beyond 0xffff"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( start + memory_length < 0x8000 ) {
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_16 );
  } else {
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
  }

  /* Note that `buffer' has been incremented by 6 above */
    
  libspectrum_snap_set_bc     ( snap, buffer[ 0] + buffer[ 1] * 0x100 );
  libspectrum_snap_set_de     ( snap, buffer[ 2] + buffer[ 3] * 0x100 );
  libspectrum_snap_set_hl     ( snap, buffer[ 4] + buffer[ 5] * 0x100 );
  libspectrum_snap_set_f      ( snap, buffer[ 6] );
  libspectrum_snap_set_a      ( snap, buffer[ 7] );
  libspectrum_snap_set_ix     ( snap, buffer[ 8] + buffer[ 9] * 0x100 );
  libspectrum_snap_set_iy     ( snap, buffer[10] + buffer[11] * 0x100 );
  libspectrum_snap_set_bc_    ( snap, buffer[12] + buffer[13] * 0x100 );
  libspectrum_snap_set_de_    ( snap, buffer[14] + buffer[15] * 0x100 );
  libspectrum_snap_set_hl_    ( snap, buffer[16] + buffer[17] * 0x100 );
  libspectrum_snap_set_f_     ( snap, buffer[18] );
  libspectrum_snap_set_a_     ( snap, buffer[19] );
  libspectrum_snap_set_r      ( snap, buffer[20] );
  libspectrum_snap_set_i      ( snap, buffer[21] );
  libspectrum_snap_set_sp     ( snap, buffer[22] + buffer[23] * 0x100 );
  libspectrum_snap_set_pc     ( snap, buffer[24] + buffer[25] * 0x100 );
  /* Bytes 32 and 33 not used */
  libspectrum_snap_set_out_ula( snap, buffer[28] );
  /* Byte 35 not used */

  buffer += 30;

  flags = libspectrum_read_word( &buffer );

  libspectrum_snap_set_iff1   ( snap,   flags & 0x01 );
  libspectrum_snap_set_iff2   ( snap, ( flags & 0x04 ) >> 2 );

  /* bit 3 1 | IM
     --------+---
         0 0 |  1
         0 1 |  2 
         1 - |  0 */

  libspectrum_snap_set_im( snap,
			   ( flags & 0x08 ? 0 : ( flags & 0x02 ? 2 : 1 ) ) );

  /* Get me 48K of zero-ed memory and then copy in the bits that were
     represented in the snap */
  memory = libspectrum_new0( libspectrum_byte, 0xc000 );

  memcpy( &memory[ start ], buffer, memory_length );

  error = libspectrum_split_to_48k_pages( snap, memory );
  if( error ) { libspectrum_free( memory ); return error; }

  libspectrum_free( memory );

  return LIBSPECTRUM_ERROR_NONE;
}
