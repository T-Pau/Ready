/* disciple.h: Routines for handling the DISCiPLE interface
   Copyright (c) 2005-2015 Stuart Brady

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

#ifndef FUSE_DISCIPLE_H
#define FUSE_DISCIPLE_H

typedef enum disciple_drive_number {
  DISCIPLE_DRIVE_1 = 0,
  DISCIPLE_DRIVE_2,
  DISCIPLE_NUM_DRIVES,
} disciple_drive_number;

#include <libspectrum.h>

#include "fdd.h"

extern int disciple_available;  /* Is the DISCiPLE available for use? */
extern int disciple_active;     /* DISCiPLE enabled? */

void disciple_register_startup( void );

void disciple_page( void );
void disciple_unpage( void );

int disciple_disk_insert( disciple_drive_number which, const char *filename,
                          int autoload );
int disciple_disk_eject( disciple_drive_number which );
int disciple_disk_save( disciple_drive_number which, int saveas );
int disciple_disk_write( disciple_drive_number which, const char *filename );
int disciple_disk_flip( disciple_drive_number which, int flip );
int disciple_disk_writeprotect( disciple_drive_number which, int wrprot );
fdd_t *disciple_get_fdd( disciple_drive_number which );

int disciple_unittest( void );

#endif                  /* #ifndef FUSE_DISCIPLE_H */
