/* trdos.h: Routines for handling the TR-DOS filesystem
   Copyright (c) 2016 Sergio Baldoví

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

   E-mail: serbalgi@gmail.com

*/

#ifndef FUSE_TRDOS_H
#define FUSE_TRDOS_H

#include <libspectrum.h>

typedef struct trdos_spec_t {
  libspectrum_byte first_free_sector; /* 0 to 15 */
  libspectrum_byte first_free_track;  /* 0 to ? */
  libspectrum_byte disk_type;
  libspectrum_byte file_count;
  libspectrum_word free_sectors;
  libspectrum_byte id;
  char password[9];                   /* not null-terminated */
  libspectrum_byte deleted_files;
  char disk_label[8];                 /* not null-terminated */
} trdos_spec_t;

typedef struct trdos_dirent_t {
  char filename[8];                   /* not null-terminated */
  char file_extension;
  libspectrum_word param1;
  libspectrum_word param2;
  libspectrum_byte file_length;       /* in sectors */
  libspectrum_byte start_sector;      /* 0 to 15 */
  libspectrum_byte start_track;       /* 0 to ? */
} trdos_dirent_t;

typedef struct trdos_boot_info_t {
  int have_boot_file;
  int basic_files_count;
  char first_basic_file[8];           /* not null-terminated */
} trdos_boot_info_t;

int
trdos_read_spec( trdos_spec_t *spec, const libspectrum_byte *src );

void
trdos_write_spec( libspectrum_byte *dest, const trdos_spec_t *spec );

int
trdos_read_dirent( trdos_dirent_t *entry, const libspectrum_byte *src );

void
trdos_write_dirent( libspectrum_byte *dest, const trdos_dirent_t *entry );

int
trdos_read_fat( trdos_boot_info_t *info, const libspectrum_byte *sectors,
                unsigned int seclen );

#endif                  /* #ifndef FUSE_TRDOS_H */
