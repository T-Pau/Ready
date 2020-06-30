/* didaktik.c: Routines for handling the Didaktik 40/80 disk interface
   Copyright (c) 2015-2016 Gergely Szasz, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 Sergio Baldoví
   Copyright (c) 2016 Fredrick Meunier

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

   szaszg@hu.inter.net

*/

#include <config.h>

#include <libspectrum.h>

#include <string.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "didaktik.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "peripherals/printer.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "unittests/unittests.h"
#include "utils.h"
#include "wd_fdc.h"
#include "options.h"	/* needed for get combo options */
#include "z80/z80.h"

#define INTRQ_ENABLED  0x80
#define DATARQ_ENABLED 0x40
/* 2KB RAM */
#define RAM_SIZE 0x0800
/* 14KB ROM */
#define ROM_SIZE 0x3800

static int didaktik_rom_memory_source, didaktik_ram_memory_source;

/* Two memory chunks accessible by the Z80 when /ROMCS is low */
static memory_page didaktik_memory_map_romcs_rom[ MEMORY_PAGES_IN_14K ];
static memory_page didaktik_memory_map_romcs_ram[ MEMORY_PAGES_IN_2K ];

int didaktik80_available = 0;
int didaktik80_active = 0;
int didaktik80_snap = 0;

static wd_fdc *didaktik_fdc;
static fdd_t didaktik_drives[ DIDAKTIK80_NUM_DRIVES ];
static ui_media_drive_info_t didaktik_ui_drives[ DIDAKTIK80_NUM_DRIVES ];

static libspectrum_byte ram[ RAM_SIZE ];

/* AUX byte */
static libspectrum_byte aux_register;

static void didaktik_reset( int hard_reset );
static void didaktik_memory_map( void );
static void didaktik_enabled_snapshot( libspectrum_snap *snap );
static void didaktik_from_snapshot( libspectrum_snap *snap );
static void didaktik_to_snapshot( libspectrum_snap *snap );
static libspectrum_byte didaktik_sr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached );
static void didaktik_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );
static libspectrum_byte didaktik_tr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached );
static void didaktik_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );
static libspectrum_byte didaktik_sec_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached );
static void didaktik_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );
static libspectrum_byte didaktik_dr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached );
static void didaktik_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );
static libspectrum_byte didaktik_8255_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached );
static void didaktik_8255_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );
static void didaktik_aux_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b );

static module_info_t didaktik_module_info = {

  /* .reset = */ didaktik_reset,
  /* .romcs = */ didaktik_memory_map,
  /* .snapshot_enabled = */ didaktik_enabled_snapshot,
  /* .snapshot_from = */ didaktik_from_snapshot,
  /* .snapshot_to = */ didaktik_to_snapshot,

};

static const periph_port_t didaktik_ports[] = {
  /* ---- ---- 1000 0001 */
  { 0x00ff, 0x0081, didaktik_sr_read, didaktik_cr_write },
  /* ---- ---- 1000 0011 */
  { 0x00ff, 0x0083, didaktik_tr_read, didaktik_tr_write },
  /* ---- ---- 1000 0101 */
  { 0x00ff, 0x0085, didaktik_sec_read, didaktik_sec_write },
  /* ---- ---- 1000 0111 */
  { 0x00ff, 0x0087, didaktik_dr_read, didaktik_dr_write },

  /* ---- ---- 0xx- ---- */
  { 0x0080, 0x0000, didaktik_8255_read, didaktik_8255_write },

  /* ---- ---- 1000 1--1 */
  { 0x00f9, 0x0089, NULL, didaktik_aux_write },

  { 0, 0, NULL, NULL }
};

