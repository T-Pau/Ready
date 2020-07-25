/* zxatasp.c: ZXATASP interface routines
   Copyright (c) 2003-2017 Garry Lancaster, Philip Kendall
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
#include "zxatasp.h"

/* A 16KB memory chunk accessible by the Z80 when /ROMCS is low */
static memory_page zxatasp_memory_map_romcs[MEMORY_PAGES_IN_16K];
static int zxatasp_memory_source;

/*
  TBD: Allow memory size selection (128K/512K)
  TBD: Should support for secondary channel be removed?
       No software ever supported it, and v2.0+ boards don't have it.
*/


/* Debugger events */
static const char * const event_type_string = "zxatasp";
static int page_event, unpage_event;

/* Private function prototypes */

static libspectrum_byte zxatasp_portA_read( libspectrum_word port,
					    libspectrum_byte *attached );
static void zxatasp_portA_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_portB_read( libspectrum_word port,
					    libspectrum_byte *attached );
static void zxatasp_portB_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_portC_read( libspectrum_word port,
					    libspectrum_byte *attached );
static void zxatasp_portC_write( libspectrum_word port,
				 libspectrum_byte data );
static libspectrum_byte zxatasp_control_read( libspectrum_word port,
					      libspectrum_byte *attached );
static void zxatasp_control_write( libspectrum_word port,
				   libspectrum_byte data );
static void zxatasp_resetports( void );
static void set_zxatasp_bank( int bank );

static void zxatasp_readide( libspectrum_ide_channel *chn,
			     libspectrum_ide_register idereg );
static void zxatasp_writeide( libspectrum_ide_channel *chn,
			      libspectrum_ide_register idereg );
static void zxatasp_activate( void );

/* Data */

static const periph_port_t zxatasp_ports[] = {
  { 0x039f, 0x009f, zxatasp_portA_read, zxatasp_portA_write },
  { 0x039f, 0x019f, zxatasp_portB_read, zxatasp_portB_write },
  { 0x039f, 0x029f, zxatasp_portC_read, zxatasp_portC_write },
  { 0x039f, 0x039f, zxatasp_control_read, zxatasp_control_write },
  { 0, 0, NULL, NULL }
};

static const periph_t zxatasp_periph = {
  /* .option = */ &settings_current.zxatasp_active,
  /* .ports = */ zxatasp_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ zxatasp_activate,
};

static libspectrum_byte zxatasp_control;
static libspectrum_byte zxatasp_portA;
static libspectrum_byte zxatasp_portB;
static libspectrum_byte zxatasp_portC;
static size_t current_page;

static libspectrum_ide_channel *zxatasp_idechn0;
static libspectrum_ide_channel *zxatasp_idechn1;

#define ZXATASP_PAGES 32
#define ZXATASP_PAGE_LENGTH 0x4000
static libspectrum_byte *ZXATASPMEM[ ZXATASP_PAGES ];
static int memory_allocated = 0;

static const size_t ZXATASP_NOT_PAGED = 0xff;

/* We're ignoring all mode bits and only emulating mode 0, basic I/O */
static const libspectrum_byte MC8255_PORT_C_LOW_IO  = 0x01;
static const libspectrum_byte MC8255_PORT_B_IO      = 0x02;
static const libspectrum_byte MC8255_PORT_C_HI_IO   = 0x08;
static const libspectrum_byte MC8255_PORT_A_IO      = 0x10;
static const libspectrum_byte MC8255_SETMODE        = 0x80;

static const libspectrum_byte ZXATASP_IDE_REG       = 0x07;
static const libspectrum_byte ZXATASP_RAM_BANK      = 0x1f;
static const libspectrum_byte ZXATASP_IDE_WR        = 0x08;
static const libspectrum_byte ZXATASP_IDE_RD        = 0x10;
static const libspectrum_byte ZXATASP_IDE_PRIMARY   = 0x20;
static const libspectrum_byte ZXATASP_RAM_LATCH     = 0x40;
static const libspectrum_byte ZXATASP_RAM_DISABLE   = 0x80;
static const libspectrum_byte ZXATASP_IDE_SECONDARY = 0x80;

#define ZXATASP_READ_PRIMARY( x )    \
  ( ( x & ( ZXATASP_IDE_PRIMARY   | ZXATASP_RAM_LATCH |        \
            ZXATASP_IDE_RD        | ZXATASP_IDE_WR      ) ) == \
          ( ZXATASP_IDE_PRIMARY   | ZXATASP_IDE_RD )             )
#define ZXATASP_WRITE_PRIMARY( x )   \
  ( ( x & ( ZXATASP_IDE_PRIMARY   | ZXATASP_RAM_LATCH |        \
            ZXATASP_IDE_RD        | ZXATASP_IDE_WR      ) ) == \
          ( ZXATASP_IDE_PRIMARY   | ZXATASP_IDE_WR )             )
