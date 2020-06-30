/* dir.c: Directory-related compatibility routines
   Copyright (c) 2009 Philip Kendall

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
#include <string.h>

#include "compat.h"

compat_dir
compat_opendir( const char *path )
{
  return opendir( path );
}

compat_dir_result_t
compat_readdir( compat_dir directory, char *name, size_t length )
{
  compat_dir_result_t r;
  struct dirent *dirent;

  errno = 0;
  dirent = readdir( directory );

  if( dirent ) {
    r = COMPAT_DIR_RESULT_OK;
    strncpy( name, dirent->d_name, length );
    name[ length - 1 ] = 0;
  } else {
    r = ( errno == 0 ? COMPAT_DIR_RESULT_END : COMPAT_DIR_RESULT_ERROR );
  }

  return r;
}

int
compat_closedir( compat_dir directory )
{
  return closedir( directory );
}