static const periph_t didaktik_periph = {
  /* .option = */ &settings_current.didaktik80,
  /* .ports = */ didaktik_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

/* Debugger events */
static const char * const event_type_string = "didaktik80";
static int page_event, unpage_event;

void
didaktik80_page( void )
{
  didaktik80_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
didaktik80_unpage( void )
{
  didaktik80_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
didaktik_memory_map( void )
{
  if( !didaktik80_active ) return;

  memory_map_romcs_8k( 0x0000, didaktik_memory_map_romcs_rom );
  memory_map_romcs_4k( 0x2000,
                       didaktik_memory_map_romcs_rom + MEMORY_PAGES_IN_8K );
  memory_map_romcs_2k( 0x3000,
                       didaktik_memory_map_romcs_rom + MEMORY_PAGES_IN_12K );
  memory_map_romcs_2k( 0x3800, didaktik_memory_map_romcs_ram );
}

static void
didaktik_set_datarq( struct wd_fdc *f )
{
  if( aux_register & DATARQ_ENABLED )
    event_add( 0, z80_nmi_event );
}

static void
didaktik_set_intrq( struct wd_fdc *f )
{
  if( aux_register & INTRQ_ENABLED )
    event_add( 0, z80_nmi_event );
}

static int
didaktik80_init( void *context )
{
  int i;
  fdd_t *d;

  didaktik_fdc = wd_fdc_alloc_fdc( WD2797, 0, WD_FLAG_DRQ | WD_FLAG_RDY );

  for( i = 0; i < DIDAKTIK80_NUM_DRIVES; i++ ) {
    d = &didaktik_drives[ i ];
    fdd_init( d, FDD_SHUGART, NULL, 0 );       /* drive geometry 'autodetect' */
    d->disk.flag = DISK_FLAG_NONE;
  }

  didaktik_fdc->current_drive = &didaktik_drives[ 0 ];
  fdd_select( &didaktik_drives[ 0 ], 1 );
  didaktik_fdc->extra_signal = 1;
  didaktik_fdc->dden = 1;
  didaktik_fdc->set_intrq = didaktik_set_intrq;
  didaktik_fdc->reset_intrq = NULL;
  didaktik_fdc->set_datarq = didaktik_set_datarq;
  didaktik_fdc->reset_datarq = NULL;

  module_register( &didaktik_module_info );

  didaktik_rom_memory_source = memory_source_register( "Didaktik 80 ROM" );
  didaktik_ram_memory_source = memory_source_register( "Didaktik 80 RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_14K; i++ )
    didaktik_memory_map_romcs_rom[i].source = didaktik_rom_memory_source;

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    didaktik_memory_map_romcs_ram[i].source = didaktik_ram_memory_source;

  periph_register( PERIPH_TYPE_DIDAKTIK80, &didaktik_periph );
  for( i = 0; i < DIDAKTIK80_NUM_DRIVES; i++ ) {
    didaktik_ui_drives[ i ].fdd = &didaktik_drives[ i ];
    ui_media_drive_register( &didaktik_ui_drives[ i ] );
  }

  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
didaktik_reset( int hard_reset )
{
  int i;

  didaktik80_active = 0;
  didaktik80_available = 0;

  ui_menu_activate( UI_MENU_ITEM_MACHINE_DIDAKTIK80_SNAP, 0 );
  if( !periph_is_active( PERIPH_TYPE_DIDAKTIK80 ) ) {
    return;
  }

  if( machine_load_rom_bank( didaktik_memory_map_romcs_rom, 0,
                             settings_current.rom_didaktik80,
                             settings_default.rom_didaktik80, ROM_SIZE ) ) {
    settings_current.didaktik80 = 0;
    periph_activate_type( PERIPH_TYPE_DIDAKTIK80, 0 );
    return;
  }

  ui_menu_activate( UI_MENU_ITEM_MACHINE_DIDAKTIK80_SNAP, 1 );

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    struct memory_page *page =
      &didaktik_memory_map_romcs_ram[ i ];
    page->page = ram + i * MEMORY_PAGE_SIZE;
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }

  machine_current->ram.romcs = 0;

  aux_register = 0;

  didaktik80_available = 1;

  if( hard_reset )
    memset( ram, 0, sizeof( ram ) );

  wd_fdc_master_reset( didaktik_fdc );

  for( i = 0; i < DIDAKTIK80_NUM_DRIVES; i++ ) {
    ui_media_drive_update_menus( &didaktik_ui_drives[ i ],
                                 UI_MEDIA_DRIVE_UPDATE_ALL );
  }

  didaktik_fdc->current_drive = &didaktik_drives[ 0 ];
  fdd_select( &didaktik_drives[ 0 ], 1 );
  fdd_select( &didaktik_drives[ 1 ], 0 );
  machine_current->memory_map();

}

static void
didaktik80_end( void )
{
  didaktik80_available = 0;
  libspectrum_free( didaktik_fdc );
}

void
didaktik80_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_DIDAKTIK, dependencies,
                            ARRAY_SIZE( dependencies ), didaktik80_init, NULL,
                            didaktik80_end );
}

static libspectrum_byte
didaktik_sr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sr_read( didaktik_fdc );
}

static void
didaktik_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_cr_write( didaktik_fdc, b );
}

