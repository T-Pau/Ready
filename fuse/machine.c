/* machine.c: Routines for handling the various machine types
   Copyright (c) 1999-2015 Philip Kendall

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

#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "machines/machines.h"
#include "machines/scorpion.h"
#include "machines/spec128.h"
#include "machines/spec48.h"
#include "machines/specplus3.h"
#include "machines/tc2068.h"
#include "memory_pages.h"
#include "module.h"
#include "movie.h"
#include "peripherals/ula.h"
#include "pokefinder/pokemem.h"
#include "rzx.h"
#include "settings.h"
#include "snapshot.h"
#include "sound.h"
#include "tape.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "utils.h"

fuse_machine_info **machine_types = NULL; /* Array of available machines */
int machine_count = 0;

fuse_machine_info *machine_current = NULL; /* The currently selected machine */
static int machine_location;	/* Where is the current machine in
				   machine_types[...]? */

static int machine_add_machine( int (*init_function)(fuse_machine_info *machine) );
static int machine_select_machine( fuse_machine_info *machine );
static void machine_set_const_timings( fuse_machine_info *machine );
static void machine_set_variable_timings( fuse_machine_info *machine );

static int
machine_init_machines( void *context )
{
  int error;

  error = machine_add_machine( spec16_init    );
  if (error ) return error;
  error = machine_add_machine( spec48_init    );
  if (error ) return error;
  error = machine_add_machine( spec48_ntsc_init );
  if (error ) return error;
  error = machine_add_machine( spec128_init   );
  if (error ) return error;
  error = machine_add_machine( specplus2_init );
  if (error ) return error;
  error = machine_add_machine( specplus2a_init );
  if( error ) return error;

  error = machine_add_machine( specplus3_init );
  if (error ) return error;

  error = machine_add_machine( specplus3e_init );
  if( error ) return error;

  error = machine_add_machine( tc2048_init );
  if (error ) return error;
  error = machine_add_machine( tc2068_init );
  if( error ) return error;
  error = machine_add_machine( ts2068_init );
  if( error ) return error;
  error = machine_add_machine( pentagon_init );
  if (error ) return error;
  error = machine_add_machine( pentagon512_init );
  if (error ) return error;
  error = machine_add_machine( pentagon1024_init );
  if (error ) return error;
  error = machine_add_machine( scorpion_init );
  if ( error ) return error;
  error = machine_add_machine( spec_se_init );
  if ( error ) return error;

  return 0;
}

static int machine_add_machine( int (*init_function)( fuse_machine_info *machine ) )
{
  fuse_machine_info *machine;
  int error;

  machine_count++;

  machine_types =
    libspectrum_renew( fuse_machine_info *, machine_types, machine_count );

  machine_types[ machine_count - 1 ] = libspectrum_new( fuse_machine_info, 1 );
  machine = machine_types[ machine_count - 1 ];

  error = init_function( machine ); if( error ) return error;

  machine_set_const_timings( machine );

  machine->capabilities = libspectrum_machine_capabilities( machine->machine );

  return 0;
}

int
machine_select( libspectrum_machine type )
{
  int i;
  int error;

  /* Stop any ongoing RZX */
  rzx_stop_recording();
  rzx_stop_playback( 1 );

  /* We don't want to have to deal with screen size changes in the movie code
     and recording movies where we change machines seems pretty obscure */
  movie_stop();

  for( i=0; i < machine_count; i++ ) {
    if( machine_types[i]->machine == type ) {
      machine_location = i;
      error = machine_select_machine( machine_types[i] );

      if( !error ) return 0;

      /* If we couldn't select the new machine type, try falling back
	 to plain old 48K */
      if( type != LIBSPECTRUM_MACHINE_48 ) 
	error = machine_select( LIBSPECTRUM_MACHINE_48 );
	
      /* If that still didn't work, give up */
      if( error ) {
	ui_error( UI_ERROR_ERROR, "can't select 48K machine. Giving up." );
	fuse_abort();
      } else {
	ui_error( UI_ERROR_INFO, "selecting 48K machine" );
	return 0;
      }
      
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "machine type %d unknown", type );
  return 1;
}

int machine_select_id( const char *id )
{
  int i, error;

  for( i=0; i < machine_count; i++ ) {
    if( ! strcmp( machine_types[i]->id, id ) ) {
      error = machine_select_machine( machine_types[i] );
      if( error ) return error;
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "Machine id '%s' unknown", id );
  return 1;
}

const char*
machine_get_id( libspectrum_machine type )
{
  int i;

  for( i=0; i<machine_count; i++ )
    if( machine_types[i]->machine == type ) return machine_types[i]->id;

  return NULL;
}

static int
machine_select_machine( fuse_machine_info *machine )
{
  int width, height;
  int capabilities;

  machine_current = machine;

  settings_set_string( &settings_current.start_machine, machine->id );
  
  tstates = 0;

  /* Reset the event stack */
  event_reset();
  event_add( 0, timer_event );
  event_add( machine->timings.tstates_per_frame, spectrum_frame_event );

  sound_end();

  if( uidisplay_end() ) return 1;

  capabilities = libspectrum_machine_capabilities( machine->machine );

  /* Set screen sizes here */
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ) {
    width = DISPLAY_SCREEN_WIDTH;
    height = 2*DISPLAY_SCREEN_HEIGHT;
  } else {
    width = DISPLAY_ASPECT_WIDTH;
    height = DISPLAY_SCREEN_HEIGHT;
  }

  if( uidisplay_init( width, height ) ) return 1;

  sound_init( settings_current.sound_device );

  /* Do a hard reset */
  if( machine_reset( 1 ) ) return 1;

  /* And the dock menu item */
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT, 0 );
  }

  /* Reset any dialogue boxes etc. which contain machine-dependent state */
  ui_widgets_reset();

  return 0;
}

