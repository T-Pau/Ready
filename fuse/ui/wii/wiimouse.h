/* wiimouse.h: routines for dealing with the Wiimote as a mouse
   Copyright (c) 2008 Bjoern Giesler

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

   E-mail: bjoern@giesler.de

*/

#ifndef FUSE_WIIMOUSE_H
#define FUSE_WIIMOUSE_H

int wiimouse_init( void );
int wiimouse_end( void );
void wiimouse_get_position( int *x, int *y );

void mouse_update( void );

#endif /* FUSE_WIIMOUSE_H */
