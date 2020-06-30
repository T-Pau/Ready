/* rectangle.h: routines for managing the set of screen area rectangles updated
                since the last display
   Copyright (c) 1999-2009 Philip Kendall, Thomas Harte, Witold Filipczyk
                           and Fredrick Meunier

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

#ifndef FUSE_RECTANGLE_H
#define FUSE_RECTANGLE_H

/* Used for grouping screen writes together */
struct rectangle { int x,y; int w,h; };

/* Those rectangles which weren't modified on the last line to be displayed */
extern struct rectangle *rectangle_inactive;
extern size_t rectangle_inactive_count, rectangle_inactive_allocated;

void rectangle_add( int y, int x, int w );
void rectangle_end_line( int y );

#endif				/* #ifndef FUSE_RECTANGLE_H */
