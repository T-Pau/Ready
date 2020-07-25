/* specdrum.h: Routines for handling the Specdrum Drum Kit
   Copyright (c) 2011-2016 Jon Mitchell, Philip Kendall

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

   Philip: philip-fuse@shadowmagic.org.uk

   Jon: ooblick@gmail.com

*/

#ifndef FUSE_SPECDRUM_H
#define FUSE_SPECDRUM_H

#include <libspectrum.h>

typedef struct specdrum_info {
  libspectrum_signed_byte specdrum_dac; /* Current byte in the SpecDrum 8bit DAC */
} specdrum_info;

void specdrum_register_startup( void );
void specdrum_write( libspectrum_word port, libspectrum_byte val );

#endif                          /* #ifndef FUSE_SPECDRUM_H */
