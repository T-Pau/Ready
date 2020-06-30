/* event.c: Routines needed for dealing with the event list
   Copyright (c) 2000-2015 Philip Kendall

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

#include <libspectrum.h>

#include "event.h"
#include "infrastructure/startup_manager.h"
#include "fuse.h"
#include "ui/ui.h"
#include "utils.h"

/* A large value to mean `no events due' */
static const libspectrum_dword event_no_events = 0xffffffff;

/* When will the next event happen? */
libspectrum_dword event_next_event;

/* The actual list of events */
static GSList *event_list = NULL;

/* An event ready to be reused */
static event_t *event_free = NULL;

/* A null event */
int event_type_null;

typedef struct event_descriptor_t {
  event_fn_t fn;
  char *description;
} event_descriptor_t; 

static GArray *registered_events;

static int
event_init( void *context )
{
  registered_events = g_array_new( FALSE, FALSE, sizeof( event_descriptor_t ) );

  event_type_null = event_register( NULL, "[Deleted event]" );

  event_next_event = event_no_events;

  return 0;
}

int
event_register( event_fn_t fn, const char *description )
{
  event_descriptor_t descriptor;

  descriptor.fn = fn;
  descriptor.description = utils_safe_strdup( description );

  g_array_append_val( registered_events, descriptor );

  return registered_events->len - 1;
}

static gint
event_add_cmp( gconstpointer a1, gconstpointer b1 )
{
  const event_t *a = a1, *b = b1;

  return a->tstates != b->tstates ? a->tstates - b->tstates
		                  : a->type - b->type;
}

/* Add an event at the correct place in the event list */
void
event_add_with_data( libspectrum_dword event_time, int type, void *user_data )
{
  event_t *ptr;

  if( event_free ) {
    ptr = event_free;
    event_free = NULL;
  } else {
    ptr = libspectrum_new( event_t, 1 );
  }

  ptr->tstates = event_time;
  ptr->type =type;
  ptr->user_data = user_data;

  if( event_time < event_next_event ) {
    event_next_event = event_time;
    event_list = g_slist_prepend( event_list, ptr );
  } else {
    event_list = g_slist_insert_sorted( event_list, ptr, event_add_cmp );
  }
}

/* Do all events which have passed */
int
event_do_events( void )
{
  event_t *ptr;

  while(event_next_event <= tstates) {
    event_descriptor_t descriptor;
    ptr = event_list->data;
    descriptor =
      g_array_index( registered_events, event_descriptor_t, ptr->type );

    /* Remove the event from the list *before* processing */
    event_list = g_slist_remove( event_list, ptr );

    if( event_list == NULL ) {
      event_next_event = event_no_events;
    } else {
      event_next_event = ((event_t*)(event_list->data))->tstates;
    }

    if( descriptor.fn ) descriptor.fn( ptr->tstates, ptr->type, ptr->user_data );

    if( event_free ) {
      libspectrum_free( ptr );
    } else {
      event_free = ptr;
    }
  }

  return 0;
}

static void
event_reduce_tstates( gpointer data, gpointer user_data )
{
  event_t *ptr = data;
  libspectrum_dword *tstates_per_frame = user_data;

  ptr->tstates -= *tstates_per_frame;
}

/* Called at end of frame to reduce T-state count of all entries */
void
event_frame( libspectrum_dword tstates_per_frame )
{
  g_slist_foreach( event_list, event_reduce_tstates, &tstates_per_frame );

  event_next_event = event_list ?
    ((event_t*)(event_list->data))->tstates : event_no_events;
}

/* Do all events that would happen between the current time and when
   the next interrupt will occur; called only when RZX playback is in
   effect */
void
event_force_events( void )
{
  while( event_next_event < machine_current->timings.tstates_per_frame ) {

    /* Jump along to the next event */
    tstates = event_next_event;
    
    /* And do that event */
    event_do_events();

  }
}

static void
set_event_null( gpointer data, gpointer user_data )
{
  event_t *ptr = data;
  int *type = user_data;

  if( ptr->type == *type ) ptr->type = event_type_null;
}

static void
set_event_null_with_user_data( gpointer data, gpointer user_data )
{
  event_t *event = data;
  event_t *template = user_data;

  if( event->type == template->type && event->user_data == template->user_data )
    event->type = event_type_null;
}

/* Remove all events of a specific type from the stack */
void
event_remove_type( int type )
{
  g_slist_foreach( event_list, set_event_null, &type );
}

/* Remove all events of a specific type and user data from the stack */
void
event_remove_type_user_data( int type, gpointer user_data )
{
  event_t template;

  template.type = type;
  template.user_data = user_data;
  g_slist_foreach( event_list, set_event_null_with_user_data, &template );
}

/* Free the memory used by a specific entry */
static void
event_free_entry( gpointer data, gpointer user_data GCC_UNUSED )
{
  libspectrum_free( data );
}

/* Clear the event stack */
void
event_reset( void )
{
  g_slist_foreach( event_list, event_free_entry, NULL );
  g_slist_free( event_list );
  event_list = NULL;

  event_next_event = event_no_events;

  libspectrum_free( event_free );
  event_free = NULL;
}

/* Call a user-supplied function for every event in the current list */
void
event_foreach( GFunc function, gpointer user_data )
{
  g_slist_foreach( event_list, function, user_data );
}

/* A textual representation of each event type */
const char*
event_name( int type )
{
  return g_array_index( registered_events, event_descriptor_t, type ).description;
}

static void
registered_events_free( void )
{
  int i;
  event_descriptor_t descriptor;

  if( !registered_events ) return;

  for( i = 0; i < registered_events->len; i++ ) {
    descriptor = g_array_index( registered_events, event_descriptor_t, i );
    libspectrum_free( descriptor.description );
  }

  g_array_free( registered_events, TRUE );
  registered_events = NULL;
}

/* Tidy-up function called at end of emulation */
static void
event_end( void )
{
  event_reset();
  registered_events_free();
}

void
event_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_EVENT, dependencies,
                            ARRAY_SIZE( dependencies ), event_init, NULL,
                            event_end );
}
