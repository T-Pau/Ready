/* ay.h: AY-3-8912 routines
   Copyright (c) 1999-2016 Philip Kendall
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

#ifndef FUSE_AY_H
#define FUSE_AY_H

#include <libspectrum.h>

#define AY_REGISTERS 16

typedef struct ayinfo {
  int current_register;
  libspectrum_byte registers[ AY_REGISTERS ];
} ayinfo;

void ay_register_startup( void );

libspectrum_byte ay_registerport_read( libspectrum_word port, libspectrum_byte *attached );
void ay_registerport_write( libspectrum_word port, libspectrum_byte b );

void ay_dataport_write( libspectrum_word port, libspectrum_byte b );

void ay_state_from_snapshot( libspectrum_snap *snap );

#endif			/* #ifndef FUSE_AY_H */
