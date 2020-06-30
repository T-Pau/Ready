/* if2.c: Interface 2 cartridge handling routines
   Copyright (c) 2003-2016 Darren Salt, Fredrick Meunier, Philip Kendall

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

   Darren: linux@youmustbejoking.demon.co.uk
   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <string.h>

#include "if2.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "unittests/unittests.h"

/* A 16KB memory chunk accessible by the Z80 when /ROMCS is low */
static memory_page if2_memory_map_romcs[MEMORY_PAGES_IN_16K];

/* IF2 cart inserted? */
int if2_active = 0;

/* IF2 memory source */
static int if2_memory_source;

static void if2_reset( int hard_reset );
static void if2_memory_map( void );
static void if2_from_snapshot( libspectrum_snap *snap );
static void if2_to_snapshot( libspectrum_snap *snap );

static module_info_t if2_module_info = {

  /* .reset = */ if2_reset,
  /* .romcs = */ if2_memory_map,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ if2_from_snapshot,
  /* .snapshot_to = */ if2_to_snapshot,

};

static const periph_t if2_periph = {
  /* .option = */ &settings_current.interface2,
  /* .ports = */ NULL,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static int
if2_init( void *context )
{
  int i;
  int if2_source;

  module_register( &if2_module_info );

  if2_source = memory_source_register( "If2" );
  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    if2_memory_map_romcs[i].source = if2_source;

  periph_register( PERIPH_TYPE_INTERFACE2, &if2_periph );

  return 0;
}

void
if2_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_IF2, dependencies,
                            ARRAY_SIZE( dependencies ), if2_init, NULL, NULL );
}

int
if2_insert( const char *filename )
{
  if ( !periph_is_active( PERIPH_TYPE_INTERFACE2 ) ) {
    ui_error( UI_ERROR_ERROR,
	      "This machine does not support the Interface 2" );
    return 1;
  }

  settings_set_string( &settings_current.if2_file, filename );

  machine_reset( 0 );

  return 0;
}

void
if2_eject( void )
{
  if ( !periph_is_active( PERIPH_TYPE_INTERFACE2 ) ) {
    ui_error( UI_ERROR_ERROR,
	      "This machine does not support the Interface 2" );
    return;
  }

  if( settings_current.if2_file ) libspectrum_free( settings_current.if2_file );
  settings_current.if2_file = NULL;

  machine_current->ram.romcs = 0;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 0 );

  machine_reset( 0 );
}

static void
if2_reset( int hard_reset GCC_UNUSED )
{
  if2_active = 0;

  if( !settings_current.if2_file ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 0 );
    return;
  }

  if ( !periph_is_active( PERIPH_TYPE_INTERFACE2 ) ) return;

  if ( machine_load_rom_bank( if2_memory_map_romcs, 0,
			      settings_current.if2_file,
			      NULL, 0x4000 ) )
    return;

  machine_current->ram.romcs = 1;

  if2_active = 1;
  memory_romcs_map();

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );
}

static void
if2_memory_map( void )
{
  if( !if2_active ) return;

  memory_map_romcs_full( if2_memory_map_romcs );
}

static void
if2_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_interface2_active( snap ) ) return;

  if2_active = 1;
  machine_current->ram.romcs = 1;

  if( libspectrum_snap_interface2_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             if2_memory_map_romcs, 0,
                             libspectrum_snap_interface2_rom( snap, 0 ),
                             0x4000,
                             1 ) )
    return;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );

  machine_current->memory_map();
}

static void
if2_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int i;

  if( !if2_active ) return;

  libspectrum_snap_set_interface2_active( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, 0x4000 );

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            if2_memory_map_romcs[ i ].page, MEMORY_PAGE_SIZE );
  libspectrum_snap_set_interface2_rom( snap, 0, buffer );
}

int
if2_unittest( void )
{
  int r = 0;

  if2_active = 1;
  machine_current->memory_map();

  r += unittests_assert_16k_page( 0x0000, if2_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  if2_active = 0;
  machine_current->memory_map();

  r += unittests_paging_test_48( 2 );

  return r;
}
