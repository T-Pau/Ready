/* plusd.c: Routines for handling the +D interface
   Copyright (c) 1999-2016 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin, Darren Salt, 2014 Gergely Szasz

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

   Philip: philip-fuse@shadowmagic.org.uk

   Stuart: stuart.brady@gmail.com

*/

#include <config.h>

#include <libspectrum.h>

#include <string.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "peripherals/printer.h"
#include "plusd.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "unittests/unittests.h"
#include "utils.h"
#include "wd_fdc.h"
#include "options.h"	/* needed for get combo options */

/* 8KB ROM */
#define ROM_SIZE 0x2000
/* 8KB RAM */
#define RAM_SIZE 0x2000

/* Two 8KB memory chunks accessible by the Z80 when /ROMCS is low */
static memory_page plusd_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page plusd_memory_map_romcs_ram[ MEMORY_PAGES_IN_8K ];
static int plusd_memory_source_rom;
static int plusd_memory_source_ram;

int plusd_available = 0;
int plusd_active = 0;

static wd_fdc *plusd_fdc;
static fdd_t plusd_drives[ PLUSD_NUM_DRIVES ];
static ui_media_drive_info_t plusd_ui_drives[ PLUSD_NUM_DRIVES ];

static libspectrum_byte *plusd_ram;
static int memory_allocated = 0;

static void plusd_reset( int hard_reset );
static void plusd_memory_map( void );
static void plusd_enabled_snapshot( libspectrum_snap *snap );
static void plusd_from_snapshot( libspectrum_snap *snap );
static void plusd_to_snapshot( libspectrum_snap *snap );
static void plusd_activate( void );

/* WD1770 registers */
static libspectrum_byte plusd_sr_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_cr_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte plusd_tr_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_tr_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte plusd_sec_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_sec_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte plusd_dr_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_dr_write( libspectrum_word port, libspectrum_byte b );

static void plusd_cn_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte plusd_patch_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_patch_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte plusd_printer_read( libspectrum_word port, libspectrum_byte *attached );
static void plusd_printer_write( libspectrum_word port, libspectrum_byte b );

static module_info_t plusd_module_info = {

  /* .reset = */ plusd_reset,
  /* .romcs = */ plusd_memory_map,
  /* .snapshot_enabled = */ plusd_enabled_snapshot,
  /* .snapshot_from = */ plusd_from_snapshot,
  /* .snapshot_to = */ plusd_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "plusd";
static int page_event, unpage_event;

static libspectrum_byte plusd_control_register;

void
plusd_page( void )
{
  plusd_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
plusd_unpage( void )
{
  plusd_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
plusd_memory_map( void )
{
  if( !plusd_active ) return;

  memory_map_romcs_8k( 0x0000, plusd_memory_map_romcs_rom );
  memory_map_romcs_8k( 0x2000, plusd_memory_map_romcs_ram );
}

static const periph_port_t plusd_ports[] = {
  /* ---- ---- 1110 0011 */
  { 0x00ff, 0x00e3, plusd_sr_read, plusd_cr_write },
  /* ---- ---- 1110 1011 */
  { 0x00ff, 0x00eb, plusd_tr_read, plusd_tr_write },
  /* ---- ---- 1111 0011 */
  { 0x00ff, 0x00f3, plusd_sec_read, plusd_sec_write },
  /* ---- ---- 1111 1011 */
  { 0x00ff, 0x00fb, plusd_dr_read, plusd_dr_write },

  /* ---- ---- 1110 1111 */
  { 0x00ff, 0x00ef, NULL, plusd_cn_write },
  /* ---- ---- 1110 0111 */
  { 0x00ff, 0x00e7, plusd_patch_read, plusd_patch_write },
  /* 0000 0000 1111 0111 */
  { 0x00ff, 0x00f7, plusd_printer_read, plusd_printer_write },

  { 0, 0, NULL, NULL }
};

static const periph_t plusd_periph = {
  /* .option = */ &settings_current.plusd,
  /* .ports = */ plusd_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ plusd_activate,
};

static int
plusd_init( void *context )
{
  int i;
  fdd_t *d;

  plusd_fdc = wd_fdc_alloc_fdc( WD1770, 0, WD_FLAG_NONE );

  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    d = &plusd_drives[ i ];
    fdd_init( d, FDD_SHUGART, NULL, 0 );
    d->disk.flag = DISK_FLAG_NONE;
  }

  plusd_fdc->current_drive = &plusd_drives[ 0 ];
  fdd_select( &plusd_drives[ 0 ], 1 );
  plusd_fdc->dden = 1;
  plusd_fdc->set_intrq = NULL;
  plusd_fdc->reset_intrq = NULL;
  plusd_fdc->set_datarq = NULL;
  plusd_fdc->reset_datarq = NULL;

  module_register( &plusd_module_info );

  plusd_memory_source_rom = memory_source_register( "PlusD ROM" );
  plusd_memory_source_ram = memory_source_register( "PlusD RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    plusd_memory_map_romcs_rom[ i ].source = plusd_memory_source_rom;
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    plusd_memory_map_romcs_ram[ i ].source = plusd_memory_source_ram;

  periph_register( PERIPH_TYPE_PLUSD, &plusd_periph );

  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    plusd_ui_drives[ i ].fdd = &plusd_drives[ i ];
    ui_media_drive_register( &plusd_ui_drives[ i ] );
  }

  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
plusd_end( void )
{
  plusd_available = 0;
  libspectrum_free( plusd_fdc );
}

void
plusd_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_PLUSD, dependencies,
                            ARRAY_SIZE( dependencies ), plusd_init, NULL,
                            plusd_end );
}

static void
plusd_reset( int hard_reset )
{
  int i;

  plusd_active = 0;
  plusd_available = 0;

  if( !periph_is_active( PERIPH_TYPE_PLUSD ) ) {
    return;
  }

  if( machine_load_rom_bank( plusd_memory_map_romcs_rom, 0,
			     settings_current.rom_plusd,
			     settings_default.rom_plusd, ROM_SIZE ) ) {
    settings_current.plusd = 0;
    periph_activate_type( PERIPH_TYPE_PLUSD, 0 );
    return;
  }

  machine_current->ram.romcs = 0;

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    struct memory_page *page = &plusd_memory_map_romcs_ram[ i ];
    page->page = &plusd_ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }

  plusd_available = 1;
  plusd_active = 1;

  if( hard_reset )
    memset( plusd_ram, 0, RAM_SIZE );

  wd_fdc_master_reset( plusd_fdc );

  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    ui_media_drive_update_menus( &plusd_ui_drives[ i ],
                                 UI_MEDIA_DRIVE_UPDATE_ALL );
  }

  plusd_fdc->current_drive = &plusd_drives[ 0 ];
  fdd_select( &plusd_drives[ 0 ], 1 );
  machine_current->memory_map();

}

