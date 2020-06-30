/* specplus3.h: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2013 Philip Kendall
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

#ifndef FUSE_SPECPLUS3_H
#define FUSE_SPECPLUS3_H

#include <libspectrum.h>

#include "machine.h"
#include "periph.h"
#include "peripherals/disk/fdd.h"

int specplus3_port_from_ula( libspectrum_word port );

int specplus3_init( fuse_machine_info *machine );
void specplus3_765_update_fdd( void );
void specplus3_765_init( void );
void specplus3_765_reset( void );

int specplus3_plus2a_common_reset( void );
void specplus3_fdc_reset( void );
void specplus3_menu_items( void );
int specplus3_shutdown( void );

void specplus3_memoryport_write( libspectrum_word port, libspectrum_byte b );
void specplus3_memoryport2_write( libspectrum_word port, libspectrum_byte b );
void specplus3_memoryport2_write_internal( libspectrum_word port,
                                           libspectrum_byte b );

libspectrum_byte specplus3_fdc_status( libspectrum_word port, libspectrum_byte *attached );
libspectrum_byte specplus3_fdc_read( libspectrum_word port, libspectrum_byte *attached );
void specplus3_fdc_write( libspectrum_word port, libspectrum_byte data );

int specplus3_memory_map( void );

typedef enum specplus3_drive_number {
  SPECPLUS3_DRIVE_A = 0,	/* First drive must be number zero */
  SPECPLUS3_DRIVE_B,
  SPECPLUS3_NUM_DRIVES,
} specplus3_drive_number;

int specplus3_disk_insert( specplus3_drive_number which, const char *filename,
                           int autoload );
fdd_t *specplus3_get_fdd( specplus3_drive_number which );

#endif			/* #ifndef FUSE_SPECPLUS3_H */
