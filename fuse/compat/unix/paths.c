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

#include "compat.h"
#include "fuse.h"
#include "ui/ui.h"

void get_relative_directory( char *buffer, size_t bufsize );

const char*
compat_get_temp_path( void )
{
  const char *dir;

  /* Use TMPDIR if specified, if not /tmp */
  dir = getenv( "TMPDIR" ); if( dir ) return dir;
  return "/tmp";
}

const char*
compat_get_config_path( void )
{
  const char *dir;
  dir = getenv( "HOME" ); if( dir ) return dir;
  return ".";
}

int
compat_is_absolute_path( const char *path )
{
  return path[0] == '/';
}

int
compat_get_next_path( path_context *ctx )
{
  char buffer[ PATH_MAX ];
  const char *path_segment, *path2;

  switch( (ctx->state)++ ) {

    /* First look relative to the current directory */
  case 0:
    strncpy( ctx->path, ".", PATH_MAX );
    return 1;

    /* Then relative to the Fuse executable */
  case 1:

    switch( ctx->type ) {
    case UTILS_AUXILIARY_LIB: path_segment = "lib"; break;
    case UTILS_AUXILIARY_ROM: path_segment = "roms"; break;
    case UTILS_AUXILIARY_WIDGET: path_segment = "ui/widget"; break;
    case UTILS_AUXILIARY_GTK: path_segment = "ui/gtk"; break;
    default:
      ui_error( UI_ERROR_ERROR, "unknown auxiliary file type %d", ctx->type );
      return 0;
    }

    if( compat_is_absolute_path( fuse_progname ) ) {
      strncpy( buffer, fuse_progname, PATH_MAX );
      buffer[ PATH_MAX - 1 ] = '\0';
    } else {
      get_relative_directory( buffer, PATH_MAX );
    }

    path2 = dirname( buffer );
    snprintf( ctx->path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s", path2,
              path_segment );
    return 1;

    /* Then where we may have installed the data files */
  case 2:

#ifndef ROMSDIR
    path2 = FUSEDATADIR;
#else				/* #ifndef ROMSDIR */
    path2 = ctx->type == UTILS_AUXILIARY_ROM ? ROMSDIR : FUSEDATADIR;
#endif				/* #ifndef ROMSDIR */
    strncpy( ctx->path, path2, PATH_MAX ); buffer[ PATH_MAX - 1 ] = '\0';
    return 1;

  case 3: return 0;
  }
  ui_error( UI_ERROR_ERROR, "unknown path_context state %d", ctx->state );
  fuse_abort();
}
