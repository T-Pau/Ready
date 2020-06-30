/* zxcf.c: ZXCF interface routines
   Copyright (c) 2003-2016 Garry Lancaster, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 Sergio Baldoví
		 
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

#include <string.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "ide.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "unittests/unittests.h"
#include "zxcf.h"

/* A 16KB memory chunk accessible by the Z80 when /ROMCS is low */
static memory_page zxcf_memory_map_romcs[MEMORY_PAGES_IN_16K];
static int zxcf_memory_source;

/*
  TBD: Allow memory size selection (128K/512K/1024K)
  TBD: Add secondary channel
*/


/* Private function prototype */

static void set_zxcf_bank( int bank );
static libspectrum_byte zxcf_memctl_read( libspectrum_word port,
					  libspectrum_byte *attached );
static void zxcf_memctl_write( libspectrum_word port, libspectrum_byte data );
static libspectrum_byte zxcf_ide_read( libspectrum_word port, libspectrum_byte *attached );
static void zxcf_ide_write( libspectrum_word port, libspectrum_byte data );
static void zxcf_activate( void );

/* Data */

static const periph_port_t zxcf_ports[] = {
  { 0x10f4, 0x10b4, zxcf_memctl_read, zxcf_memctl_write },
  { 0x10f4, 0x00b4, zxcf_ide_read, zxcf_ide_write },
  { 0, 0, NULL, NULL }
};

static const periph_t zxcf_periph = {
  /* .option = */ &settings_current.zxcf_active,
  /* .ports = */ zxcf_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ zxcf_activate,
};

static int zxcf_writeenable;

static libspectrum_ide_channel *zxcf_idechn;

#define ZXCF_PAGES 64
#define ZXCF_PAGE_LENGTH 0x4000
static libspectrum_byte *ZXCFMEM[ ZXCF_PAGES ];
static int memory_allocated = 0;

static libspectrum_byte last_memctl;

static void zxcf_reset( int hard_reset );
static void zxcf_memory_map( void );
static void zxcf_snapshot_enabled( libspectrum_snap *snap );
static void zxcf_from_snapshot( libspectrum_snap *snap );
static void zxcf_to_snapshot( libspectrum_snap *snap );

static module_info_t zxcf_module_info = {

  /* .reset = */ zxcf_reset,
  /* .romcs = */ zxcf_memory_map,
  /* .snapshot_enabled = */ zxcf_snapshot_enabled,
  /* .snapshot_from = */ zxcf_from_snapshot,
  /* .snapshot_to = */ zxcf_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "zxcf";
static int page_event, unpage_event;

/* Housekeeping functions */

static int
zxcf_init( void *context )
{
  int error, i;

  last_memctl = 0x00;
                                
  zxcf_idechn = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 0 );

  if( settings_current.zxcf_pri_file ) {
    error = libspectrum_ide_insert( zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
				    settings_current.zxcf_pri_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 1 );
  }

  module_register( &zxcf_module_info );

  zxcf_memory_source = memory_source_register( "ZXCF" );
  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    zxcf_memory_map_romcs[i].source = zxcf_memory_source;

  periph_register( PERIPH_TYPE_ZXCF, &zxcf_periph );
  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
zxcf_end( void )
{
  libspectrum_ide_free( zxcf_idechn );
}

void
zxcf_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_ZXCF, dependencies,
                            ARRAY_SIZE( dependencies ), zxcf_init, NULL,
                            zxcf_end );
}

static void
zxcf_reset( int hard_reset GCC_UNUSED )
{
  if( !settings_current.zxcf_active ) return;

  machine_current->ram.romcs = 1;

  set_zxcf_bank( 0 );
  zxcf_writeenable = 0;
  machine_current->memory_map();

  libspectrum_ide_reset( zxcf_idechn );
}

int
zxcf_insert( const char *filename )
{
  return ide_insert( filename, zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
		     &settings_current.zxcf_pri_file,
		     UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT );
}

int
zxcf_commit( void )
{
  int error;

  error = libspectrum_ide_commit( zxcf_idechn, LIBSPECTRUM_IDE_MASTER );

  return error;
}

int
zxcf_eject( void )
{
  return ide_eject( zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
		    &settings_current.zxcf_pri_file,
		    UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT );
}

