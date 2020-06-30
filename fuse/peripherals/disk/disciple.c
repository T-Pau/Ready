/* disciple.c: Routines for handling the DISCiPLE interface
   Copyright (c) 1999-2016 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin, Darren Salt, Gergely Szasz

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
#include "disciple.h"
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

/* Two 8 KiB memory chunks accessible by the Z80 when /ROMCS is low */
/* One 8 KiB chunk of ROM, one 8 KiB chunk of RAM */
/* TODO: add support for 16 KiB ROM images. */
/* Real hardware supports the use of a 16 KiB ROM, but all of the 16 KiB
 * 'ROM dumps' for the DISCiPLE actually contain a dump of GDOS (RAM).
 * Uni-DOS also uses an 8 KiB ROM. */
static memory_page disciple_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page disciple_memory_map_romcs_ram[ MEMORY_PAGES_IN_8K ];

static int disciple_memory_source_rom;
static int disciple_memory_source_ram;

int disciple_memswap = 0;        /* Are the ROM and RAM pages swapped? */
/* TODO: add support for 16 KiB ROM images. */
/* int disciple_rombank = 0; */
int disciple_inhibited;

int disciple_available = 0;
int disciple_active = 0;

static wd_fdc *disciple_fdc;
static fdd_t disciple_drives[ DISCIPLE_NUM_DRIVES ];
static ui_media_drive_info_t disciple_ui_drives[ DISCIPLE_NUM_DRIVES ];

static libspectrum_byte *disciple_ram;
static int memory_allocated = 0;

static void disciple_reset( int hard_reset );
static void disciple_memory_map( void );
static void disciple_activate( void );
static void disciple_enabled_snapshot( libspectrum_snap *snap );
static void disciple_from_snapshot( libspectrum_snap *snap );
static void disciple_to_snapshot( libspectrum_snap *snap );

/* WD1770 registers */
static libspectrum_byte disciple_sr_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_cr_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte disciple_tr_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_tr_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte disciple_sec_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_sec_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte disciple_dr_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_dr_write( libspectrum_word port, libspectrum_byte b );

static libspectrum_byte disciple_joy_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_cn_write( libspectrum_word port, libspectrum_byte b );
static void disciple_net_write( libspectrum_word port, libspectrum_byte b);
static libspectrum_byte disciple_boot_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_boot_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte disciple_patch_read( libspectrum_word port, libspectrum_byte *attached );
static void disciple_patch_write( libspectrum_word port, libspectrum_byte b );
static void disciple_printer_write( libspectrum_word port, libspectrum_byte b );

/* 8KB ROM */
#define ROM_SIZE 0x2000
/* 8KB RAM */
#define RAM_SIZE 0x2000

