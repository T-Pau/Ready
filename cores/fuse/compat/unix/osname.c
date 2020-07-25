/* osname.c: Get a representation of the OS we're running on
   Copyright (c) 1999-2007 Philip Kendall

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
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "ui/ui.h"

int compat_osname( char *buffer, size_t length )
{
  struct utsname osname;
  int error;

  error = uname( &osname );
  if( error < 0 ) {
    ui_error( UI_ERROR_ERROR, "error getting system information: %s",
	      strerror( errno ) );
    return 1;
  }

  snprintf( buffer, length, "%s %s %s", osname.sysname, osname.machine,
	    osname.release );
  return 0;
}
