/* opus.h: Routines for handling the Opus Discovery interface
   Copyright (c) 2005-2013 Stuart Brady, Fredrick Meunier

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

   Stuart: stuart.brady@gmail.com

*/

#ifndef FUSE_OPUS_H
#define FUSE_OPUS_H

#include <libspectrum.h>

#include "fdd.h"

typedef enum opus_drive_number {
  OPUS_DRIVE_1 = 0,
  OPUS_DRIVE_2,
  OPUS_NUM_DRIVES,
} opus_drive_number;

extern int opus_available;  /* Is the Opus available for use? */
extern int opus_active;     /* Opus enabled? */

void opus_register_startup( void );

void opus_page( void );
void opus_unpage( void );

libspectrum_byte opus_read( libspectrum_word address );
void opus_write( libspectrum_word address, libspectrum_byte b );

int opus_disk_insert( opus_drive_number which, const char *filename,
		       int autoload );
int opus_disk_eject( opus_drive_number which );
int opus_disk_save( opus_drive_number which, int saveas );
int opus_disk_write( opus_drive_number which, const char *filename );
int opus_disk_flip( opus_drive_number which, int flip );
int opus_disk_writeprotect( opus_drive_number which, int wrprot );
fdd_t *opus_get_fdd( opus_drive_number which );

int opus_unittest( void );

#endif                  /* #ifndef FUSE_OPUS_H */
