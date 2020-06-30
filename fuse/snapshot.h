/* snapshot.h: snapshot handling routines
   Copyright (c) 1999,2001-2002 Philip Kendall

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

#ifndef FUSE_SNAPSHOT_H
#define FUSE_SNAPSHOT_H

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include <libspectrum.h>
#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

int snapshot_read( const char *filename );
int snapshot_read_buffer( const unsigned char *buffer, size_t length,
			  libspectrum_id_t type );

int snapshot_copy_from( libspectrum_snap *snap );

int snapshot_write( const char *filename );
int snapshot_copy_to( libspectrum_snap *snap );

#endif
