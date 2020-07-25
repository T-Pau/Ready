/* relative_paths.c: Path-related compatibility routines
   Copyright (c) 1999-2012 Philip Kendall

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
#include <unistd.h>

#include "fuse.h"
#include "ui/ui.h"

void
get_relative_directory( char *buffer, size_t bufsize )
{
  ssize_t retval = readlink( "/proc/self/exe", buffer, bufsize - 1 );
  if( retval < 0 ) {
    ui_error( UI_ERROR_ERROR, "error getting current working directory: %s",
              strerror( -errno ) );
    fuse_abort();
  }
  buffer[ retval ] = '\0';
}
