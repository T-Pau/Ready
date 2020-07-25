/* file.c: File-related compatibility routines
   Copyright (c) 2008 Philip Kendall

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

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "compat.h"
#include "utils.h"
#include "ui/ui.h"

const compat_fd COMPAT_FILE_OPEN_FAILED = NULL;

compat_fd
compat_file_open( const char *path, int write )
{
  return fopen( path, write ? "wb" : "rb" );
}

off_t
compat_file_get_length( compat_fd fd )
{
  struct stat file_info;

  if( fstat( fileno( fd ), &file_info ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't stat file: %s", strerror( errno ) );
    return -1;
  }

  return file_info.st_size;
}

int
compat_file_read( compat_fd fd, utils_file *file )
{
  size_t bytes = fread( file->buffer, 1, file->length, fd );
  if( bytes != file->length ) {
    ui_error( UI_ERROR_ERROR,
              "error reading file: expected %lu bytes, but read only %lu",
              (unsigned long)file->length, (unsigned long)bytes );
    return 1;
  }

  return 0;
}

int
compat_file_write( compat_fd fd, const unsigned char *buffer, size_t length )
{
  size_t bytes = fwrite( buffer, 1, length, fd );
  if( bytes != length ) {
    ui_error( UI_ERROR_ERROR,
              "error writing file: expected %lu bytes, but wrote only %lu",
              (unsigned long)length, (unsigned long)bytes );
    return 1;
  }

  return 0;
}

int
compat_file_close( compat_fd fd )
{
  return fclose( fd );
}

int
compat_file_exists( const char *path )
{
  return ( access( path, R_OK ) != -1 );
}
