/* snp.c: Routines for handling .snp snapshots
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

#include "internals.h"

#define SNP_TRAILER_LENGTH 31

libspectrum_error
libspectrum_snp_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_error error;

  /* Length must be at least 48K of RAM plus the 'trailer' */
  if( length < 0xc000 + SNP_TRAILER_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_snp_read: not enough bytes for a .snp file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* All .snp snaps are of the 48K machine */
  libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );

  /* Get the RAM */
  error = libspectrum_split_to_48k_pages( snap, buffer );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  buffer += 0xc000;

  libspectrum_snap_set_f      ( snap, buffer[ 0] );
  libspectrum_snap_set_a      ( snap, buffer[ 1] );
  libspectrum_snap_set_out_ula( snap, buffer[ 2] );
  /* Byte 3 not used */
  libspectrum_snap_set_bc     ( snap, buffer[ 4] + buffer[ 5] * 0x100 );
  libspectrum_snap_set_de     ( snap, buffer[ 6] + buffer[ 7] * 0x100 );
  libspectrum_snap_set_hl     ( snap, buffer[ 8] + buffer[ 9] * 0x100 );
  libspectrum_snap_set_pc     ( snap, buffer[10] + buffer[11] * 0x100 );
  libspectrum_snap_set_sp     ( snap, buffer[12] + buffer[13] * 0x100 );
  libspectrum_snap_set_ix     ( snap, buffer[14] + buffer[15] * 0x100 );
  libspectrum_snap_set_iy     ( snap, buffer[16] + buffer[17] * 0x100 );
  libspectrum_snap_set_iff1   ( snap, buffer[18] );
  libspectrum_snap_set_iff2   ( snap, buffer[19] );
  libspectrum_snap_set_im     ( snap, buffer[20] );
  libspectrum_snap_set_r      ( snap, buffer[21] );
  libspectrum_snap_set_i      ( snap, buffer[22] );
  libspectrum_snap_set_f_     ( snap, buffer[23] );
  libspectrum_snap_set_a_     ( snap, buffer[24] );
  libspectrum_snap_set_bc_    ( snap, buffer[25] + buffer[26] * 0x100 );
  libspectrum_snap_set_de_    ( snap, buffer[27] + buffer[28] * 0x100 );
  libspectrum_snap_set_hl_    ( snap, buffer[29] + buffer[30] * 0x100 );

  return LIBSPECTRUM_ERROR_NONE;
}
