/* event.c: Debugger system variables
   Copyright (c) 2016 Philip Kendall

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

#include <string.h>
#ifdef HAVE_STRINGS_STRCASECMP
#include <strings.h>
#endif      /* #ifdef HAVE_STRINGS_STRCASECMP */

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "debugger_internals.h"
#include "fuse.h"
#include "ui/ui.h"
#include "utils.h"

typedef struct system_variable_t {
  char *type;
  char *detail;
  debugger_get_system_variable_fn_t get;
  debugger_set_system_variable_fn_t set;
} system_variable_t;

static GArray *system_variables;

void
debugger_system_variable_init( void )
{
  system_variables = g_array_new( FALSE, FALSE, sizeof( system_variable_t ) );
}

void
debugger_system_variable_register( const char *type, const char *detail,
                                   debugger_get_system_variable_fn_t get,
                                   debugger_set_system_variable_fn_t set )
{
  system_variable_t sysvar;

  sysvar.type = utils_safe_strdup( type );
  sysvar.detail = utils_safe_strdup( detail );
  sysvar.get = get;
  sysvar.set = set;

  g_array_append_val( system_variables, sysvar );
}

static int
system_variable_matches( system_variable_t *sysvar, const char *type, const char *detail )
{
  return strcasecmp( type, sysvar->type ) == 0 &&
         strcasecmp( detail, sysvar->detail ) == 0;
}

static int
find_system_variable( const char *type, const char *detail, system_variable_t *out )
{
  size_t i;

  for( i = 0; i < system_variables->len; i++ ) {
    system_variable_t sysvar =
      g_array_index( system_variables, system_variable_t, i );

    if( system_variable_matches( &sysvar, type, detail ) ) {
      if( out != NULL ) *out = sysvar;
      return i;
    }
  }

  return -1;
}

int
debugger_system_variable_find( const char *type, const char *detail )
{
  return find_system_variable( type, detail, NULL );
}

libspectrum_dword
debugger_system_variable_get( int system_variable )
{
  system_variable_t sysvar =
    g_array_index( system_variables, system_variable_t, system_variable );

  return sysvar.get();
}

void
debugger_system_variable_set( const char *type, const char *detail,
                              libspectrum_dword value )
{
  int index;
  system_variable_t sysvar;

  index = find_system_variable( type, detail, &sysvar );
  if( index == -1 ) {
    ui_error( UI_ERROR_ERROR, "Unknown system variable %s:%s", type, detail );
    return;
  }

  if (sysvar.set == NULL) {
    ui_error( UI_ERROR_ERROR, "System variable %s:%s cannot be set", type,
              detail );
    return;
  }

  sysvar.set( value );
}

void
debugger_system_variable_text( char *buffer, size_t length,
                               int system_variable )
{
  system_variable_t sysvar =
    g_array_index( system_variables, system_variable_t, system_variable );

  snprintf( buffer, length, "%s:%s", sysvar.type, sysvar.detail );
}

/* Tidy-up function called at end of emulation */
void
debugger_system_variable_end( void )
{
  int i;
  system_variable_t sysvar;

  if( !system_variables ) return;

  for( i = 0; i < system_variables->len; i++ ) {
    sysvar = g_array_index( system_variables, system_variable_t, i );
    libspectrum_free( sysvar.detail );
    libspectrum_free( sysvar.type );
  }

  g_array_free( system_variables, TRUE );
  system_variables = NULL;
}
