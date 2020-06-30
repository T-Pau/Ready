/* profile.c: Z80 profiler
   Copyright (c) 2005-2016 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <libspectrum.h>

#include "event.h"
#include "infrastructure/startup_manager.h"
#include "module.h"
#include "profile.h"
#include "ui/ui.h"
#include "z80/z80.h"

int profile_active = 0;

static int total_tstates[ 0x10000 ];
static libspectrum_word profile_last_pc;
static libspectrum_dword profile_last_tstates;

static void profile_from_snapshot( libspectrum_snap *snap GCC_UNUSED );

static module_info_t profile_module_info = {

  NULL,
  NULL,
  NULL,
  profile_from_snapshot,
  NULL,

};

static int
profile_init( void *context )
{
  module_register( &profile_module_info );

  return 0;
}

void
profile_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_PROFILE, dependencies,
                            ARRAY_SIZE( dependencies ), profile_init, NULL,
                            NULL );
}

static void
init_profiling_counters( void )
{
  profile_last_pc = z80.pc.w;
  profile_last_tstates = tstates;
}

void
profile_start( void )
{
  memset( total_tstates, 0, sizeof( total_tstates ) );

  profile_active = 1;
  init_profiling_counters();

  /* Schedule an event to ensure that the main z80 emulation loop recognises
     profiling is turned on; otherwise problems occur if we started while
     the debugger was active (bug #54) */
  event_add( tstates, event_type_null );

  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 1 );
}

void
profile_map( libspectrum_word pc )
{
  total_tstates[ profile_last_pc ] += tstates - profile_last_tstates;

  profile_last_pc = z80.pc.w;
  profile_last_tstates = tstates;
}

void
profile_frame( libspectrum_dword frame_length )
{
  profile_last_tstates -= frame_length;
}

/* On snapshot load, PC and the tstate counter will jump so reset our
   current views of these */
static void
profile_from_snapshot( libspectrum_snap *snap GCC_UNUSED )
{
  init_profiling_counters();
}

void
profile_finish( const char *filename )
{
  FILE *f;
  size_t i;

  f = fopen( filename, "w" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "unable to open profile map '%s' for writing",
	      filename );
    return;
  }

  for( i = 0; i < 0x10000; i++ ) {

    if( !total_tstates[ i ] ) continue;

    fprintf( f, "0x%04lx,%d\n", (unsigned long)i, total_tstates[ i ] );

  }

  fclose( f );

  profile_active = 0;

  /* Again, schedule an event to ensure this change is picked up by
     the main loop */
  event_add( tstates, event_type_null );

  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 0 );
}
