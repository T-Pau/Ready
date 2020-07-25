/* tc2068.h: Timex TC2068 specific routines
   Copyright (c) 2004,2015 Fredrick Meunier
   Copyright (c) 2015 Stuart Brady

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

#ifndef FUSE_TS2068_H
#define FUSE_TS2068_H

#include <libspectrum.h>

#include "machine.h"

int tc2068_init( fuse_machine_info *machine );
void tc2068_tc2048_common_display_setup( void );
void tc2068_tc2048_common_reset( void );

libspectrum_byte tc2068_ay_registerport_read( libspectrum_word port,
                                              libspectrum_byte *attached );
libspectrum_byte tc2068_ay_dataport_read( libspectrum_word port,
                                          libspectrum_byte *attached );

int tc2068_memory_map( void );

extern memory_page tc2068_empty_mapping[MEMORY_PAGES_IN_8K];

#endif			/* #ifndef FUSE_TS2068_H */