static void
set_zxcf_bank( int bank )
{
  memory_page *page;
  size_t i, offset;

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ ) {

    page = &zxcf_memory_map_romcs[i];
    offset = i * MEMORY_PAGE_SIZE;
    
    page->page = &ZXCFMEM[ bank ][ offset ];
    page->writable = zxcf_writeenable;
    page->contended = 0;
    
    page->page_num = bank;
    page->offset = offset;
  }
}  

/* Port read/writes */

static libspectrum_byte
zxcf_memctl_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */

  return 0xff;
}

static void
zxcf_memctl_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  int was_paged = machine_current->ram.romcs;

  last_memctl = data;

  /* Bit 7 MEMOFF: 0=mem on, 1 =mem off */
  machine_current->ram.romcs = ( data & 0x80 ) ? 0 : 1;

  /* Bit 6 /MWRPROT: 0=mem protected, 1=mem writable */
  zxcf_writeenable = ( data & 0x40 ) ? 1 : 0;
  
  /* Bits 5-0: MEMBANK */
  set_zxcf_bank( data & 0x3f );

  machine_current->memory_map();

  if( machine_current->ram.romcs != was_paged )
    debugger_event( machine_current->ram.romcs ? page_event : unpage_event );
}

libspectrum_byte
zxcf_last_memctl( void )
{
  return last_memctl;
}

static libspectrum_byte
zxcf_ide_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_ide_register idereg = ( port >> 8 ) & 0x07;
  
  *attached = 0xff; /* TODO: check this */

  return libspectrum_ide_read( zxcf_idechn, idereg ); 
}

static void
zxcf_ide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  idereg = ( port >> 8 ) & 0x07;
  libspectrum_ide_write( zxcf_idechn, idereg, data ); 
}

static void
zxcf_memory_map( void )
{
  int map_read;

  if( !settings_current.zxcf_active ) return;

  map_read = !settings_current.zxcf_upload;
  memory_map_16k_read_write( 0x0000, zxcf_memory_map_romcs, 0, map_read, 1 );
}

static void
zxcf_snapshot_enabled( libspectrum_snap *snap )
{
  settings_current.zxcf_active = libspectrum_snap_zxcf_active( snap );
}

static void
zxcf_from_snapshot( libspectrum_snap *snap )
{
  size_t i;

  if( !libspectrum_snap_zxcf_active( snap ) ) return;

  settings_current.zxcf_upload = libspectrum_snap_zxcf_upload( snap );

  zxcf_memctl_write( 0x10bf, libspectrum_snap_zxcf_memctl( snap ) );

  for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ )
    if( libspectrum_snap_zxcf_ram( snap, i ) )
      memcpy( ZXCFMEM[ i ], libspectrum_snap_zxcf_ram( snap, i ),
	      ZXCF_PAGE_LENGTH );
}

static void
zxcf_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.zxcf_active ) return;

  libspectrum_snap_set_zxcf_active( snap, 1 );
  libspectrum_snap_set_zxcf_upload( snap, settings_current.zxcf_upload );
  libspectrum_snap_set_zxcf_memctl( snap, last_memctl );
  libspectrum_snap_set_zxcf_pages( snap, ZXCF_PAGES );

  for( i = 0; i < ZXCF_PAGES; i++ ) {

    buffer = libspectrum_new( libspectrum_byte, ZXCF_PAGE_LENGTH );

    memcpy( buffer, ZXCFMEM[ i ], ZXCF_PAGE_LENGTH );
    libspectrum_snap_set_zxcf_ram( snap, i, buffer );
  }
}

static void
zxcf_activate( void )
{
  if( !memory_allocated ) {
    int i;
    libspectrum_byte *memory =
      memory_pool_allocate_persistent( ZXCF_PAGES * ZXCF_PAGE_LENGTH, 1 );
    for( i = 0; i < ZXCF_PAGES; i++ )
      ZXCFMEM[i] = memory + i * ZXCF_PAGE_LENGTH;
    memory_allocated = 1;
  }
}

int
zxcf_unittest( void )
{
  int r = 0;

  int old_setting = settings_current.zxcf_active;

  settings_current.zxcf_active = 1;

  zxcf_memctl_write( 0x10b4, 0x00 );
  r += unittests_assert_16k_page( 0x0000, zxcf_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  zxcf_memctl_write( 0x10b4, 0x3f );
  r += unittests_assert_16k_page( 0x0000, zxcf_memory_source, 63 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  zxcf_memctl_write( 0x10b4, 0x80 );
  r += unittests_paging_test_48( 2 );

  settings_current.zxcf_active = old_setting;

  return r;
}