static libspectrum_byte
plusd_sr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sr_read( plusd_fdc );
}

static void
plusd_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_cr_write( plusd_fdc, b );
}

static libspectrum_byte
plusd_tr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_tr_read( plusd_fdc );
}

static void
plusd_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_tr_write( plusd_fdc, b );
}

static libspectrum_byte
plusd_sec_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sec_read( plusd_fdc );
}

static void
plusd_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_sec_write( plusd_fdc, b );
}

static libspectrum_byte
plusd_dr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_dr_read( plusd_fdc );
}

static void
plusd_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_dr_write( plusd_fdc, b );
}

static void
plusd_cn_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int drive, side;
  int i;

  plusd_control_register = b;

  drive = ( b & 0x03 ) == 2 ? 1 : 0;
  side = ( b & 0x80 ) ? 1 : 0;

  /* TODO: set current_drive to NULL when bits 0 and 1 of b are '00' or '11' */
  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    fdd_set_head( &plusd_drives[ i ], side );
  }
  fdd_select( &plusd_drives[ (!drive) ], 0 );
  fdd_select( &plusd_drives[ drive ], 1 );

  if( plusd_fdc->current_drive != &plusd_drives[ drive ] ) {
    if( plusd_fdc->current_drive->motoron ) {            /* swap motoron */
      fdd_motoron( &plusd_drives[ (!drive) ], 0 );
      fdd_motoron( &plusd_drives[ drive ], 1 );
    }
    plusd_fdc->current_drive = &plusd_drives[ drive ];
  }

  printer_parallel_strobe_write( b & 0x40 );
}

static libspectrum_byte
plusd_patch_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached GCC_UNUSED )
{
  /* should we set bits in *attached? */

  plusd_page();
  return 0;
}

static void
plusd_patch_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b GCC_UNUSED )
{
  plusd_unpage();
}

static libspectrum_byte
plusd_printer_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */

  /* bit 7 = busy. other bits high? */

  if(!settings_current.printer)
    return(0xff); /* no printer attached */

  return(0x7f);   /* never busy */
}

static void
plusd_printer_write( libspectrum_word port, libspectrum_byte b )
{
  printer_parallel_write( port, b );
}

