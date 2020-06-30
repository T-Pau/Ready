/* breakpoint.c: a debugger breakpoint
   Copyright (c) 2002-2011 Philip Kendall
   Copyright (c) 2013-2015 Sergio Baldoví
   Copyright (c) 2015 Tom Seddon
   Copyright (c) 2016 BogDan Vatra

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

#include <ctype.h>
#include <string.h>

#include <libspectrum.h>

#include "debugger_internals.h"
#include "event.h"
#include "fuse.h"
#include "memory_pages.h"
#include "ui/ui.h"
#include "utils.h"

/* The current breakpoints */
GSList *debugger_breakpoints;

/* The next breakpoint ID to use */
static size_t next_breakpoint_id;

/* Textual representations of the breakpoint types and lifetimes */
const char *debugger_breakpoint_type_text[] = {
  "Execute", "Read", "Write", "Port Read", "Port Write", "Time", "Event",
};

const char debugger_breakpoint_type_abbr[][4] = {
  "Exe", "Rd", "Wr", "PtR", "PtW", "Tm", "Ev",
};

const char *debugger_breakpoint_life_text[] = {
  "Permanent", "One Shot",
};

const char debugger_breakpoint_life_abbr[][5] = {
  "Perm", "Once",
};

static int breakpoint_add( debugger_breakpoint_type type,
			   debugger_breakpoint_value value, size_t ignore,
			   debugger_breakpoint_life life,
			   debugger_expression *condition );
static int breakpoint_check( debugger_breakpoint *bp,
			     debugger_breakpoint_type type,
			     libspectrum_dword value );
static debugger_breakpoint* get_breakpoint_by_id( size_t id );
static gint find_breakpoint_by_id( gconstpointer data,
				   gconstpointer user_data );
static void remove_time( gpointer data, gpointer user_data );
static gint find_breakpoint_by_id( gconstpointer data,
				   gconstpointer user_data );
static gint find_breakpoint_by_address( gconstpointer data,
					gconstpointer user_data );
static void free_breakpoint( gpointer data, gpointer user_data );
static void add_time_event( gpointer data, gpointer user_data );

/* Add a breakpoint */
int
debugger_breakpoint_add_address( debugger_breakpoint_type type, int source,
                                 int page, libspectrum_word offset,
                                 size_t ignore, debugger_breakpoint_life life,
				 debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
  case DEBUGGER_BREAKPOINT_TYPE_WRITE:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_address given type %d",
	      type );
    fuse_abort();
  }

  value.address.source = source;
  value.address.page = page;
  value.address.offset = offset;

  return breakpoint_add( type, value, ignore, life, condition );
}

int
debugger_breakpoint_add_port( debugger_breakpoint_type type,
			      libspectrum_word port, libspectrum_word mask,
			      size_t ignore, debugger_breakpoint_life life,
			      debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_port given type %d",
	      type );
    fuse_abort();
  }

  value.port.port = port;
  value.port.mask = mask;

  return breakpoint_add( type, value, ignore, life, condition );
}

int
debugger_breakpoint_add_time( debugger_breakpoint_type type,
			      libspectrum_dword breakpoint_tstates,
                              size_t ignore, debugger_breakpoint_life life,
			      debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_TIME:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "debugger_breakpoint_add_time given type %d",
	      type );
    fuse_abort();
  }

  value.time.triggered = 0;
  value.time.tstates = breakpoint_tstates;
  value.time.initial_tstates = breakpoint_tstates;

  return breakpoint_add( type, value, ignore, life, condition );
}

int
debugger_breakpoint_add_event( debugger_breakpoint_type type,
			       const char *type_string, const char *detail,
			       size_t ignore, debugger_breakpoint_life life,
			       debugger_expression *condition )
{
  debugger_breakpoint_value value;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_EVENT:
    break;

  default:
    ui_error( UI_ERROR_ERROR, "%s given type %d", __func__, type );
    fuse_abort();
  }

  if( !debugger_event_is_registered( type_string, detail ) ) {
    ui_error( UI_ERROR_WARNING, "Event type %s:%s not known", type_string,
              detail );
    return 1;
  }

  value.event.detail = NULL;
  value.event.type = utils_safe_strdup( type_string );
  value.event.detail = utils_safe_strdup( detail );

  return breakpoint_add( type, value, ignore, life, condition );
}

static int
breakpoint_add( debugger_breakpoint_type type, debugger_breakpoint_value value,
		size_t ignore, debugger_breakpoint_life life,
		debugger_expression *condition )
{
  debugger_breakpoint *bp;

  bp = libspectrum_new( debugger_breakpoint, 1 );

  bp->id = next_breakpoint_id++; bp->type = type;
  bp->value = value;
  bp->ignore = ignore; bp->life = life;
  if( condition ) {
    bp->condition = debugger_expression_copy( condition );
    if( !bp->condition ) {
      libspectrum_free( bp );
      return 1;
    }
  } else {
    bp->condition = NULL;
  }

  bp->commands = NULL;

  debugger_breakpoints = g_slist_append( debugger_breakpoints, bp );

  if( debugger_mode == DEBUGGER_MODE_INACTIVE )
    debugger_mode = DEBUGGER_MODE_ACTIVE;

  /* If this was a timed breakpoint, set an event to stop emulation
     at that point */
  if( type == DEBUGGER_BREAKPOINT_TYPE_TIME )
    event_add( value.time.tstates, debugger_breakpoint_event );

  ui_breakpoints_updated();

  return 0;
}