static libspectrum_byte
didaktik_tr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_tr_read( didaktik_fdc );
}

static void
didaktik_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_tr_write( didaktik_fdc, b );
}

static libspectrum_byte
didaktik_sec_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sec_read( didaktik_fdc );
}

static void
didaktik_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_sec_write( didaktik_fdc, b );
}

static libspectrum_byte
didaktik_dr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_dr_read( didaktik_fdc );
}

static void
didaktik_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_dr_write( didaktik_fdc, b );
}

static libspectrum_byte
didaktik_8255_read( libspectrum_word port, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return 0xff;
}

static void
didaktik_8255_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  return;
}

static void
didaktik_aux_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( ( b & 0x01 ) != ( aux_register & 0x01 ) )
    fdd_select( &didaktik_drives[ 0 ], b & 0x01 ? 1 : 0 );
  if( ( b & 0x02 ) != ( aux_register & 0x02 ) )
    fdd_select( &didaktik_drives[ 1 ], b & 0x02 ? 1 : 0 );
  didaktik_fdc->current_drive = &didaktik_drives[ b & 0x02 ? 1 : 0 ];

  if( ( b & 0x04 ) != ( aux_register & 0x04 ) )
    fdd_motoron( &didaktik_drives[ 0 ], b & 0x04 ? 1 : 0 );
  if( ( b & 0x08 ) != ( aux_register & 0x08 ) )
    fdd_motoron( &didaktik_drives[ 1 ], b & 0x08 ? 1 : 0 );

  aux_register = b;
}