static module_info_t disciple_module_info = {

  /* .reset = */ disciple_reset,
  /* .romcs = */ disciple_memory_map,
  /* .snapshot_enabled = */ disciple_enabled_snapshot,
  /* .snapshot_from = */ disciple_from_snapshot,
  /* .snapshot_to = */ disciple_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "disciple";
static int page_event, unpage_event;

static libspectrum_byte disciple_control_register;

void
disciple_page( void )
{
  disciple_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
disciple_unpage( void )
{
  disciple_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

void
disciple_memory_map( void )
{
  struct memory_page *rom_page, *lower_page, *upper_page;

  if( !disciple_active ) return;

  /* TODO: add support for 16 KiB ROM images. */
  rom_page = disciple_memory_map_romcs_rom;

  if( !disciple_memswap ) {
    lower_page = rom_page;
    upper_page = disciple_memory_map_romcs_ram;
  } else {
    lower_page = disciple_memory_map_romcs_ram;
    upper_page = rom_page;
  }

  memory_map_romcs_8k( 0x0000, lower_page );
  memory_map_romcs_8k( 0x2000, upper_page );
}

static const periph_port_t disciple_ports[] = {
  /* ---- ---- 0001 1011 */
  { 0x00ff, 0x001b, disciple_sr_read, disciple_cr_write },
  /* ---- ---- 0101 1011  */
  { 0x00ff, 0x005b, disciple_tr_read, disciple_tr_write },
  /* ---- ---- 1001 1011 */
  { 0x00ff, 0x009b, disciple_sec_read, disciple_sec_write },
  /* ---- ---- 1101 1011 */
  { 0x00ff, 0x00db, disciple_dr_read, disciple_dr_write },

  /* ---- ---- 0001 1111 */
  { 0x00ff, 0x001f, disciple_joy_read, disciple_cn_write },
  /* ---- ---- 0011 1011 */
  { 0x00ff, 0x003b, NULL, disciple_net_write },
  /* ---- ---- 0111 1011 */
  { 0x00ff, 0x007b, disciple_boot_read, disciple_boot_write },
  /* ---- ---- 1011 1011 */
  { 0x00ff, 0x00bb, disciple_patch_read, disciple_patch_write },
  /* ---- ---- 1111 1011 */
  { 0x00ff, 0x00fb, NULL, disciple_printer_write },

  { 0, 0, NULL, NULL }
};

static const periph_t disciple_periph = {
  /* .option = */ &settings_current.disciple,
  /* .ports = */ disciple_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ disciple_activate,
};

static int
disciple_init( void *context )
{
  int i;
  fdd_t *d;

  disciple_fdc = wd_fdc_alloc_fdc( WD1770, 0, WD_FLAG_NONE );

  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    d = &disciple_drives[ i ];
    fdd_init( d, FDD_SHUGART, NULL, 0 );
    d->disk.flag = DISK_FLAG_NONE;
  }

  disciple_fdc->current_drive = &disciple_drives[ 0 ];
  fdd_select( &disciple_drives[ 0 ], 1 );
  disciple_fdc->dden = 1;
  disciple_fdc->set_intrq = NULL;
  disciple_fdc->reset_intrq = NULL;
  disciple_fdc->set_datarq = NULL;
  disciple_fdc->reset_datarq = NULL;

  module_register( &disciple_module_info );

  disciple_memory_source_rom = memory_source_register( "DISCiPLE ROM" );
  disciple_memory_source_ram = memory_source_register( "DISCiPLE RAM" );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    disciple_memory_map_romcs_rom[i].source = disciple_memory_source_rom;
    disciple_memory_map_romcs_rom[i].page_num = 0;
    disciple_memory_map_romcs_rom[i].writable = 0;
  }

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    disciple_memory_map_romcs_ram[i].source = disciple_memory_source_ram;
    disciple_memory_map_romcs_ram[i].page_num = 0;
    disciple_memory_map_romcs_ram[i].writable = 1;
  }

  periph_register( PERIPH_TYPE_DISCIPLE, &disciple_periph );

  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    disciple_ui_drives[ i ].fdd = &disciple_drives[ i ];
    ui_media_drive_register( &disciple_ui_drives[ i ] );
  }

  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
disciple_end( void )
{
  disciple_available = 0;
  libspectrum_free( disciple_fdc );
}

void
disciple_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_DISCIPLE, dependencies,
                            ARRAY_SIZE( dependencies ), disciple_init, NULL,
                            disciple_end );
}

static void
disciple_reset( int hard_reset )
{
  int i;

  disciple_active = 0;
  disciple_available = 0;

  if( !periph_is_active( PERIPH_TYPE_DISCIPLE ) ) {
    return;
  }

  /* TODO: add support for 16 KiB ROM images. */
  if( machine_load_rom_bank( disciple_memory_map_romcs_rom, 0,
			     settings_current.rom_disciple,
			     settings_default.rom_disciple, ROM_SIZE ) ) {
    settings_current.disciple = 0;
    periph_activate_type( PERIPH_TYPE_DISCIPLE, 0 );
    return;
  }

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    struct memory_page *page = &disciple_memory_map_romcs_ram[ i ];
    page->page = &disciple_ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
  }

  machine_current->ram.romcs = 1;

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    disciple_memory_map_romcs_ram[ i ].writable = 1;

  disciple_available = 1;
  disciple_active = 1;

  disciple_memswap = 0;
  /* TODO: add support for 16 KiB ROM images. */
  /* disciple_rombank = 0; */

  if( hard_reset )
    memset( disciple_ram, 0, RAM_SIZE );

  wd_fdc_master_reset( disciple_fdc );

  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    ui_media_drive_update_menus( &disciple_ui_drives[ i ],
                                 UI_MEDIA_DRIVE_UPDATE_ALL );
  }

  disciple_fdc->current_drive = &disciple_drives[ 0 ];
  fdd_select( &disciple_drives[ 0 ], 1 );
  machine_current->memory_map();

}

static void
disciple_inhibit( void )
{
  /* TODO: check how this affects the hardware */
  disciple_inhibited = 1;
}

static libspectrum_byte
disciple_sr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sr_read( disciple_fdc );
}

static void
disciple_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_cr_write( disciple_fdc, b );
}

static libspectrum_byte
disciple_tr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_tr_read( disciple_fdc );
}

static void
disciple_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_tr_write( disciple_fdc, b );
}

static libspectrum_byte
disciple_sec_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_sec_read( disciple_fdc );
}