/* Check whether the debugger should become active at this point */
int
debugger_check( debugger_breakpoint_type type, libspectrum_dword value )
{
  GSList *ptr; debugger_breakpoint *bp;
  GSList *ptr_next;

  int signal_breakpoints_updated = 0;

  switch( debugger_mode ) {

  case DEBUGGER_MODE_INACTIVE: return 0;

  case DEBUGGER_MODE_ACTIVE:
    for( ptr = debugger_breakpoints; ptr; ptr = ptr_next ) {

      bp = ptr->data;
      ptr_next = ptr->next;

      if( breakpoint_check( bp, type, value ) ) {
        debugger_mode = DEBUGGER_MODE_HALTED;
        debugger_command_evaluate( bp->commands );

        if( bp->life == DEBUGGER_BREAKPOINT_LIFE_ONESHOT ) {
          debugger_breakpoints = g_slist_remove( debugger_breakpoints, bp );
          libspectrum_free( bp );
          signal_breakpoints_updated = 1;
        }
      }

    }
    break;

  case DEBUGGER_MODE_HALTED: return 1;

  }

  if( signal_breakpoints_updated )
      ui_breakpoints_updated();

  /* Debugger mode could have been reset by a breakpoint command */
  return ( debugger_mode == DEBUGGER_MODE_HALTED );
}

void
debugger_breakpoint_reduce_tstates( libspectrum_dword tstates )
{
  GSList *ptr;
  debugger_breakpoint *bp;

  if( debugger_mode != DEBUGGER_MODE_ACTIVE ) return;

  for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {
    bp = ptr->data;

    if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME && !bp->value.time.triggered )
      bp->value.time.tstates -= tstates;
  }
}

static memory_page*
get_page( debugger_breakpoint_type type, libspectrum_word address )
{
  memory_page *bank;

  switch( type ) {
  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
    bank = memory_map_read;
    break;

  case DEBUGGER_BREAKPOINT_TYPE_WRITE:
    bank = memory_map_write;
    break;

  default:
    ui_error( UI_ERROR_ERROR,
	      "%s:get_page: unexpected breakpoint type %d", __FILE__, type );
    fuse_abort();
  }

  return &bank[ address >> MEMORY_PAGE_SIZE_LOGARITHM ];
}

int
debugger_breakpoint_trigger( debugger_breakpoint *bp )
{
  if( bp->ignore ) { bp->ignore--; return 0; }

  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME )
    bp->value.time.triggered = 1;

  if( bp->condition && !debugger_expression_evaluate( bp->condition ) )
    return 0;

  return 1;
}

/* Check whether 'bp' should trigger if we're looking for a breakpoint
   of 'type' with parameter 'value'. Returns non-zero if we should trigger */
static int
breakpoint_check( debugger_breakpoint *bp, debugger_breakpoint_type type,
		  libspectrum_dword value )
{
  if( bp->type != type ) return 0;

  switch( type ) {

  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
  case DEBUGGER_BREAKPOINT_TYPE_WRITE:

    /* If source == memory_source_any, value must match exactly; otherwise,
       the source, page and offset must match */
    if( bp->value.address.source == memory_source_any ) {
      if( bp->value.address.offset != value ) return 0;
    } else {
      memory_page *page = get_page( type, value );
      if( bp->value.address.source != page->source ||
          bp->value.address.page != page->page_num ||
          bp->value.address.offset != ( value & 0x3fff ) ) return 0;
    }
    break;

    /* Port values must match after masking */
  case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
    if( ( value & bp->value.port.mask ) != bp->value.port.port ) return 0;
    break;

    /* Timed breakpoints trigger if we're past the relevant time */
  case DEBUGGER_BREAKPOINT_TYPE_TIME:
    if( bp->value.time.triggered || bp->value.time.tstates > tstates ) return 0;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "Unknown breakpoint type %d", bp->type );
    fuse_abort();

  }

  return debugger_breakpoint_trigger( bp );
}

struct remove_t {

  libspectrum_dword tstates;
  int done;

};

/* Remove breakpoint with the given ID */
int
debugger_breakpoint_remove( size_t id )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  debugger_breakpoints = g_slist_remove( debugger_breakpoints, bp );
  if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  /* If this was a timed breakpoint, remove the event as well */
  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME ) {

    struct remove_t remove;

    remove.tstates = bp->value.time.tstates;
    remove.done = 0;

    event_foreach( remove_time, &remove );
  }

  libspectrum_free( bp );

  ui_breakpoints_updated();

  return 0;
}

static debugger_breakpoint*
get_breakpoint_by_id( size_t id )
{
  GSList *ptr;

  ptr = g_slist_find_custom( debugger_breakpoints, &id,
			     find_breakpoint_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "Breakpoint %ld does not exist",
	      (unsigned long)id );
    return NULL;
  }

  return ptr->data;
}

