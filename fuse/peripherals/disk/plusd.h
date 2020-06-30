/* plusd.h: Routines for handling the +D interface
   Copyright (c) 2005-2016 Stuart Brady, Philip Kendall

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

#ifndef FUSE_PLUSD_H
#define FUSE_PLUSD_H

typedef enum plusd_drive_number {
  PLUSD_DRIVE_1 = 0,
  PLUSD_DRIVE_2,
  PLUSD_NUM_DRIVES,
} plusd_drive_number;

#include <libspectrum.h>

#include "fdd.h"

extern int plusd_available;  /* Is the +D available for use? */
extern int plusd_active;     /* +D enabled? */

void plusd_register_startup( void );

void plusd_page( void );
void plusd_unpage( void );

int plusd_disk_insert( plusd_drive_number which, const char *filename,
		       int autoload );
int plusd_disk_eject( plusd_drive_number which );
int plusd_disk_save( plusd_drive_number which, int saveas );
int plusd_disk_write( plusd_drive_number which, const char *filename );
int plusd_disk_flip( plusd_drive_number which, int flip );
int plusd_disk_writeprotect( plusd_drive_number which, int wrprot );
fdd_t *plusd_get_fdd( plusd_drive_number which );

int plusd_unittest( void );

#endif                  /* #ifndef FUSE_PLUSD_H */
