/* paths.c: Path-related compatibility routines
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

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif				/* #ifdef HAVE_LIBGEN_H */
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "compat.h"
#include "fuse.h"
#include "ui/ui.h"

const char*
compat_get_temp_path( void )
{
  const char *dir;

  /* Something close to this algorithm specified at
     http://msdn.microsoft.com/en-us/library/windows/desktop/aa364992%28v=vs.85%29.aspx
  */
  dir = getenv( "TMP" ); if( dir ) return dir;
  dir = getenv( "TEMP" ); if( dir ) return dir;
  return ".";
}

const char*
compat_get_config_path( void )
{
  const char *dir;
  dir = getenv( "USERPROFILE" ); if( dir ) return dir;
  dir = getenv( "WINDIR" ); if( dir ) return dir;
  return ".";
}

int
compat_is_absolute_path( const char *path )
{
  if( path[0] == '\\' ) return 1;
  if( path[0] && path[1] == ':' ) return 1;
  return 0;
}

int
compat_get_next_path( path_context *ctx )
{
  char buffer[ PATH_MAX ];
  const char *path_segment, *path2;

  switch( ctx->type ) {
  case UTILS_AUXILIARY_LIB: path_segment = "lib"; break;
  case UTILS_AUXILIARY_ROM: path_segment = "roms"; break;
  case UTILS_AUXILIARY_WIDGET: path_segment = "ui/widget"; break;
  case UTILS_AUXILIARY_GTK: path_segment = "ui/gtk"; break;
  default:
    ui_error( UI_ERROR_ERROR, "unknown auxiliary file type %d", ctx->type );
    return 0;
  }

  switch( (ctx->state)++ ) {

    /* First look relative to the Fuse executable */
  case 0:
    if( compat_is_absolute_path( fuse_progname ) ) {
      strncpy( buffer, fuse_progname, PATH_MAX );
      buffer[ PATH_MAX - 1 ] = '\0';
    } else {
      DWORD retval; 
      retval = GetModuleFileName( NULL, buffer, PATH_MAX );
      if( !retval ) return 0;
    }

    path2 = dirname( buffer );
    snprintf( ctx->path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s", path2,
              path_segment );
    return 1;

    /* Then relative to %APPDATA%/Fuse directory */
  case 1:
    path2 = getenv( "APPDATA" );
    if( !path2 ) return 0;
    snprintf( ctx->path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "Fuse" 
              FUSE_DIR_SEP_STR "%s", path2, path_segment );

    return 1;

    /* Then relative to the current directory */
  case 2:
    snprintf( ctx->path, PATH_MAX, "." FUSE_DIR_SEP_STR "%s",
              path_segment );
    return 1;

  case 3: return 0;
  }

  ui_error( UI_ERROR_ERROR, "unknown path_context state %d", ctx->state );
  fuse_abort();
}