#define ZXATASP_READ_SECONDARY( x )  \
  ( ( x & ( ZXATASP_IDE_SECONDARY | ZXATASP_RAM_LATCH |        \
            ZXATASP_IDE_RD        | ZXATASP_IDE_WR      ) ) == \
          ( ZXATASP_IDE_SECONDARY | ZXATASP_IDE_RD )             )
#define ZXATASP_WRITE_SECONDARY( x ) \
  ( ( x & ( ZXATASP_IDE_SECONDARY | ZXATASP_RAM_LATCH |        \
            ZXATASP_IDE_RD        | ZXATASP_IDE_WR      ) ) == \
          ( ZXATASP_IDE_SECONDARY | ZXATASP_IDE_WR )             )

static void zxatasp_reset( int hard_reset );
static void zxatasp_memory_map( void );
static void zxatasp_snapshot_enabled( libspectrum_snap *snap );
static void zxatasp_from_snapshot( libspectrum_snap *snap );
static void zxatasp_to_snapshot( libspectrum_snap *snap );

static module_info_t zxatasp_module_info = {

  /* .reset = */ zxatasp_reset,
  /* .romcs = */ zxatasp_memory_map,
  /* .snapshot_enabled = */ zxatasp_snapshot_enabled,
  /* .snapshot_from = */ zxatasp_from_snapshot,
  /* .snapshot_to = */ zxatasp_to_snapshot,

};

/* Housekeeping functions */

static int
zxatasp_init( void *context )
{
  int error, i;

  zxatasp_idechn0 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );
  zxatasp_idechn1 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );

  error = ide_init( zxatasp_idechn0,
                    settings_current.zxatasp_master_file,
                    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
                    settings_current.zxatasp_slave_file,
                    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT );
  if( error ) return error;

  module_register( &zxatasp_module_info );

  zxatasp_memory_source = memory_source_register( "ZXATASP" );
  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    zxatasp_memory_map_romcs[i].source = zxatasp_memory_source;

  periph_register( PERIPH_TYPE_ZXATASP, &zxatasp_periph );
  periph_register_paging_events( event_type_string, &page_event,
				 &unpage_event );

  return 0;
}

static void
zxatasp_end( void )
{
  libspectrum_ide_free( zxatasp_idechn0 );
  libspectrum_ide_free( zxatasp_idechn1 );
}

void
zxatasp_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_ZXATASP, dependencies,
                            ARRAY_SIZE( dependencies ), zxatasp_init, NULL,
                            zxatasp_end );
}

static void
zxatasp_reset( int hard_reset GCC_UNUSED )
{
  if( !settings_current.zxatasp_active ) return;
  
  machine_current->ram.romcs = 1;

  set_zxatasp_bank( 0 ); current_page = 0;
  machine_current->memory_map();

  zxatasp_control = MC8255_SETMODE | MC8255_PORT_A_IO | MC8255_PORT_B_IO |
                    MC8255_PORT_C_HI_IO | MC8255_PORT_C_LOW_IO;
  zxatasp_resetports();

  libspectrum_ide_reset( zxatasp_idechn0 );
  libspectrum_ide_reset( zxatasp_idechn1 );
}

int
zxatasp_insert( const char *filename, libspectrum_ide_unit unit )
{
  return ide_master_slave_insert(
    zxatasp_idechn0, unit, filename,
    &settings_current.zxatasp_master_file,
    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
    &settings_current.zxatasp_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT );
}

int
zxatasp_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( zxatasp_idechn0, unit );

  return error;
}

int
zxatasp_eject( libspectrum_ide_unit unit )
{
  return ide_master_slave_eject(
    zxatasp_idechn0, unit,
    &settings_current.zxatasp_master_file,
    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
    &settings_current.zxatasp_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT );
}

/* Port read/writes */

libspectrum_byte
zxatasp_portA_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  
  return zxatasp_portA;
}

static void
zxatasp_portA_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !( zxatasp_control & MC8255_PORT_A_IO ) ) zxatasp_portA = data;
}

libspectrum_byte
zxatasp_portB_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  
  return zxatasp_portB;
}

static void
zxatasp_portB_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !( zxatasp_control & MC8255_PORT_B_IO ) ) zxatasp_portB = data;
}

libspectrum_byte
zxatasp_portC_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  
  return zxatasp_portC;
}