static void
disciple_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_sec_write( disciple_fdc, b );
}

static libspectrum_byte
disciple_dr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;
  return wd_fdc_dr_read( disciple_fdc );
}

static void
disciple_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  wd_fdc_dr_write( disciple_fdc, b );
}

static libspectrum_byte
disciple_joy_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */

  /* bit 6 - printer busy */
  if( !settings_current.printer )
    return 0xbf; /* no printer attached */

  return 0xff;   /* never busy */
}

static void
disciple_cn_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int drive, side;
  int i;

  disciple_control_register = b;

  drive = ( b & 0x01 ) ? 0 : 1;
  side = ( b & 0x02 ) ? 1 : 0;

  for( i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
    fdd_set_head( &disciple_drives[ i ], side );
    fdd_select( &disciple_drives[ i ], drive == i );
  }

  if( disciple_fdc->current_drive != &disciple_drives[ drive ] ) {
    if( disciple_fdc->current_drive->motoron ) {
      for (i = 0; i < DISCIPLE_NUM_DRIVES; i++ ) {
        fdd_motoron( &disciple_drives[ i ], drive == i );
      }
    }
    disciple_fdc->current_drive = &disciple_drives[ drive ];
  }

  printer_parallel_strobe_write( b & 0x40 );

  /* We only support the use of an 8 KiB ROM. */
  /* disciple_rombank = ( b & 0x08 ) ? 1 : 0; */
  machine_current->memory_map();
  if( b & 0x10 )
    disciple_inhibit();
}

static void
disciple_net_write( libspectrum_word port GCC_UNUSED,
		    libspectrum_byte b GCC_UNUSED )
{
  /* TODO: implement network emulation */
}

static libspectrum_byte
disciple_boot_read( libspectrum_word port GCC_UNUSED,
		    libspectrum_byte *attached GCC_UNUSED )
{
  disciple_memswap = 0;
  machine_current->memory_map();
  return 0;
}

static void
disciple_boot_write( libspectrum_word port GCC_UNUSED,
		     libspectrum_byte b GCC_UNUSED )
{
  disciple_memswap = 1;
  machine_current->memory_map();
}

static libspectrum_byte
disciple_patch_read( libspectrum_word port GCC_UNUSED,
		     libspectrum_byte *attached GCC_UNUSED )
{
  disciple_page();
  return 0;
}

static void
disciple_patch_write( libspectrum_word port GCC_UNUSED,
		    libspectrum_byte b GCC_UNUSED )
{
  disciple_unpage();
}

static void
disciple_printer_write( libspectrum_word port, libspectrum_byte b )
{
  printer_parallel_write( port, b );
}

