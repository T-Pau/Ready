/* osname.c: Get a representation of the OS we're running on
   Copyright (c) 1999-2007 Philip Kendall
   Copyright (c) 2015 Sergio Baldoví

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

#include <windows.h>

#include "ui/ui.h"

int compat_osname( char *osname, size_t length )
{
  OSVERSIONINFO buf;
  const char *windows_name;
  int error;

  buf.dwOSVersionInfoSize = sizeof( buf );
  error = GetVersionEx( &buf );
  if( error == 0 ) {
    ui_error( UI_ERROR_ERROR, "error getting system information." );
    return 1;
  }

  switch( buf.dwPlatformId ) {
  case VER_PLATFORM_WIN32_NT:	   windows_name = "NT";      break;
  case VER_PLATFORM_WIN32_WINDOWS: windows_name = "95/98";   break;
  case VER_PLATFORM_WIN32s:	   windows_name = "3.1";     break;
  default:			   windows_name = "unknown"; break;
  }

  /* FIXME: verify function below is unicode compliant */
  /* The casts to int work around a suspected Wine (or MinGW) bug */
  snprintf( osname, length, "Windows %s %i.%i build %i %s",
	    windows_name, (int)buf.dwMajorVersion, (int)buf.dwMinorVersion,
	    (int)buf.dwBuildNumber, buf.szCSDVersion );

  return 0;
}