int
machine_load_rom_bank_from_buffer( memory_page* bank_map, int page_num,
  unsigned char *buffer, size_t length, int custom )
{
  size_t offset;
  libspectrum_byte *data = memory_pool_allocate( length );
  memory_page *page;

  memcpy( data, buffer, length );

  for( page = &bank_map[ page_num * MEMORY_PAGES_IN_16K ], offset = 0;
       offset < length;
       page++, offset += MEMORY_PAGE_SIZE ) {
    page->offset = offset;
    page->page_num = page_num;
    page->page = data + offset;
    page->writable = 0;
    page->save_to_snapshot = custom;
  }

  return 0;
}

static int
machine_load_rom_bank_from_file( memory_page* bank_map, int page_num,
  const char *filename, size_t expected_length, int custom )
{
  int error;
  utils_file rom;

  error = utils_read_auxiliary_file( filename, &rom, UTILS_AUXILIARY_ROM );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find ROM '%s'", filename );
    return 1;
  }
  if( error ) return error;
  
  if( rom.length != expected_length ) {
    ui_error( UI_ERROR_ERROR,
	      "ROM '%s' is %ld bytes long; expected %ld bytes",
	      filename, (unsigned long)rom.length,
	      (unsigned long)expected_length );
    utils_close_file( &rom );
    return 1;
  }

  error = machine_load_rom_bank_from_buffer( bank_map, page_num, rom.buffer,
    rom.length, custom );

  utils_close_file( &rom );

  return error;
}

int
machine_load_rom_bank( memory_page* bank_map, int page_num,
  const char *filename, const char *fallback, size_t expected_length )
{
  int custom = 0;
  int retval;

  if( fallback ) custom = !!strcmp( filename, fallback );

  retval = machine_load_rom_bank_from_file( bank_map, page_num, filename,
    expected_length, custom );
  if( retval && fallback && custom )
    retval = machine_load_rom_bank_from_file( bank_map, page_num, fallback,
      expected_length, 0 );
  return retval;
}

int
machine_load_rom( int page_num, const char *filename, const char *fallback,
  size_t expected_length )
{
  return machine_load_rom_bank( memory_map_rom, page_num, filename, fallback,
    expected_length );
}

int
machine_reset( int hard_reset )
{
  size_t i;
  int error;

  /* Clear poke list (undoes effects of active pokes on Spectrum memory) */
  pokemem_clear();

  sound_ay_reset();

  tape_stop();

  memory_pool_free();

  machine_current->ram.romcs = 0;

  machine_set_variable_timings( machine_current );

  memory_reset();

  /* Do the machine-specific bits, including loading the ROMs */
  error = machine_current->reset(); if( error ) return error;

  module_reset( hard_reset );

  error = machine_current->memory_map(); if( error ) return error;

  /* Set up the contention array */
  for( i = 0; i < machine_current->timings.tstates_per_frame; i++ ) {
    ula_contention[ i ] = machine_current->ram.contend_delay( i );
    ula_contention_no_mreq[ i ] = machine_current->ram.contend_delay_no_mreq( i );
  }

  /* Update the disk menu items */
  ui_menu_disk_update();

  /* clear out old display image ready for new one */
  display_refresh_all();

  return 0;
}

static void
machine_set_const_timings( fuse_machine_info *machine )
{
  /* Pull timings we use repeatedly out of libspectrum and store them
     for ourself */
  machine->timings.processor_speed =
    libspectrum_timings_processor_speed( machine->machine );
  machine->timings.left_border =
    libspectrum_timings_left_border( machine->machine );
  machine->timings.horizontal_screen =
    libspectrum_timings_horizontal_screen( machine->machine );
  machine->timings.right_border =
    libspectrum_timings_right_border( machine->machine );
  machine->timings.tstates_per_line =
    libspectrum_timings_tstates_per_line( machine->machine );
  machine->timings.interrupt_length =
    libspectrum_timings_interrupt_length( machine->machine );
  machine->timings.tstates_per_frame =
    libspectrum_timings_tstates_per_frame( machine->machine );
}

static void
machine_set_variable_timings( fuse_machine_info *machine )
{
  size_t y;

  /* Magic number alert

     libspectrum_timings_top_left_pixel gives us the number of tstates
     after the interrupt at which the top-left pixel of the screen is
     displayed. However, what's more useful for Fuse is when the first
     pixel of the top line of the border is displayed.

     Fuse shows DISPLAY_BORDER_HEIGHT lines of top border and
     DISPLAY_BORDER_WIDTH_COLS columns of left border, so we subtract
     the appropriate offset to get when the top-left pixel of the
     border is displayed.
  */

  machine->line_times[0]=
    libspectrum_timings_top_left_pixel( machine->machine ) -
    /* DISPLAY_BORDER_HEIGHT lines of top border */
    DISPLAY_BORDER_HEIGHT * machine->timings.tstates_per_line -
    4 * DISPLAY_BORDER_WIDTH_COLS; /* Left border at 4 tstates per column */

  if( settings_current.late_timings ) machine->line_times[0]++;

  for( y=1; y<DISPLAY_SCREEN_HEIGHT+1; y++ ) {
    machine->line_times[y] = machine->line_times[y-1] + 
                             machine->timings.tstates_per_line;
  }
}

static void
machine_end( void )
{
  int i;

  for( i=0; i<machine_count; i++ ) {
    if( machine_types[i]->shutdown ) machine_types[i]->shutdown();
    libspectrum_free( machine_types[i] );
  }

  libspectrum_free( machine_types );
}

void
machine_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_MACHINE, dependencies,
                            ARRAY_SIZE( dependencies ), machine_init_machines,
                            NULL, machine_end );
}
