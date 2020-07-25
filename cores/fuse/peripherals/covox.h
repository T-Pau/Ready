/* covox.h: Routines for handling the Covox
   Copyright (c) 2011-2017 Jon Mitchell, Philip Kendall, Fredrick Meunier

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

#ifndef FUSE_COVOX_H
#define FUSE_COVOX_H

#include <libspectrum.h>

typedef struct covox_info {
  libspectrum_byte covox_dac; /* Current byte in the Covox 8bit DAC */
} covox_info;

void covox_register_startup( void );
void covox_write( libspectrum_word port, libspectrum_byte val );

#endif                          /* #ifndef FUSE_COVOX_H */
