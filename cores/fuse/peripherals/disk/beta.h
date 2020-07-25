/* beta.h: Routines for handling the Beta disk interface
   Copyright (c) 2003-2016 Fredrick Meunier, Philip Kendall
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

     Fred: fredm@spamcop.net

*/

#ifndef FUSE_BETA_H
#define FUSE_BETA_H

#include <libspectrum.h>

#include "memory_pages.h"
#include "fdd.h"

extern int beta_available;  /* Is the Beta disk interface available for use? */
extern int beta_active;     /* Is the Beta disk interface enabled? */
extern int beta_builtin;    /* Is the Beta disk interface built-in? */

/* A 16KB memory chunk accessible by the Z80 when /ROMCS is low */
extern memory_page beta_memory_map_romcs[MEMORY_PAGES_IN_16K];

extern libspectrum_word beta_pc_mask; /* Bits to mask in PC for enable check */
extern libspectrum_word beta_pc_value; /* Value to compare masked PC against */

void beta_register_startup( void );

void beta_page( void );
void beta_unpage( void );

void beta_cr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte beta_sr_read( libspectrum_word port, libspectrum_byte *attached );

libspectrum_byte beta_tr_read( libspectrum_word port, libspectrum_byte *attached );
void beta_tr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte beta_sec_read( libspectrum_word port, libspectrum_byte *attached );
void beta_sec_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte beta_dr_read( libspectrum_word port, libspectrum_byte *attached );
void beta_dr_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte beta_sp_read( libspectrum_word port, libspectrum_byte *attached );
void beta_sp_write( libspectrum_word port, libspectrum_byte b );

typedef enum beta_drive_number {
  BETA_DRIVE_A = 0,
  BETA_DRIVE_B,
  BETA_DRIVE_C,
  BETA_DRIVE_D,
  BETA_NUM_DRIVES,
} beta_drive_number;

int beta_disk_insert( beta_drive_number which, const char *filename,
                       int autoload );
int beta_disk_eject( beta_drive_number which );
int beta_disk_save( beta_drive_number which, int saveas );
int beta_disk_flip( beta_drive_number which, int flip );
int beta_disk_writeprotect( beta_drive_number which, int wrprot );
int beta_disk_write( beta_drive_number which, const char *filename );
fdd_t *beta_get_fdd( beta_drive_number which );

int beta_unittest( void );

#endif                  /* #ifndef FUSE_BETA_H */
