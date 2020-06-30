/* opus.c: Routines for handling the Opus Discovery interface
   Copyright (c) 1999-2016 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin, Darren Salt, Michael D Wynne, Gergely Szasz

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
#include "opus.h"
#include "peripherals/printer.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "unittests/unittests.h"
#include "utils.h"
#include "wd_fdc.h"
#include "options.h"	/* needed for get combo options */
#include "z80/z80.h"

/* 8KB ROM */
#define OPUS_ROM_SIZE 0x2000
/* 2KB ROM */
#define OPUS_RAM_SIZE 0x0800

static int opus_rom_memory_source, opus_ram_memory_source;

/* Two memory chunks accessible by the Z80 when /ROMCS is low */
static memory_page opus_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page opus_memory_map_romcs_ram[ MEMORY_PAGES_IN_2K ];

int opus_available = 0;
int opus_active = 0;

static wd_fdc *opus_fdc;
static fdd_t opus_drives[ OPUS_NUM_DRIVES ];
static ui_media_drive_info_t opus_ui_drives[ OPUS_NUM_DRIVES ];

static libspectrum_byte opus_ram[ OPUS_RAM_SIZE ];

/* 6821 PIA internal registers */
static libspectrum_byte data_reg_a, data_dir_a, control_a;
static libspectrum_byte data_reg_b, data_dir_b, control_b;

static void opus_reset( int hard_reset );
static void opus_memory_map( void );
static void opus_enabled_snapshot( libspectrum_snap *snap );
static void opus_from_snapshot( libspectrum_snap *snap );
static void opus_to_snapshot( libspectrum_snap *snap );

static module_info_t opus_module_info = {

  /* .reset = */ opus_reset,
  /* .romcs = */ opus_memory_map,
  /* .snapshot_enabled = */ opus_enabled_snapshot,
  /* .snapshot_from = */ opus_from_snapshot,
  /* .snapshot_to = */ opus_to_snapshot,

};