int
plusd_disk_insert( plusd_drive_number which, const char *filename,
		   int autoload )
{
  if( which >= PLUSD_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "plusd_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &plusd_ui_drives[ which ], filename, autoload );
}

fdd_t *
plusd_get_fdd( plusd_drive_number which )
{
  return &( plusd_drives[ which ] );
}

static void
plusd_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.plusd = libspectrum_snap_plusd_active( snap );
}

static void
plusd_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_plusd_active( snap ) ) return;

  if( libspectrum_snap_plusd_custom_rom( snap ) &&
      libspectrum_snap_plusd_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             plusd_memory_map_romcs_rom, 0,
                             libspectrum_snap_plusd_rom( snap, 0 ),
                             ROM_SIZE, 1 ) )
    return;

  if( libspectrum_snap_plusd_ram( snap, 0 ) ) {
    memcpy( plusd_ram,
            libspectrum_snap_plusd_ram( snap, 0 ), RAM_SIZE );
  }

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_plusd_drive_count( snap )
   */

  plusd_fdc->direction = libspectrum_snap_plusd_direction( snap );

  plusd_cr_write ( 0x00e3, libspectrum_snap_plusd_status ( snap ) );
  plusd_tr_write ( 0x00eb, libspectrum_snap_plusd_track  ( snap ) );
  plusd_sec_write( 0x00f3, libspectrum_snap_plusd_sector ( snap ) );
  plusd_dr_write ( 0x00fb, libspectrum_snap_plusd_data   ( snap ) );
  plusd_cn_write ( 0x00ef, libspectrum_snap_plusd_control( snap ) );

  if( libspectrum_snap_plusd_paged( snap ) ) {
    plusd_page();
  } else {
    plusd_unpage();
  }
}

static void
plusd_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int drive_count = 0;
  int i;

  if( !periph_is_active( PERIPH_TYPE_PLUSD ) ) return;

  libspectrum_snap_set_plusd_active( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, ROM_SIZE );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            plusd_memory_map_romcs_rom[ i ].page, MEMORY_PAGE_SIZE );
  libspectrum_snap_set_plusd_rom( snap, 0, buffer );

  if( plusd_memory_map_romcs_rom[ 0 ].save_to_snapshot )
    libspectrum_snap_set_plusd_custom_rom( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, RAM_SIZE );
  memcpy( buffer, plusd_ram, RAM_SIZE );
  libspectrum_snap_set_plusd_ram( snap, 0, buffer );

  drive_count++; /* Drive 1 is not removable */
  if( option_enumerate_diskoptions_drive_plusd2_type() > 0 ) drive_count++;
  libspectrum_snap_set_plusd_drive_count( snap, drive_count );

  libspectrum_snap_set_plusd_paged ( snap, plusd_active );
  libspectrum_snap_set_plusd_direction( snap, plusd_fdc->direction );
  libspectrum_snap_set_plusd_status( snap, plusd_fdc->status_register );
  libspectrum_snap_set_plusd_track ( snap, plusd_fdc->track_register );
  libspectrum_snap_set_plusd_sector( snap, plusd_fdc->sector_register );
  libspectrum_snap_set_plusd_data  ( snap, plusd_fdc->data_register );
  libspectrum_snap_set_plusd_control( snap, plusd_control_register );
}

static void
plusd_activate( void )
{
  if( !memory_allocated ) {
    plusd_ram = memory_pool_allocate_persistent( RAM_SIZE, 1 );
    memory_allocated = 1;
  }
}

int
plusd_unittest( void )
{
  int r = 0;

  plusd_page();

  r += unittests_assert_8k_page( 0x0000, plusd_memory_source_rom, 0 );
  r += unittests_assert_8k_page( 0x2000, plusd_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  plusd_unpage();

  r += unittests_paging_test_48( 2 );

  return r;
}

static int
ui_drive_is_available( void )
{
  return plusd_available;
}

static const fdd_params_t *
ui_drive_get_params_1( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_plusd1_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_2( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_plusd2_type() ];
}

static ui_media_drive_info_t plusd_ui_drives[ PLUSD_NUM_DRIVES ] = {
  {
    /* .name = */ "+D Disk 1",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_PLUSD,
    /* .drive_index = */ PLUSD_DRIVE_1,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_1,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_1,
  },
  {
    /* .name = */ "+D Disk 2",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_PLUSD,
    /* .drive_index = */ PLUSD_DRIVE_2,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_2,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_2,
  },
};
