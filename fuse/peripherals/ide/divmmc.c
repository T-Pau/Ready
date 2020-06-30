/* divmmc.c: DivMMC interface routines
   Copyright (c) 2005-2017 Matthew Westcott, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

#include <libspectrum.h>

#include <string.h>

#include "debugger/debugger.h"
#include "ide.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "unittests/unittests.h"
#include "divmmc.h"
#include "divxxx.h"

/* Private function prototypes */

static void divmmc_control_write( libspectrum_word port, libspectrum_byte data );
static void divmmc_card_select( libspectrum_word port, libspectrum_byte data );
static libspectrum_byte divmmc_mmc_read( libspectrum_word port, libspectrum_byte *attached );
static void divmmc_mmc_write( libspectrum_word port, libspectrum_byte data );
static void divmmc_activate( void );
static libspectrum_dword get_control_register( void );
static void set_control_register( libspectrum_dword value );

/* Data */

static const periph_port_t divmmc_ports[] = {
  { 0x00ff, 0x00e3, NULL, divmmc_control_write },
  { 0x00ff, 0x00e7, NULL, divmmc_card_select },
  { 0x00ff, 0x00eb, divmmc_mmc_read, divmmc_mmc_write },
  { 0, 0, NULL, NULL }
};

static const periph_t divmmc_periph = {
  /* .option = */ &settings_current.divmmc_enabled,
  /* .ports = */ divmmc_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ divmmc_activate,
};

static divxxx_t *divmmc_state;

/* The card inserted into the DivMMC. For now, we emulate only one card. */
static libspectrum_mmc_card *card;

/* The card currently selected via the "card select" call */
static libspectrum_mmc_card *current_card;

/* *Our* DivMMC has 128 Kb of RAM */
#define DIVMMC_PAGES 16
#define DIVMMC_PAGE_LENGTH 0x2000

static void divmmc_reset( int hard_reset );
static void divmmc_memory_map( void );
static void divmmc_enabled_snapshot( libspectrum_snap *snap );
static void divmmc_from_snapshot( libspectrum_snap *snap );
static void divmmc_to_snapshot( libspectrum_snap *snap );

static module_info_t divmmc_module_info = {

  /* .reset = */ divmmc_reset,
  /* .romcs = */ divmmc_memory_map,
  /* .snapshot_enabled = */ divmmc_enabled_snapshot,
  /* .snapshot_from = */ divmmc_from_snapshot,
  /* .snapshot_to = */ divmmc_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "divmmc";

/* Debugger system variables */
static const char * const debugger_type_string = "divmmc";
static const char * const control_register_detail_string = "control";

/* Eject menu item */
static const ui_menu_item eject_menu_item = UI_MENU_ITEM_MEDIA_IDE_DIVMMC_EJECT;

/* Housekeeping functions */

static int
divmmc_init( void *context )
{
  card = libspectrum_mmc_alloc();

  ui_menu_activate( eject_menu_item, 0 );

  if( settings_current.divmmc_file ) {
    int error;

    error =
      libspectrum_mmc_insert( card, settings_current.divmmc_file );
    if( error ) return error;

    error = ui_menu_activate( eject_menu_item, 1 );
    if( error ) return error;
  }

  module_register( &divmmc_module_info );

  periph_register( PERIPH_TYPE_DIVMMC, &divmmc_periph );

  divmmc_state = divxxx_alloc( "DivMMC EPROM", DIVMMC_PAGES, "DivMMC RAM",
      event_type_string, &settings_current.divmmc_enabled,
      &settings_current.divmmc_wp );

  debugger_system_variable_register(
    debugger_type_string, control_register_detail_string, get_control_register,
    set_control_register );

  return 0;
}

static void
divmmc_end( void )
{
  divxxx_free( divmmc_state );
  libspectrum_mmc_free( card );
}

void
divmmc_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_DIVMMC, dependencies,
                            ARRAY_SIZE( dependencies ), divmmc_init, NULL,
                            divmmc_end );
}

static void
divmmc_reset( int hard_reset )
{
  divxxx_reset( divmmc_state, hard_reset );
  libspectrum_mmc_reset( card );
}

static int
dirty_fn_wrapper( void *context )
{
  return libspectrum_mmc_dirty( (libspectrum_mmc_card*)context );
}

static libspectrum_error
commit_fn_wrapper( void *context )
{
  libspectrum_mmc_commit( (libspectrum_mmc_card*)context );
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
eject_fn_wrapper( void *context )
{
  libspectrum_mmc_eject( (libspectrum_mmc_card*)context );
  return LIBSPECTRUM_ERROR_NONE;
}

static int
mmc_eject( libspectrum_mmc_card *card )
{
  return ide_eject_mass_storage( dirty_fn_wrapper, commit_fn_wrapper,
      eject_fn_wrapper, card,
      "Card has been modified.\nDo you want to save it?",
      &settings_current.divmmc_file,
      eject_menu_item );
}

int
divmmc_insert( const char *filename )
{
  int error;

  /* Remove any currently inserted card; abort if we want to keep the current
     card */
  if( settings_current.divmmc_file )
    if( mmc_eject( card ) )
      return 0;

  settings_set_string( &settings_current.divmmc_file, filename );

  error = libspectrum_mmc_insert( card, filename );
  if( error ) return error;
  return ui_menu_activate( eject_menu_item, 1 );
}
  
void
divmmc_commit( void )
{
  libspectrum_mmc_commit( card );
}

int
divmmc_eject( void )
{
  return mmc_eject( card );
}

/* Port read/writes */

static void
divmmc_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  divxxx_control_write( divmmc_state, data );
}

