/* divide.c: DivIDE interface routines
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
#include "divide.h"
#include "divxxx.h"

/* Private function prototypes */

static libspectrum_byte divide_ide_read( libspectrum_word port, libspectrum_byte *attached );
static void divide_ide_write( libspectrum_word port, libspectrum_byte data );
static void divide_control_write( libspectrum_word port, libspectrum_byte data );
static libspectrum_ide_register port_to_ide_register( libspectrum_byte port );
static void divide_activate( void );

/* Data */

static const periph_port_t divide_ports[] = {
  { 0x00e3, 0x00a3, divide_ide_read, divide_ide_write },
  { 0x00ff, 0x00e3, NULL, divide_control_write },
  { 0, 0, NULL, NULL }
};

static const periph_t divide_periph = {
  /* .option = */ &settings_current.divide_enabled,
  /* .ports = */ divide_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ divide_activate,
};

int divide_automapping_enabled = 0;
static divxxx_t *divide_state;

static libspectrum_ide_channel *divide_idechn0;
static libspectrum_ide_channel *divide_idechn1;

#define DIVIDE_PAGES 4
#define DIVIDE_PAGE_LENGTH 0x2000

static void divide_reset( int hard_reset );
static void divide_memory_map( void );
static void divide_enabled_snapshot( libspectrum_snap *snap );
static void divide_from_snapshot( libspectrum_snap *snap );
static void divide_to_snapshot( libspectrum_snap *snap );

static module_info_t divide_module_info = {

  /* .reset = */ divide_reset,
  /* .romcs = */ divide_memory_map,
  /* .snapshot_enabled = */ divide_enabled_snapshot,
  /* .snapshot_from = */ divide_from_snapshot,
  /* .snapshot_to = */ divide_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "divide";

/* Housekeeping functions */

static int
divide_init( void *context )
{
  int error;

  divide_idechn0 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );
  divide_idechn1 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );
  
  error = ide_init( divide_idechn0,
		    settings_current.divide_master_file,
		    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT,
		    settings_current.divide_slave_file,
		    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT );
  if( error ) return error;

  module_register( &divide_module_info );

  periph_register( PERIPH_TYPE_DIVIDE, &divide_periph );

  divide_state = divxxx_alloc( "DivIDE EPROM", DIVIDE_PAGES, "DivIDE RAM",
      event_type_string, &settings_current.divide_enabled,
      &settings_current.divide_wp );

  return 0;
}

static void
divide_end( void )
{
  divxxx_free( divide_state );
  libspectrum_ide_free( divide_idechn0 );
  libspectrum_ide_free( divide_idechn1 );
}

void
divide_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_DIVIDE, dependencies,
                            ARRAY_SIZE( dependencies ), divide_init, NULL,
                            divide_end );
}

/* DivIDE does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
static void
divide_reset( int hard_reset )
{
  divxxx_reset( divide_state, hard_reset );

  libspectrum_ide_reset( divide_idechn0 );
  libspectrum_ide_reset( divide_idechn1 );
}

int
divide_insert( const char *filename, libspectrum_ide_unit unit )
{
  return ide_master_slave_insert(
    divide_idechn0, unit, filename,
    &settings_current.divide_master_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT,
    &settings_current.divide_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT );
}

int
divide_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( divide_idechn0, unit );

  return error;
}

int
divide_eject( libspectrum_ide_unit unit )
{
  return ide_master_slave_eject(
    divide_idechn0, unit,
    &settings_current.divide_master_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT,
    &settings_current.divide_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT );
}

/* Port read/writes */

static libspectrum_ide_register
port_to_ide_register( libspectrum_byte port )
{
  switch( port & 0xff ) {
    case 0xa3:
      return LIBSPECTRUM_IDE_REGISTER_DATA;
    case 0xa7:
      return LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE;
    case 0xab:
      return LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT;
    case 0xaf:
      return LIBSPECTRUM_IDE_REGISTER_SECTOR;
    case 0xb3:
      return LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW;
    case 0xb7:
      return LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH;
    case 0xbb:
      return LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE;
    default: /* 0xbf */
      return LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS;
  }
}

