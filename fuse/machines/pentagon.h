/* pentagon.h: Pentagon specific routines
   Copyright (c) 1999-2007 Philip Kendall and Fredrick Meunier
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

#ifndef FUSE_PENTAGON_H
#define FUSE_PENTAGON_H

#include <libspectrum.h>

#include "machine.h"

libspectrum_byte pentagon_select_1f_read( libspectrum_word port,
					  libspectrum_byte *attached );
libspectrum_byte pentagon_select_ff_read( libspectrum_word port,
					  libspectrum_byte *attached );
int pentagon_port_from_ula( libspectrum_word port );
int pentagon_port_contended( libspectrum_word port );

void pentagon1024_memoryport_write( libspectrum_word port, libspectrum_byte b );
void pentagon1024_v22_memoryport_write( libspectrum_word port, libspectrum_byte b );

#endif			/* #ifndef FUSE_PENTAGON_H */