static const periph_t opus_periph = {
  /* .option = */ &settings_current.opus,
  /* .ports = */ NULL,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

/* Debugger events */
static const char * const event_type_string = "opus";
static int page_event, unpage_event;

void
opus_page( void )
{
  opus_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
opus_unpage( void )
{
  opus_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
opus_memory_map( void )
{
  if( !opus_active ) return;

  memory_map_romcs_8k( 0x0000, opus_memory_map_romcs_rom );
  memory_map_romcs_2k( 0x2000, opus_memory_map_romcs_ram );
  /* FIXME: should we add mirroring at 0x2800, 0x3000 and/or 0x3800? */
}

static void
opus_set_datarq( struct wd_fdc *f )
{
  event_add( 0, z80_nmi_event );
}

static int
opus_init( void *context )
{
  int i;
  fdd_t *d;

  opus_fdc = wd_fdc_alloc_fdc( WD1770, 0, WD_FLAG_DRQ );

  for( i = 0; i < OPUS_NUM_DRIVES; i++ ) {
    d = &opus_drives[ i ];
    fdd_init( d, FDD_SHUGART, NULL, 0 );	/* drive geometry 'autodetect' */
    d->disk.flag = DISK_FLAG_NONE;
  }

  opus_fdc->current_drive = &opus_drives[ 0 ];
  fdd_select( &opus_drives[ 0 ], 1 );
  opus_fdc->dden = 1;
  opus_fdc->set_intrq = NULL;
  opus_fdc->reset_intrq = NULL;
  opus_fdc->set_datarq = opus_set_datarq;
  opus_fdc->reset_datarq = NULL;

  module_register( &opus_module_info );

  opus_rom_memory_source = memory_source_register( "Opus ROM" );
  opus_ram_memory_source = memory_source_register( "Opus RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    opus_memory_map_romcs_rom[i].source = opus_rom_memory_source;
  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    opus_memory_map_romcs_ram[i].source = opus_ram_memory_source;

  periph_register( PERIPH_TYPE_OPUS, &opus_periph );
  for( i = 0; i < OPUS_NUM_DRIVES; i++ ) {
    opus_ui_drives[ i ].fdd = &opus_drives[ i ];
    ui_media_drive_register( &opus_ui_drives[ i ] );
  }

  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
opus_end( void )
{
  opus_available = 0;
  libspectrum_free( opus_fdc );
}

void
opus_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_OPUS, dependencies,
                            ARRAY_SIZE( dependencies ), opus_init, NULL,
                            opus_end );
}

static void
opus_reset( int hard_reset )
{
  int i;

  opus_active = 0;
  opus_available = 0;

  if( !periph_is_active( PERIPH_TYPE_OPUS ) ) {
    return;
  }

  if( machine_load_rom_bank( opus_memory_map_romcs_rom, 0,
                             settings_current.rom_opus,
                             settings_default.rom_opus, OPUS_ROM_SIZE ) ) {
    settings_current.opus = 0;
    periph_activate_type( PERIPH_TYPE_OPUS, 0 );
    return;
  }

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    struct memory_page *page =
      &opus_memory_map_romcs_ram[ i ];
    page->page = opus_ram + i * MEMORY_PAGE_SIZE;
    page->offset = i * MEMORY_PAGE_SIZE;
  }

  machine_current->ram.romcs = 0;

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    opus_memory_map_romcs_ram[ i ].writable = 1;

  data_reg_a = 0;
  data_dir_a = 0;
  control_a  = 0;
  data_reg_b = 0;
  data_dir_b = 0;
  control_b  = 0;

  opus_available = 1;

  if( hard_reset )
    memset( opus_ram, 0, sizeof( opus_ram ) );

  wd_fdc_master_reset( opus_fdc );

  for( i = 0; i < OPUS_NUM_DRIVES; i++ ) {
    ui_media_drive_update_menus( &opus_ui_drives[ i ],
                                 UI_MEDIA_DRIVE_UPDATE_ALL );
  }

  opus_fdc->current_drive = &opus_drives[ 0 ];
  fdd_select( &opus_drives[ 0 ], 1 );
  machine_current->memory_map();
}

/*
 * opus_6821_access( reg, data, dir )
 *
 * reg - register to access:
 *
 * data - if dir = 1 the value being written else ignored
 *
 * dir - direction of data. 0 = read, 1 = write
 *
 * returns: value of register if dir = 0 else 0
 *
 * Mostly borrowed from EightyOne - A Windows ZX80/81/clone emulator
 */

static libspectrum_byte
opus_6821_access( libspectrum_byte reg, libspectrum_byte data,
                  libspectrum_byte dir )
{
  int drive, side;
  int i;

  switch( reg & 0x03 ) {
  case 0:
    if( dir ) {
      if( control_a & 0x04 ) {
        data_reg_a = data;

        drive = ( data & 0x02 ) == 2 ? 1 : 0;
        side = ( data & 0x10 )>>4 ? 1 : 0;

        for( i = 0; i < OPUS_NUM_DRIVES; i++ ) {
          fdd_set_head( &opus_drives[ i ], side );
        }

        fdd_select( &opus_drives[ (!drive) ], 0 );
        fdd_select( &opus_drives[ drive ], 1 );

        if( opus_fdc->current_drive != &opus_drives[ drive ] ) {
          if( opus_fdc->current_drive->motoron ) {        /* swap motoron */
            fdd_motoron( &opus_drives[ (!drive) ], 0 );
            fdd_motoron( &opus_drives[ drive ], 1 );
          }
          opus_fdc->current_drive = &opus_drives[ drive ];
        }
      } else {
        data_dir_a = data;
      }
    } else {
      if( control_a & 0x04 ) {
        /* printer never busy (bit 6) */
        data_reg_a &= ~0x40;
        return data_reg_a;
      } else {
        return data_dir_a;
      }
    }
    break;
  case 1:
    if( dir ) {
      control_a = data;
    } else {
      /* Always return bit 6 set to ACK parallel port actions */
      return control_a | 0x40;
    }
    break;
  case 2:
    if( dir ) {
      if( control_b & 0x04 ) {
        data_reg_b = data;
        printer_parallel_write( 0x00, data );
        /* Don't worry about emulating the strobes from the ROM, they are
           all bound up with checking current printer busy status which we
           don't emulate, so just send the char now */
        printer_parallel_strobe_write( 0 );
        printer_parallel_strobe_write( 1 );
        printer_parallel_strobe_write( 0 );
      } else {
        data_dir_b = data;
      }
    } else {
      if( control_b & 0x04 ) {
        return data_reg_b;
      } else {
        return data_dir_b;
      }
    }
    break;
  case 3:
    if( dir ) {
      control_b = data;
    } else {
      return control_b;
    }
    break;
  }

  return 0;
}

int
opus_disk_insert( opus_drive_number which, const char *filename,
		   int autoload )
{
  if( which >= OPUS_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "opus_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &opus_ui_drives[ which ], filename, autoload );
}

fdd_t *
opus_get_fdd( opus_drive_number which )
{
  return &( opus_drives[ which ] );
}

libspectrum_byte
opus_read( libspectrum_word address )
{
  libspectrum_byte data = 0xff;

  if( address >= 0x3800 ) data = 0xff; /* Undefined on Opus */
  else if( address >= 0x3000 )         /* 6821 PIA */
    data = opus_6821_access( address, 0, 0 );
  else if( address >= 0x2800 ) {       /* WD1770 FDC */
    switch( address & 0x03 ) {
    case 0:
      data = wd_fdc_sr_read( opus_fdc );
      break;
    case 1:
      data = wd_fdc_tr_read( opus_fdc );
      break;
    case 2:
      data = wd_fdc_sec_read( opus_fdc );
      break;
    case 3:
      data = wd_fdc_dr_read( opus_fdc );
      break;
    }
  }

  return data;
}

void
opus_write( libspectrum_word address, libspectrum_byte b )
{
  if( address < 0x2000 ) return;
  if( address >= 0x3800 ) return;

  if( address >= 0x3000 ) {
    opus_6821_access( address, b, 1 );
  } else if( address >= 0x2800 ) {
    switch( address & 0x03 ) {
    case 0:
      wd_fdc_cr_write( opus_fdc, b );
      break;
    case 1:
      wd_fdc_tr_write( opus_fdc, b );
      break;
    case 2:
      wd_fdc_sec_write( opus_fdc, b );
      break;
    case 3:
      wd_fdc_dr_write( opus_fdc, b );
      break;
    }
  }
}

static void
opus_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.opus = libspectrum_snap_opus_active( snap );
}

static void
opus_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_opus_active( snap ) ) return;

  if( libspectrum_snap_opus_custom_rom( snap ) &&
      libspectrum_snap_opus_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             opus_memory_map_romcs_rom, 0,
                             libspectrum_snap_opus_rom( snap, 0 ),
                             OPUS_ROM_SIZE, 1 ) )
    return;

  if( libspectrum_snap_opus_ram( snap, 0 ) ) {
    memcpy( opus_ram,
            libspectrum_snap_opus_ram( snap, 0 ), OPUS_RAM_SIZE );
  }

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_opus_drive_count( snap )
   */

  opus_fdc->direction = libspectrum_snap_opus_direction( snap );

  wd_fdc_cr_write ( opus_fdc, libspectrum_snap_opus_status ( snap ) );
  wd_fdc_tr_write ( opus_fdc, libspectrum_snap_opus_track  ( snap ) );
  wd_fdc_sec_write( opus_fdc, libspectrum_snap_opus_sector ( snap ) );
  wd_fdc_dr_write ( opus_fdc, libspectrum_snap_opus_data   ( snap ) );
  data_reg_a = libspectrum_snap_opus_data_reg_a( snap );
  data_dir_a = libspectrum_snap_opus_data_dir_a( snap );
  control_a  = libspectrum_snap_opus_control_a ( snap );
  data_reg_b = libspectrum_snap_opus_data_reg_b( snap );
  data_dir_b = libspectrum_snap_opus_data_dir_b( snap );
  control_b  = libspectrum_snap_opus_control_b ( snap );

  if( libspectrum_snap_opus_paged( snap ) ) {
    opus_page();
  } else {
    opus_unpage();
  }
}

