/* scorpion.h: Scorpion 256K specific routines
   Copyright (c) 1999-2004 Philip Kendall and Fredrick Meunier
   Copyright (c) 2004 Stuart Brady

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

#ifndef FUSE_SCORPION_H
#define FUSE_SCORPION_H

#include "machine.h"

int scorpion_init( fuse_machine_info *machine );
void scorpion_memoryport2_write( libspectrum_word port, libspectrum_byte b );

#endif			/* #ifndef FUSE_SCORPION_H */
