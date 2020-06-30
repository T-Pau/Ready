/* spec128.h: Spectrum 128K specific routines
   Copyright (c) 1999-2004 Philip Kendall

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

#ifndef FUSE_SPEC128_H
#define FUSE_SPEC128_H

#include <libspectrum.h>

#include "machine.h"

int spec128_init( fuse_machine_info *machine );
int spec128_common_reset( int contention );

void spec128_memoryport_write( libspectrum_word port, libspectrum_byte b );
void spec128_select_rom( int rom );
void spec128_select_page( int page );

int spec128_memory_map( void );

#endif			/* #ifndef FUSE_SPEC128_H */
