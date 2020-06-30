/* w5100.c: Wiznet W5100 emulation
   
   Emulates a minimal subset of the Wiznet W5100 TCP/IP controller.

   Copyright (c) 2011 Philip Kendall
   
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

#ifndef FUSE_W5100_H
#define FUSE_W5100_H

#include <libspectrum.h>

typedef struct nic_w5100_t nic_w5100_t;

nic_w5100_t* nic_w5100_alloc( void );
void nic_w5100_free( nic_w5100_t *self );

void nic_w5100_reset( nic_w5100_t *self );

libspectrum_byte nic_w5100_read( nic_w5100_t *self, libspectrum_word reg);
void nic_w5100_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b );

void nic_w5100_from_snapshot( nic_w5100_t *self, libspectrum_byte *data );
libspectrum_byte* nic_w5100_to_snapshot( nic_w5100_t *self );

#endif                          /* #ifndef FUSE_W5100_H */
