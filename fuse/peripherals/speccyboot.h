/* speccyboot.h: SpeccyBoot Ethernet emulation

   Emulates SPI communication and (a minimal subset of) the
   functionality of the Microchip ENC28J60 Ethernet controller. Refer
   to the ENC28J60 data sheet and SpeccyBoot documentation for
   details.

   ENC28J60 data sheet:
     http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en022889

   SpeccyBoot:
     http://patrikpersson.github.io/speccyboot/
  
   Copyright (c) 2009-2016 Patrik Persson, Philip Kendall

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

#ifndef FUSE_SPECCYBOOT_H
#define FUSE_SPECCYBOOT_H

void speccyboot_register_startup( void );

int speccyboot_unittest( void );

#endif /* #ifndef FUSE_SPECCYBOOT_H */