int
disciple_disk_insert( disciple_drive_number which, const char *filename,
		      int autoload )
{
  if( which >= DISCIPLE_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "disciple_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &disciple_ui_drives[ which ], filename, autoload );
}

fdd_t *
disciple_get_fdd( disciple_drive_number which )
{
  return &( disciple_drives[ which ] );
}

static void
disciple_activate( void )
{
  if( !memory_allocated ) {
    disciple_ram = memory_pool_allocate_persistent( RAM_SIZE, 1 );
    memory_allocated = 1;
  }
}

int
disciple_unittest( void )
{
  int r = 0;
  /* We only support the use of an 8 KiB ROM.  Change this to 1 if adding
   * support for 16 KiB ROMs. */
  const int upper_rombank = 0;

  disciple_page();

  r += unittests_assert_8k_page( 0x0000, disciple_memory_source_rom, 0 );
  r += unittests_assert_8k_page( 0x2000, disciple_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  disciple_cn_write( 0x001f, 0x08 );
  r += unittests_assert_8k_page( 0x0000, disciple_memory_source_rom,
				 upper_rombank );
  r += unittests_assert_8k_page( 0x2000, disciple_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  disciple_boot_write( 0x007b, 0x00 );
  r += unittests_assert_8k_page( 0x0000, disciple_memory_source_ram, 0 );
  r += unittests_assert_8k_page( 0x2000, disciple_memory_source_rom,
				 upper_rombank );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  disciple_cn_write( 0x001f, 0x00 );
  r += unittests_assert_8k_page( 0x0000, disciple_memory_source_ram, 0 );
  r += unittests_assert_8k_page( 0x2000, disciple_memory_source_rom, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  disciple_boot_read( 0x007b, NULL );
  r += unittests_assert_8k_page( 0x0000, disciple_memory_source_rom, 0 );
  r += unittests_assert_8k_page( 0x2000, disciple_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  disciple_unpage();

  r += unittests_paging_test_48( 2 );

  return r;
}

static int
ui_drive_is_available( void )
{
  return disciple_available;
}

static const fdd_params_t *
ui_drive_get_params_1( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_disciple1_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_2( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_disciple2_type() ];
}

static ui_media_drive_info_t disciple_ui_drives[ DISCIPLE_NUM_DRIVES ] = {
  {
    /* .name = */ "DISCiPLE Disk 1",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_DISCIPLE,
    /* .drive_index = */ DISCIPLE_DRIVE_1,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_1,
  },
  {
    /* .name = */ "DISCiPLE Disk 2",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_DISCIPLE,
    /* .drive_index = */ DISCIPLE_DRIVE_2,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_2,
  },
};

static void
disciple_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.disciple = libspectrum_snap_disciple_active( snap );
}

static void
disciple_from_snapshot( libspectrum_snap *snap )
{
  int i;

  if( !libspectrum_snap_disciple_active( snap ) ) return;

  if( libspectrum_snap_disciple_custom_rom( snap ) &&
      libspectrum_snap_disciple_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             disciple_memory_map_romcs_rom, 0,
                             libspectrum_snap_disciple_rom( snap, 0 ),
                             libspectrum_snap_disciple_rom_length( snap, 0 ),
                             1 ) )
    return;

  if( libspectrum_snap_disciple_ram( snap, 0 ) ) {
    for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
      memcpy( disciple_memory_map_romcs_ram[ i ].page,
              libspectrum_snap_disciple_ram( snap, 0 ) + i * MEMORY_PAGE_SIZE,
              MEMORY_PAGE_SIZE );
  }

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_disciple_drive_count( snap )
   */

  /* FIXME: As disciple_inhibited can flip on and off the hardware inhibit
     button is effectively always pressed
  libspectrum_snap_disciple_inhibit_button( snap );
   */

  disciple_fdc->direction = libspectrum_snap_plusd_direction( snap );

  disciple_cr_write ( 0x001b, libspectrum_snap_disciple_status ( snap ) );
  disciple_tr_write ( 0x005b, libspectrum_snap_disciple_track  ( snap ) );
  disciple_sec_write( 0x009b, libspectrum_snap_disciple_sector ( snap ) );
  disciple_dr_write ( 0x00db, libspectrum_snap_disciple_data   ( snap ) );
  disciple_cn_write ( 0x001f, libspectrum_snap_disciple_control( snap ) );
  /* FIXME: Set disciple_inhibited based on the value in
     libspectrum_snap_disciple_control() */

  if( libspectrum_snap_disciple_paged( snap ) ) {
    disciple_page();
  } else {
    disciple_unpage();
  }
}

static void
disciple_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int drive_count = 0;
  int i;

  if( !periph_is_active( PERIPH_TYPE_DISCIPLE ) ) return;

  libspectrum_snap_set_disciple_active( snap, 1 );

  /* Always save ROM to snapshot to ensure any loading emulator has the matching
     image */
  libspectrum_snap_set_disciple_custom_rom( snap, 1 );
  libspectrum_snap_set_disciple_rom_length( snap, 0, ROM_SIZE );

  buffer = libspectrum_new( libspectrum_byte, ROM_SIZE );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            disciple_memory_map_romcs_rom[ i ].page, MEMORY_PAGE_SIZE );

  libspectrum_snap_set_disciple_rom( snap, 0, buffer );

  buffer = libspectrum_new( libspectrum_byte, RAM_SIZE );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            disciple_memory_map_romcs_ram[ i ].page, MEMORY_PAGE_SIZE );
  libspectrum_snap_set_disciple_ram( snap, 0, buffer );

  drive_count++; /* Drive 1 is not removable */
  if( option_enumerate_diskoptions_drive_disciple2_type() > 0 ) drive_count++;
  libspectrum_snap_set_disciple_drive_count( snap, drive_count );

  libspectrum_snap_set_disciple_paged ( snap, disciple_active );
  /* FIXME: As disciple_inhibited can flip on and off the hardware inhibit
     button is effectively always pressed but should be emulated */
  libspectrum_snap_set_disciple_inhibit_button( snap, 1 );
  libspectrum_snap_set_disciple_direction( snap, disciple_fdc->direction );
  libspectrum_snap_set_disciple_status( snap, disciple_fdc->status_register );
  libspectrum_snap_set_disciple_track ( snap, disciple_fdc->track_register );
  libspectrum_snap_set_disciple_sector( snap, disciple_fdc->sector_register );
  libspectrum_snap_set_disciple_data  ( snap, disciple_fdc->data_register );
  libspectrum_snap_set_disciple_control( snap, disciple_control_register );
}
