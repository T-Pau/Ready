/* event.h: Routines needed for dealing with the event list
   Copyright (c) 2000-2004 Philip Kendall

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

#ifndef FUSE_EVENT_H
#define FUSE_EVENT_H

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

/* Information about an event */
typedef struct event_t {
  libspectrum_dword tstates;
  int type;
  void *user_data;
} event_t;

/* A null event type */
extern int event_type_null;

/* The function to be called when an event occurs */
typedef void (*event_fn_t)( libspectrum_dword tstates, int type, void *user_data );

/* When will the next event happen? */
extern libspectrum_dword event_next_event;

/* Register a new event type */
int event_register( event_fn_t fn, const char *description );

/* Add an event at the correct place in the event list */
void event_add_with_data( libspectrum_dword event_time, int type,
			  void *user_data );

static inline void
event_add( libspectrum_dword event_time, int type )
{
  event_add_with_data( event_time, type, NULL );
}

/* Do all events which have passed */
int event_do_events(void);

/* Called at end of frame to reduce T-state count of all entries */
void event_frame( libspectrum_dword tstates_per_frame );

/* Force all events between now and the next interrupt to happen */
void event_force_events( void );

/* Remove all events of a specific type from the stack */
void event_remove_type( int type );

/* Remove all events of a specific type and user data from the stack */
void event_remove_type_user_data( int type, gpointer user_data );

/* Clear the event stack */
void event_reset( void );

/* Call a user-supplied function for every event in the current list */
void event_foreach( GFunc function, gpointer user_data );

/* A textual representation of each event type */
const char *event_name( int type );

/* Register the init and end functions */
void event_register_startup( void );

#endif				/* #ifndef FUSE_EVENT_H */