static void
divmmc_card_select( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
//  printf("divmmc_card_select( 0x%02x )\n", data );

  /* D0 = MMC0, D1 = MMC1, active LOW
     somehow logic prevents enabling both cards at the same time */
  switch( data & 0x03 ) {
    case 0x02:
      current_card = card;
      break;
    case 0x01:
      /* TODO: select second card */
      current_card = NULL;
      break;
    default:
      current_card = NULL;
      break;
  }
}

static libspectrum_byte
divmmc_mmc_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;

  return current_card ? libspectrum_mmc_read( card ) : 0xff;
}

static void
divmmc_mmc_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( current_card ) libspectrum_mmc_write( card, data );
}

void
divmmc_set_automap( int state )
{
  divxxx_set_automap( divmmc_state, state );
}

void
divmmc_refresh_page_state( void )
{
  divxxx_refresh_page_state( divmmc_state );
}

void
divmmc_memory_map( void )
{
  divxxx_memory_map( divmmc_state );
}

static void
divmmc_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_divmmc_active( snap ) )
    settings_current.divmmc_enabled = 1;
}

static void
divmmc_from_snapshot( libspectrum_snap *snap )
{
  size_t i;

  if( !libspectrum_snap_divmmc_active( snap ) ) return;

  settings_current.divmmc_wp =
    libspectrum_snap_divmmc_eprom_writeprotect( snap );
  divxxx_control_write_internal( divmmc_state, libspectrum_snap_divmmc_control( snap ) );

  if( libspectrum_snap_divmmc_eprom( snap, 0 ) ) {
    memcpy( divxxx_get_eprom( divmmc_state ),
	    libspectrum_snap_divmmc_eprom( snap, 0 ), DIVMMC_PAGE_LENGTH );
  }

  for( i = 0; i < libspectrum_snap_divmmc_pages( snap ); i++ )
    if( libspectrum_snap_divmmc_ram( snap, i ) ) {
      libspectrum_byte *ram = divxxx_get_ram( divmmc_state, i );
      memcpy( ram, libspectrum_snap_divmmc_ram( snap, i ), DIVMMC_PAGE_LENGTH );
    }

  if( libspectrum_snap_divmmc_paged( snap ) ) {
    divxxx_page( divmmc_state );
  } else {
    divxxx_unpage( divmmc_state );
  }
}

static void
divmmc_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.divmmc_enabled ) return;

  libspectrum_snap_set_divmmc_active( snap, 1 );
  libspectrum_snap_set_divmmc_eprom_writeprotect( snap,
                                                  settings_current.divmmc_wp );
  libspectrum_snap_set_divmmc_paged( snap, divxxx_get_active( divmmc_state ) );
  libspectrum_snap_set_divmmc_control( snap, divxxx_get_control( divmmc_state ) );

  buffer = libspectrum_new( libspectrum_byte, DIVMMC_PAGE_LENGTH );

  memcpy( buffer, divxxx_get_eprom( divmmc_state ), DIVMMC_PAGE_LENGTH );
  libspectrum_snap_set_divmmc_eprom( snap, 0, buffer );

  libspectrum_snap_set_divmmc_pages( snap, DIVMMC_PAGES );

  for( i = 0; i < DIVMMC_PAGES; i++ ) {

    buffer = libspectrum_new( libspectrum_byte, DIVMMC_PAGE_LENGTH );

    memcpy( buffer, divxxx_get_ram( divmmc_state, i ), DIVMMC_PAGE_LENGTH );
    libspectrum_snap_set_divmmc_ram( snap, i, buffer );
  }
}

static void
divmmc_activate( void )
{
  divxxx_activate( divmmc_state );
}

static libspectrum_dword
get_control_register( void )
{
  return divxxx_get_control( divmmc_state );
}

static void
set_control_register( libspectrum_dword value )
{
  divxxx_control_write_internal( divmmc_state, value & 0xff );
}

int
divmmc_unittest( void )
{
  int r = 0;
  int eprom_memory_source = divxxx_get_eprom_memory_source( divmmc_state );
  int ram_memory_source = divxxx_get_ram_memory_source( divmmc_state );

  divmmc_set_automap( 1 );

  divmmc_control_write( 0x00e3, 0x80 );
  r += unittests_assert_8k_page( 0x0000, eprom_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x83 );
  r += unittests_assert_8k_page( 0x0000, eprom_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 3 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x40 );
  r += unittests_assert_8k_page( 0x0000, ram_memory_source, 3 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x02 );
  r += unittests_assert_8k_page( 0x0000, ram_memory_source, 3 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 2 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  /* We have only 128 Kb of RAM (16 x 8 Kb pages), so setting all of bits 0-5
     results in page 15 being selected */
  divmmc_control_write( 0x00e3, 0x3f );
  r += unittests_assert_8k_page( 0x0000, ram_memory_source, 3 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 15 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_set_automap( 0 );

  r += unittests_paging_test_48( 2 );

  return r;
}