int
didaktik80_disk_insert( didaktik80_drive_number which, const char *filename,
		   int autoload )
{
  if( which >= DIDAKTIK80_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "didaktik80_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &didaktik_ui_drives[ which ], filename, autoload );
}

fdd_t *
didaktik80_get_fdd( didaktik80_drive_number which )
{
  return &( didaktik_drives[ which ] );
}

int
didaktik80_unittest( void )
{
  int r = 0;

  didaktik80_page();

  r += unittests_assert_8k_page( 0x0000, didaktik_rom_memory_source, 0 );
  r += unittests_assert_4k_page( 0x2000, didaktik_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3000, didaktik_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3800, didaktik_ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  didaktik80_unpage();

  r += unittests_paging_test_48( 2 );

  return r;
}

static int
ui_drive_is_available( void )
{
  return didaktik80_available;
}

static const fdd_params_t *
ui_drive_get_params_a( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_didaktik80a_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_b( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_didaktik80b_type() ];
}

static ui_media_drive_info_t didaktik_ui_drives[ DIDAKTIK80_NUM_DRIVES ] = {
  {
    /* .name = */ "Didaktik Disk A",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_DIDAKTIK,
    /* .drive_index = */ DIDAKTIK80_DRIVE_A,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_a,
  },
  {
    /* .name = */ "Didaktik Disk B",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_DIDAKTIK,
    /* .drive_index = */ DIDAKTIK80_DRIVE_B,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_b,
  },
};

static void
didaktik_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.didaktik80 = libspectrum_snap_didaktik80_active( snap );
}

static void
didaktik_from_snapshot( libspectrum_snap *snap )
{
  int i;

  if( !libspectrum_snap_didaktik80_active( snap ) ) return;

  if( libspectrum_snap_didaktik80_custom_rom( snap ) &&
      libspectrum_snap_didaktik80_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             didaktik_memory_map_romcs_rom, 0,
                             libspectrum_snap_didaktik80_rom( snap, 0 ),
                             ROM_SIZE, 1 ) )
    return;

  if( libspectrum_snap_didaktik80_ram( snap, 0 ) ) {
    for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
      memcpy( didaktik_memory_map_romcs_ram[ i ].page,
              libspectrum_snap_didaktik80_ram( snap, 0 ) + i * MEMORY_PAGE_SIZE,
              MEMORY_PAGE_SIZE );
  }

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_didaktik80_drive_count( snap )
   */

  didaktik_fdc->direction = libspectrum_snap_plusd_direction( snap );

  didaktik_cr_write ( 0x0081, libspectrum_snap_didaktik80_status ( snap ) );
  didaktik_tr_write ( 0x0083, libspectrum_snap_didaktik80_track  ( snap ) );
  didaktik_sec_write( 0x0085, libspectrum_snap_didaktik80_sector ( snap ) );
  didaktik_dr_write ( 0x0087, libspectrum_snap_didaktik80_data   ( snap ) );
  didaktik_aux_write( 0x0089, libspectrum_snap_didaktik80_aux    ( snap ) );

  if( libspectrum_snap_didaktik80_paged( snap ) ) {
    didaktik80_page();
  } else {
    didaktik80_unpage();
  }
}

static void
didaktik_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int drive_count = 0;
  int i;
  size_t memory_length;

  if( !periph_is_active( PERIPH_TYPE_DIDAKTIK80 ) ) return;

  libspectrum_snap_set_didaktik80_active( snap, 1 );

  memory_length = ROM_SIZE;

  libspectrum_snap_set_didaktik80_custom_rom( snap, 1 );
  libspectrum_snap_set_didaktik80_rom_length( snap, 0, memory_length );

  buffer = libspectrum_new( libspectrum_byte, memory_length );

  for( i = 0; i < MEMORY_PAGES_IN_14K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            didaktik_memory_map_romcs_rom[ i ].page, MEMORY_PAGE_SIZE );

  libspectrum_snap_set_didaktik80_rom( snap, 0, buffer );

  memory_length = RAM_SIZE;
  buffer = libspectrum_new( libspectrum_byte, memory_length );

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            didaktik_memory_map_romcs_ram[ i ].page, MEMORY_PAGE_SIZE );
  libspectrum_snap_set_didaktik80_ram( snap, 0, buffer );

  drive_count++; /* Drive 1 is not removable */
  if( option_enumerate_diskoptions_drive_didaktik80b_type() > 0 ) drive_count++;
  libspectrum_snap_set_didaktik80_drive_count( snap, drive_count );

  libspectrum_snap_set_didaktik80_paged ( snap, didaktik80_active );
  libspectrum_snap_set_didaktik80_direction( snap, didaktik_fdc->direction );
  libspectrum_snap_set_didaktik80_status( snap, didaktik_fdc->status_register );
  libspectrum_snap_set_didaktik80_track ( snap, didaktik_fdc->track_register );
  libspectrum_snap_set_didaktik80_sector( snap, didaktik_fdc->sector_register );
  libspectrum_snap_set_didaktik80_data  ( snap, didaktik_fdc->data_register );
  libspectrum_snap_set_didaktik80_aux   ( snap, aux_register );
}
