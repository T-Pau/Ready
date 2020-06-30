/* spec48.h: Spectrum 48K specific routines
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

#ifndef FUSE_SPEC48_H
#define FUSE_SPEC48_H

#include <libspectrum.h>

#include "machine.h"

int spec48_port_from_ula( libspectrum_word port );

int spec48_init( fuse_machine_info *machine );
void spec48_common_display_setup( void );
int spec48_common_reset( void );
int spec48_memory_map( void );

#endif			/* #ifndef FUSE_SPEC48_H */
