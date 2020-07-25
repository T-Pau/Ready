/* loader.h: loader detection
   Copyright (c) 2006 Philip Kendall

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

#ifndef FUSE_LOADER_H
#define FUSE_LOADER_H

#include <libspectrum.h>

void loader_frame( libspectrum_dword frame_length );
void loader_tape_play( void );
void loader_tape_stop( void );
void loader_detect_loader( void );
void loader_set_acceleration_flags( int flags, int from_acceleration );

#endif			/* #ifndef FUSE_LOADER_H */
