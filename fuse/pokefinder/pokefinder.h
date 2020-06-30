/* pokefinder.h: help with finding pokes
   Copyright (c) 2003-2004 Philip Kendall

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

#ifndef FUSE_POKEFINDER_H
#define FUSE_POKEFINDER_H

#include <libspectrum.h>

extern libspectrum_byte pokefinder_possible[][ MEMORY_PAGE_SIZE ];
extern libspectrum_byte pokefinder_impossible[][ MEMORY_PAGE_SIZE / 8 ];
extern size_t pokefinder_count;

void pokefinder_clear( void );
int pokefinder_search( libspectrum_byte value );
int pokefinder_incremented( void );
int pokefinder_decremented( void );

#endif				/* #ifndef FUSE_POKEFINDER_H */
