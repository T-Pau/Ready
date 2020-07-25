/* machines.h: machine specific routines
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

#ifndef FUSE_MACHINES_H
#define FUSE_MACHINES_H

#include <libspectrum.h>

#include "machine.h"

int pentagon_init( fuse_machine_info *machine );
int pentagon_shutdown( void );
int pentagon512_init( fuse_machine_info *machine );
int pentagon1024_init( fuse_machine_info *machine );

int spec16_init( fuse_machine_info *machine );
int spec48_ntsc_init( fuse_machine_info *machine );
int spec_se_init( fuse_machine_info *machine );
int specplus2_init( fuse_machine_info *machine );
int specplus2a_init( fuse_machine_info *machine );
int specplus3e_init( fuse_machine_info *machine );

int tc2048_init( fuse_machine_info *machine );
int tc2048_port_from_ula( libspectrum_word port );
int ts2068_init( fuse_machine_info *machine );

#endif			/* #ifndef FUSE_MACHINES_H */
