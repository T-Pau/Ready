/* trdos.c: Routines for handling the TR-DOS filesystem
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

#include <config.h>

#include <string.h>
#include "trdos.h"

int
trdos_read_spec( trdos_spec_t *spec, const libspectrum_byte *src )
{
  if( *src ) return -1;

  spec->first_free_sector   = src[225];
  spec->first_free_track    = src[226];
  spec->disk_type           = src[227];
  spec->file_count          = src[228];
  spec->free_sectors        = src[229] + src[230] * 0x100;
  spec->id                  = src[231];
  if( spec->id != 16 ) return -1;

  memcpy( spec->password, src + 234, 9 );
  spec->deleted_files       = src[244];
  memcpy( spec->disk_label, src + 245, 8 );

  return 0;
}

void
trdos_write_spec( libspectrum_byte *dest, const trdos_spec_t *spec )
{
  memset( dest, 0, 256 );
  dest[225] = spec->first_free_sector;
  dest[226] = spec->first_free_track;
  dest[227] = spec->disk_type;
  dest[228] = spec->file_count;
  dest[229] = spec->free_sectors & 0xff;
  dest[230] = spec->free_sectors >> 8;
  dest[231] = spec->id;
  memcpy( dest + 234, spec->password, 9 );
  dest[244] = spec->deleted_files;
  memcpy( dest + 245, spec->disk_label, 8 );
}

int
trdos_read_dirent( trdos_dirent_t *entry, const libspectrum_byte *src )
{
  memcpy( entry->filename, src, 8 );
  entry->file_extension = src[8];
  entry->param1         = src[9]  + src[10] * 0x100;
  entry->param2         = src[11] + src[12] * 0x100;
  entry->file_length    = src[13];
  entry->start_sector   = src[14];
  entry->start_track    = src[15];

  return entry->filename[0]? 0 : 1;
}

void
trdos_write_dirent( libspectrum_byte *dest, const trdos_dirent_t *entry )
{
  memcpy( dest, entry->filename, 8 );
  dest[8]  = entry->file_extension;
  dest[9]  = entry->param1 & 0xff;
  dest[10] = entry->param1 >> 8;
  dest[11] = entry->param2 & 0xff;
  dest[12] = entry->param2 >> 8;
  dest[13] = entry->file_length;
  dest[14] = entry->start_sector;
  dest[15] = entry->start_track;
}

int
trdos_read_fat( trdos_boot_info_t *info, const libspectrum_byte *sectors,
                unsigned int seclen )
{
  int i, j, error;
  trdos_dirent_t entry;
  const libspectrum_byte *sector;

  info->have_boot_file = 0;
  info->basic_files_count = 0;

  /* FAT sectors */
  for( i = 0; i < 8; i++ ) {
    sector = sectors + i * seclen * 2;    /* interleaved */

    /* Note: some TR-DOS versions like 5.04T have a turbo format with 
       sequential sectors: 1, 2, 3, ..., 8, 9, 10, ...
       The SCL/TRD image formats can't specify a format mode and Fuse
       load the sectors as interleaved: 1, 9, 2, 10, 3, ...
    */

    /* FAT entries */
    for( j = 0; j < 16; j++ ) {
      error = trdos_read_dirent( &entry, sector + j * 16 );
      if( error ) return 0;

      /* Basic files */
      if( entry.filename[0] > 0x01 &&
          entry.file_extension == 'B' ) {

        /* Boot file */
        if( !info->have_boot_file &&
            !strncmp( (const char *)entry.filename, "boot    ", 8 ) ) {
          info->have_boot_file = 1;
        }

        /* First basic program */
        if( info->basic_files_count == 0 ) {
          memcpy( info->first_basic_file, entry.filename, 8 );
        }

        info->basic_files_count++;
      }
    }
  }

  return 0;
}