libspectrum_byte
divide_ide_read( libspectrum_word port, libspectrum_byte *attached )
{
  int ide_register;

  *attached = 0xff; /* TODO: check this */
  ide_register = port_to_ide_register( port );

  return libspectrum_ide_read( divide_idechn0, ide_register );
}

static void
divide_ide_write( libspectrum_word port, libspectrum_byte data )
{
  int ide_register;
  
  ide_register = port_to_ide_register( port );
  
  libspectrum_ide_write( divide_idechn0, ide_register, data );
}

static void
divide_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  divxxx_control_write( divide_state, data );
}

void
divide_set_automap( int state )
{
  divxxx_set_automap( divide_state, state );
}

void
divide_refresh_page_state( void )
{
  divxxx_refresh_page_state( divide_state );
}

void
divide_memory_map( void )
{
  divxxx_memory_map( divide_state );
}

static void
divide_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.divide_enabled = libspectrum_snap_divide_active( snap );
}

static void
divide_from_snapshot( libspectrum_snap *snap )
{
  size_t i;

  if( !libspectrum_snap_divide_active( snap ) ) return;

  settings_current.divide_wp =
    libspectrum_snap_divide_eprom_writeprotect( snap );
  divxxx_control_write_internal( divide_state, libspectrum_snap_divide_control( snap ) );

  if( libspectrum_snap_divide_eprom( snap, 0 ) ) {
    memcpy( divxxx_get_eprom( divide_state ),
	    libspectrum_snap_divide_eprom( snap, 0 ), DIVIDE_PAGE_LENGTH );
  }

  for( i = 0; i < libspectrum_snap_divide_pages( snap ); i++ )
    if( libspectrum_snap_divide_ram( snap, i ) ) {
      libspectrum_byte *ram = divxxx_get_ram( divide_state, i );
      memcpy( ram, libspectrum_snap_divide_ram( snap, i ), DIVIDE_PAGE_LENGTH );
    }

  if( libspectrum_snap_divide_paged( snap ) ) {
    divxxx_page( divide_state );
  } else {
    divxxx_unpage( divide_state );
  }
}

static void
divide_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.divide_enabled ) return;

  libspectrum_snap_set_divide_active( snap, 1 );
  libspectrum_snap_set_divide_eprom_writeprotect( snap,
                                                  settings_current.divide_wp );
  libspectrum_snap_set_divide_paged( snap, divxxx_get_active( divide_state ) );
  libspectrum_snap_set_divide_control( snap, divxxx_get_control( divide_state ) );

  buffer = libspectrum_new( libspectrum_byte, DIVIDE_PAGE_LENGTH );

  memcpy( buffer, divxxx_get_eprom( divide_state ), DIVIDE_PAGE_LENGTH );
  libspectrum_snap_set_divide_eprom( snap, 0, buffer );

  libspectrum_snap_set_divide_pages( snap, DIVIDE_PAGES );

  for( i = 0; i < DIVIDE_PAGES; i++ ) {

    buffer = libspectrum_new( libspectrum_byte, DIVIDE_PAGE_LENGTH );

    memcpy( buffer, divxxx_get_ram( divide_state, i ), DIVIDE_PAGE_LENGTH );
    libspectrum_snap_set_divide_ram( snap, i, buffer );
  }
}

static void
divide_activate( void )
{
  divxxx_activate( divide_state );
}

int
divide_unittest( void )
{
  int r = 0;
  int eprom_memory_source = divxxx_get_eprom_memory_source( divide_state );
  int ram_memory_source = divxxx_get_ram_memory_source( divide_state );

  divide_set_automap( 1 );

  divide_control_write( 0x00e3, 0x80 );
  r += unittests_assert_8k_page( 0x0000, eprom_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divide_control_write( 0x00e3, 0x83 );
  r += unittests_assert_8k_page( 0x0000, eprom_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 3 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divide_control_write( 0x00e3, 0x40 );
  r += unittests_assert_8k_page( 0x0000, ram_memory_source, 3 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divide_control_write( 0x00e3, 0x02 );
  r += unittests_assert_8k_page( 0x0000, ram_memory_source, 3 );
  r += unittests_assert_8k_page( 0x2000, ram_memory_source, 2 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divide_set_automap( 0 );

  r += unittests_paging_test_48( 2 );

  return r;
}