static void
zxatasp_portC_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  libspectrum_byte oldC = zxatasp_portC;
  libspectrum_byte newC;
  
  /* Determine new port C value, dependent on I/O modes */
  newC = ( zxatasp_control & MC8255_PORT_C_LOW_IO )
            ? ( oldC & 0x0f ) : ( data & 0x0f );
            
  newC |= ( zxatasp_control & MC8255_PORT_C_HI_IO )
            ? ( oldC & 0xf0 ) : ( data & 0xf0 );
            
  /* Set the new port value */
  zxatasp_portC = newC;
  
  /* No action can occur if high part of port C is in input mode */
  if( zxatasp_control & MC8255_PORT_C_HI_IO ) return;
  
  /* Check for any I/O action */
  if(  ( ZXATASP_READ_PRIMARY( newC ) ) &&
      !( ZXATASP_READ_PRIMARY( oldC ) )   ) {
    zxatasp_readide( zxatasp_idechn0, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_READ_SECONDARY( newC ) ) &&
      !( ZXATASP_READ_SECONDARY( oldC ) )   ) {
    zxatasp_readide( zxatasp_idechn1, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_WRITE_PRIMARY( newC ) ) &&
      !( ZXATASP_WRITE_PRIMARY( oldC ) )   ) {
    zxatasp_writeide( zxatasp_idechn0, ( newC & ZXATASP_IDE_REG ) );
    return;
  }
  
  if(  ( ZXATASP_WRITE_SECONDARY( newC ) ) &&
      !( ZXATASP_WRITE_SECONDARY( oldC ) )   ) {
    zxatasp_writeide( zxatasp_idechn1, ( newC & ZXATASP_IDE_REG ) );
    return;
  }

  if( newC & ZXATASP_RAM_LATCH ) {
    int was_paged = machine_current->ram.romcs;

    set_zxatasp_bank( newC & ZXATASP_RAM_BANK );

    if( newC & ZXATASP_RAM_DISABLE ) {
      machine_current->ram.romcs = 0;
      current_page = ZXATASP_NOT_PAGED;
      if( was_paged ) debugger_event( unpage_event );
    } else {
      machine_current->ram.romcs = 1;
      current_page = newC & ZXATASP_RAM_BANK;
      if( !was_paged ) debugger_event( page_event );
    }

    machine_current->memory_map();
    return;
  }
}

libspectrum_byte
zxatasp_control_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  
  return zxatasp_control;
}

static void
zxatasp_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( data & MC8255_SETMODE ) {
    /* Set the control word and reset the ports */
    zxatasp_control = data;
    zxatasp_resetports();

  } else {

    /* Set or reset a bit of port C */
    libspectrum_byte bit = (data >> 1) & 7;
    libspectrum_byte newC = zxatasp_portC;
      
    if( data & 1 ) {
      newC |=  ( 1 << bit );
    } else {
      newC &= ~( 1 << bit );
    }
    
    zxatasp_portC_write( 0, newC );
  }
}

static void
zxatasp_resetports( void )
{
  /* In input mode, ports will return 0xff unless IDE is active,
     which it won't be just after a reset. Output ports are always
     reset to zero.
  */
  zxatasp_portA = (zxatasp_control & MC8255_PORT_A_IO) ? 0xff : 0x00;
  zxatasp_portB = (zxatasp_control & MC8255_PORT_B_IO) ? 0xff : 0x00;
  zxatasp_portC = (zxatasp_control & MC8255_PORT_C_LOW_IO) ? 0x0f : 0x00;
  zxatasp_portC |= (zxatasp_control & MC8255_PORT_C_HI_IO) ? 0xf0 : 0x00;
}

static void
set_zxatasp_bank( int bank )
{
  memory_page *page;
  size_t i, offset;

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ ) {

    page = &zxatasp_memory_map_romcs[i];
    offset = i * MEMORY_PAGE_SIZE;

    page->page = &ZXATASPMEM[ bank ][ offset ];
    page->writable = !settings_current.zxatasp_wp;
    page->contended = 0;
    
    page->page_num = bank;
    page->offset = offset;
  }
}

/* IDE access */

static void
zxatasp_readide(libspectrum_ide_channel *chn,
                libspectrum_ide_register idereg)
{
  libspectrum_byte dataHi, dataLo;
  
  dataLo = libspectrum_ide_read( chn, idereg );
  
  if( idereg == LIBSPECTRUM_IDE_REGISTER_DATA ) {
    dataHi = libspectrum_ide_read( chn, idereg );
  } else {
    dataHi = 0xff;
  }

  if( zxatasp_control & MC8255_PORT_A_IO ) zxatasp_portA = dataLo;
  if( zxatasp_control & MC8255_PORT_B_IO ) zxatasp_portB = dataHi;
}

static void
zxatasp_writeide(libspectrum_ide_channel *chn,
                 libspectrum_ide_register idereg)
{
  libspectrum_byte dataHi, dataLo;

  dataLo = ( zxatasp_control & MC8255_PORT_A_IO ) ? 0xff : zxatasp_portA;
  dataHi = ( zxatasp_control & MC8255_PORT_B_IO ) ? 0xff : zxatasp_portB;
  
  libspectrum_ide_write( chn, idereg, dataLo );
  
  if( idereg == LIBSPECTRUM_IDE_REGISTER_DATA )
    libspectrum_ide_write( chn, idereg, dataHi );
}