static void
opus_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int drive_count = 0;
  int i;

  if( !periph_is_active( PERIPH_TYPE_OPUS ) ) return;

  libspectrum_snap_set_opus_active( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, OPUS_ROM_SIZE );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            opus_memory_map_romcs_rom[ i ].page, MEMORY_PAGE_SIZE );

  libspectrum_snap_set_opus_rom( snap, 0, buffer );

  if( opus_memory_map_romcs_rom[0].save_to_snapshot )
    libspectrum_snap_set_opus_custom_rom( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, OPUS_RAM_SIZE );
  memcpy( buffer, opus_ram, OPUS_RAM_SIZE );
  libspectrum_snap_set_opus_ram( snap, 0, buffer );

  drive_count++; /* Drive 1 is not removable */
  if( option_enumerate_diskoptions_drive_opus2_type() > 0 ) drive_count++;
  libspectrum_snap_set_opus_drive_count( snap, drive_count );

  libspectrum_snap_set_opus_paged     ( snap, opus_active );
  libspectrum_snap_set_opus_direction ( snap, opus_fdc->direction );
  libspectrum_snap_set_opus_status    ( snap, opus_fdc->status_register );
  libspectrum_snap_set_opus_track     ( snap, opus_fdc->track_register );
  libspectrum_snap_set_opus_sector    ( snap, opus_fdc->sector_register );
  libspectrum_snap_set_opus_data      ( snap, opus_fdc->data_register );
  libspectrum_snap_set_opus_data_reg_a( snap, data_reg_a );
  libspectrum_snap_set_opus_data_dir_a( snap, data_dir_a );
  libspectrum_snap_set_opus_control_a ( snap, control_a );
  libspectrum_snap_set_opus_data_reg_b( snap, data_reg_b );
  libspectrum_snap_set_opus_data_dir_b( snap, data_dir_b );
  libspectrum_snap_set_opus_control_b ( snap, control_b );
}

int
opus_unittest( void )
{
  int r = 0;

  opus_page();

  r += unittests_assert_8k_page( 0x0000, opus_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2000, opus_ram_memory_source, 0 );
  /* FIXME: should we add mirroring at 0x2800, 0x3000 and/or 0x3800? */
  r += unittests_assert_4k_page( 0x3000, memory_source_rom, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  opus_unpage();

  r += unittests_paging_test_48( 2 );

  return r;
}

static int
ui_drive_is_available( void )
{
  return opus_available;
}

static const fdd_params_t *
ui_drive_get_params_1( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_opus1_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_2( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_opus2_type() ];
}

static ui_media_drive_info_t opus_ui_drives[ OPUS_NUM_DRIVES ] = {
  {
    /* .name = */ "Opus Disk 1",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_OPUS,
    /* .drive_index = */ OPUS_DRIVE_1,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_OPUS,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_1,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_1_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_1_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_1_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_1,
  },
  {
    /* .name = */ "Opus Disk 2",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_OPUS,
    /* .drive_index = */ OPUS_DRIVE_2,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_OPUS,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_2,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_2_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_2_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_OPUS_2_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_2,
  },
};

