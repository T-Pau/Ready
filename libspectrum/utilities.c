/* utilities.c: miscellaneous utility routines
   Copyright (c) 2011 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include "internals.h"

#define TZX_HZ 3500000

static const libspectrum_dword tstates_per_ms = TZX_HZ / 1000;

libspectrum_dword 
libspectrum_ms_to_tstates( libspectrum_dword ms )
{
  return ms * tstates_per_ms;
}

libspectrum_dword 
libspectrum_tstates_to_ms( libspectrum_dword tstates )
{
  return tstates / tstates_per_ms;
}

void
libspectrum_set_pause_ms( libspectrum_tape_block *block,
                          libspectrum_dword pause_ms )
{
  libspectrum_tape_block_set_pause( block, pause_ms );
  libspectrum_tape_block_set_pause_tstates( block,
                                        libspectrum_ms_to_tstates( pause_ms ) );
}

void
libspectrum_set_pause_tstates( libspectrum_tape_block *block,
                               libspectrum_dword pause_tstates )
{
  libspectrum_tape_block_set_pause_tstates( block, pause_tstates );
  libspectrum_tape_block_set_pause( block,
                                  libspectrum_tstates_to_ms( pause_tstates ) );
}

size_t
libspectrum_bits_to_bytes( size_t bits )
{
  return ( bits + LIBSPECTRUM_BITS_IN_BYTE - 1 ) / LIBSPECTRUM_BITS_IN_BYTE;
}