static gint
find_breakpoint_by_id( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  size_t id = *(const size_t*)user_data;

  return bp->id - id;
}

static void
remove_time( gpointer data, gpointer user_data )
{
  event_t *event;
  struct remove_t *remove;

  event = data; remove = user_data;

  if( remove->done ) return;

  if( event->type == debugger_breakpoint_event &&
      event->tstates == remove->tstates ) {
    event->type = event_type_null;
    remove->done = 1;
  }
}

/* Remove all breakpoints at the given address */
int
debugger_breakpoint_clear( libspectrum_word address )
{
  GSList *ptr;
  gpointer ptr_data;

  int found = 0;

  while( 1 ) {

    ptr = g_slist_find_custom( debugger_breakpoints, &address,
			       find_breakpoint_by_address );
    if( !ptr ) break;

    found++;

    ptr_data = ptr->data;
    debugger_breakpoints = g_slist_remove( debugger_breakpoints, ptr_data );
    if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
      debugger_mode = DEBUGGER_MODE_INACTIVE;

    free_breakpoint( ptr_data, NULL );
  }

  if( !found ) {
    if( debugger_output_base == 10 ) {
      ui_error( UI_ERROR_ERROR, "No breakpoint at %d", address );
    } else {
      ui_error( UI_ERROR_ERROR, "No breakpoint at 0x%04x", address );
    }
  } else {
      ui_breakpoints_updated();
  }

  return 0;
}

static gint
find_breakpoint_by_address( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  libspectrum_word address = *(const libspectrum_word*)user_data;

  if( bp->type != DEBUGGER_BREAKPOINT_TYPE_EXECUTE &&
      bp->type != DEBUGGER_BREAKPOINT_TYPE_READ    &&
      bp->type != DEBUGGER_BREAKPOINT_TYPE_WRITE      )
    return 1;

  /* Ignore all page-specific breakpoints */
  if( bp->value.address.source != memory_source_any ) return 1;

  return bp->value.address.offset - address;
}

/* Remove all breakpoints */
int
debugger_breakpoint_remove_all( void )
{
  g_slist_foreach( debugger_breakpoints, free_breakpoint, NULL );
  g_slist_free( debugger_breakpoints ); debugger_breakpoints = NULL;

  if( debugger_mode == DEBUGGER_MODE_ACTIVE )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  /* Restart the breakpoint numbering */
  next_breakpoint_id = 1;

  ui_breakpoints_updated();

  return 0;
}

static void
free_breakpoint( gpointer data, gpointer user_data GCC_UNUSED )
{
  debugger_breakpoint *bp = data;

  switch( bp->type ) {
  case DEBUGGER_BREAKPOINT_TYPE_EVENT:
    libspectrum_free( bp->value.event.type );
    libspectrum_free( bp->value.event.detail );
    break;

  case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
  case DEBUGGER_BREAKPOINT_TYPE_READ:
  case DEBUGGER_BREAKPOINT_TYPE_WRITE:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
  case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
  case DEBUGGER_BREAKPOINT_TYPE_TIME:
    /* No action needed */
    break;
  }

  if( bp->condition ) debugger_expression_delete( bp->condition );
  if( bp->commands ) libspectrum_free( bp->commands );

  libspectrum_free( bp );
}

/* Ignore breakpoint 'id' the next 'ignore' times it hits */
int
debugger_breakpoint_ignore( size_t id, size_t ignore )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  bp->ignore = ignore;

  return 0;
}

/* Set the breakpoint's conditional expression */
int
debugger_breakpoint_set_condition( size_t id, debugger_expression *condition )
{
  debugger_breakpoint *bp;

  bp = get_breakpoint_by_id( id ); if( !bp ) return 1;

  if( bp->condition ) debugger_expression_delete( bp->condition );

  if( condition ) {
    bp->condition = debugger_expression_copy( condition );
    if( !bp->condition ) return 1;
  } else {
    bp->condition = NULL;
  }

  return 0;
}

int
debugger_breakpoint_set_commands( size_t id, const char *commands )
{
  debugger_breakpoint *bp = get_breakpoint_by_id( id );
  if( !bp ) return 1;

  libspectrum_free( bp->commands );
  bp->commands = utils_safe_strdup( commands );

  return 0;
}

/* Add events corresponding to all the time breakpoints to happen during
   this frame */
int
debugger_add_time_events( void )
{
  g_slist_foreach( debugger_breakpoints, add_time_event, NULL );
  return 0;
}

static void
add_time_event( gpointer data, gpointer user_data GCC_UNUSED )
{
  debugger_breakpoint *bp = data;

  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_TIME && bp->value.time.triggered ) {
    bp->value.time.triggered = 0;
    bp->value.time.tstates = bp->value.time.initial_tstates;
    event_add( bp->value.time.tstates, debugger_breakpoint_event );
  }
}

void
debugger_breakpoint_time_fn( libspectrum_dword tstates, int type GCC_UNUSED,
                             void *user_data GCC_UNUSED )
{
  debugger_check( DEBUGGER_BREAKPOINT_TYPE_TIME, 0 );
}
