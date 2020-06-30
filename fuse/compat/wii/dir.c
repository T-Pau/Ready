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

#include "compat.h"

compat_dir
compat_opendir( const char *path )
{
  return diropen( path );
}

compat_dir_result_t
compat_readdir( compat_dir directory, char *path, size_t length )
{
  struct stat fstat;

  int done = dirnext( directory, path, &fstat );

  return done ? COMPAT_DIR_RESULT_END : COMPAT_DIR_RESULT_OK;
}

int
compat_closedir( compat_dir directory )
{
  return dirclose( directory );
}