static void
zxatasp_memory_map( void )
{
  int i, writable, map_read;

  if( !settings_current.zxatasp_active ) return;

  if( settings_current.zxatasp_wp &&
      ( zxatasp_memory_map_romcs[0].page_num & 1 ) ) {
    writable = 0;
  } else {
    writable = 1;
  }

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    zxatasp_memory_map_romcs[i].writable = writable;

  map_read = !settings_current.zxatasp_upload;
  memory_map_16k_read_write( 0x000, zxatasp_memory_map_romcs, 0, map_read, 1 );
}

static void
zxatasp_snapshot_enabled( libspectrum_snap *snap )
{
  settings_current.zxatasp_active = libspectrum_snap_zxatasp_active( snap );
}

static void
zxatasp_from_snapshot( libspectrum_snap *snap )
{
  size_t i, page;

  if( !libspectrum_snap_zxatasp_active( snap ) ) return;

  settings_current.zxatasp_upload = libspectrum_snap_zxatasp_upload( snap );
  settings_current.zxatasp_wp = libspectrum_snap_zxatasp_writeprotect( snap );

  zxatasp_portA   = libspectrum_snap_zxatasp_port_a ( snap );
  zxatasp_portB   = libspectrum_snap_zxatasp_port_b ( snap );
  zxatasp_portC   = libspectrum_snap_zxatasp_port_c ( snap );
  zxatasp_control = libspectrum_snap_zxatasp_control( snap );

  page = libspectrum_snap_zxatasp_current_page( snap );

  if( page != ZXATASP_NOT_PAGED ) {
    machine_current->ram.romcs = 1;
    set_zxatasp_bank( page );
  }

  for( i = 0; i < libspectrum_snap_zxatasp_pages( snap ); i++ )
    if( libspectrum_snap_zxatasp_ram( snap, i ) )
      memcpy( ZXATASPMEM[ i ], libspectrum_snap_zxatasp_ram( snap, i ),
	      ZXATASP_PAGE_LENGTH );

  machine_current->memory_map();
}

static void
zxatasp_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.zxatasp_active ) return;

  libspectrum_snap_set_zxatasp_active( snap, 1 );
  libspectrum_snap_set_zxatasp_upload( snap, settings_current.zxatasp_upload );
  libspectrum_snap_set_zxatasp_writeprotect( snap,
					     settings_current.zxatasp_wp );

  libspectrum_snap_set_zxatasp_port_a( snap, zxatasp_portA );
  libspectrum_snap_set_zxatasp_port_b( snap, zxatasp_portB );
  libspectrum_snap_set_zxatasp_port_c( snap, zxatasp_portC );
  libspectrum_snap_set_zxatasp_control( snap, zxatasp_control );
  libspectrum_snap_set_zxatasp_current_page( snap, current_page );

  libspectrum_snap_set_zxatasp_pages( snap, ZXATASP_PAGES );

  for( i = 0; i < ZXATASP_PAGES; i++ ) {

    buffer = libspectrum_new( libspectrum_byte, ZXATASP_PAGE_LENGTH );

    memcpy( buffer, ZXATASPMEM[ i ], ZXATASP_PAGE_LENGTH );
    libspectrum_snap_set_zxatasp_ram( snap, i, buffer );
  }
}

static void
zxatasp_activate( void )
{
  if( !memory_allocated ) {
    int i;
    libspectrum_byte *memory =
      memory_pool_allocate_persistent( ZXATASP_PAGES * ZXATASP_PAGE_LENGTH, 1 );
    for( i = 0; i < ZXATASP_PAGES; i++ )
      ZXATASPMEM[i] = memory + i * ZXATASP_PAGE_LENGTH;
    memory_allocated = 1;
  }
}

int
zxatasp_unittest( void )
{
  int r = 0;
  int old_setting = settings_current.zxatasp_active;

  settings_current.zxatasp_active = 1;

  zxatasp_portC_write( 0xfeff, 0x40 );
  r += unittests_assert_16k_page( 0x0000, zxatasp_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  zxatasp_portC_write( 0xfeff, 0x41 );
  r += unittests_assert_16k_page( 0x0000, zxatasp_memory_source, 1 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  zxatasp_portC_write( 0xfeff, 0x5f );
  r += unittests_assert_16k_page( 0x0000, zxatasp_memory_source, 31 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  zxatasp_portC_write( 0xfeff, 0xc0 );
  r += unittests_paging_test_48( 2 );

  settings_current.zxatasp_active = old_setting;

  return r;
}

