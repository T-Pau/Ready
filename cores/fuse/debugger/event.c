/* event.c: Debugger events
   Copyright (c) 2008 Philip Kendall
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

static GArray *registered_events;

void
debugger_event_init( void )
{
  registered_events = g_array_new( FALSE, FALSE, sizeof( debugger_event_t ) );
}

int
debugger_event_register( const char *type, const char *detail )
{
  debugger_event_t event;

  event.type = utils_safe_strdup( type );
  event.detail = utils_safe_strdup( detail );

  g_array_append_val( registered_events, event );

  return registered_events->len - 1;
}

static int
event_matches( debugger_event_t *event, const char *type, const char *detail )
{
  if( strcasecmp( type, event->type ) ) return 0;
  if( strcmp( detail, "*" ) == 0 ) return 1;
  if( strcmp( event->detail, "*" ) == 0 ) return 1;
  return strcasecmp( detail, event->detail ) == 0;
}

int
debugger_event_is_registered( const char *type, const char *detail )
{
  size_t i;

  for( i = 0; i < registered_events->len; i++ ) {
    debugger_event_t event =
      g_array_index( registered_events, debugger_event_t, i );

    if( event_matches( &event, type, detail ) ) return 1;
  }

  return 0;
}

void
debugger_event( int event_code )
{
  debugger_event_t event;
  debugger_breakpoint *bp;
  GSList *ptr, *ptr_next;

  int signal_breakpoints_updated = 0;

  if( event_code >= registered_events->len ) {
    ui_error( UI_ERROR_ERROR, "internal error: invalid debugger event %d",
	      event_code );
    fuse_abort();
  }

  event = g_array_index( registered_events, debugger_event_t, event_code );

  for( ptr = debugger_breakpoints; ptr; ptr = ptr_next ) {

    bp = ptr->data;
    ptr_next = ptr->next;

    if( bp->type != DEBUGGER_BREAKPOINT_TYPE_EVENT ) continue;

    if( event_matches( &bp->value.event, event.type, event.detail ) &&
        debugger_breakpoint_trigger( bp ) ) {
      debugger_mode = DEBUGGER_MODE_HALTED;
      debugger_command_evaluate( bp->commands );

      if( bp->life == DEBUGGER_BREAKPOINT_LIFE_ONESHOT ) {
        debugger_breakpoints = g_slist_remove( debugger_breakpoints, bp );
        libspectrum_free( bp );
        signal_breakpoints_updated = 1;
      }
    }
  }

  if( signal_breakpoints_updated )
      ui_breakpoints_updated();
}

/* Tidy-up function called at end of emulation */
void
debugger_event_end( void )
{
  int i;
  debugger_event_t event;

  if( !registered_events ) return;

  for( i = 0; i < registered_events->len; i++ ) {
    event = g_array_index( registered_events, debugger_event_t, i );
    libspectrum_free( event.detail );
    libspectrum_free( event.type );
  }

  g_array_free( registered_events, TRUE );
  registered_events = NULL;
}
