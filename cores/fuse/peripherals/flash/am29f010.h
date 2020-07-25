/* am29f010.c 1Mbit flash chip emulation
   
   Emulates the AMD 29F010 flash chip

   Copyright (c) 2011 Guesser, Philip Kendall
   
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
 
*/

#ifndef FUSE_AM29F010_H
#define FUSE_AM29F010_H

#include <libspectrum.h>

typedef struct flash_am29f010_t flash_am29f010_t;

flash_am29f010_t* flash_am29f010_alloc( void );
void flash_am29f010_free( flash_am29f010_t *self );
void flash_am29f010_init( flash_am29f010_t *self, libspectrum_byte *memory );

void flash_am29f010_write( flash_am29f010_t *self, libspectrum_byte page, libspectrum_word address, libspectrum_byte b );

#endif                          /* #ifndef FUSE_AM29F010_H */
