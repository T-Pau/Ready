/* machines_periph.h: various machine-specific peripherals
   Copyright (c) 2011-2016 Philip Kendall

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

#ifndef FUSE_MACHINES_PERIPH_H
#define FUSE_MACHINES_PERIPH_H

void machines_periph_register_startup( void );

void machines_periph_48( void );
void machines_periph_128( void );
void machines_periph_plus3( void );
void machines_periph_timex( void );
void machines_periph_pentagon( void );

#endif  /* #ifndef FUSE_MACHINES_PERIPH_H */
