/* divide.h: DivIDE interface routines
   Copyright (c) 2005-2016 Matthew Westcott, Philip Kendall

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

#ifndef FUSE_DIVIDE_H
#define FUSE_DIVIDE_H

#include <libspectrum.h>

/* Whether DivIDE is currently paged in */
extern int divide_active;

/* Notify DivIDE hardware of an opcode fetch to one of the designated
   entry / exit points. Depending on configuration, it may or may not
   result in the DivIDE memory being paged in */
void divide_set_automap( int state );

/* Call this after some state change other than an opcode fetch which could
   trigger DivIDE paging (such as updating the write-protect flag), to
   re-evaluate whether paging will actually happen */
void divide_refresh_page_state( void );

void divide_register_startup( void );
int divide_insert( const char *filename, libspectrum_ide_unit unit );
int divide_commit( libspectrum_ide_unit unit );
int divide_eject( libspectrum_ide_unit unit );

int divide_unittest( void );

#endif			/* #ifndef FUSE_DIVIDE_H */
