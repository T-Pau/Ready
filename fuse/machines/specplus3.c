/* specplus3.c: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2015 Philip Kendall, Darren Salt, Gergely Szasz
   Copyright (c) 2016 Gergely Szasz

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

#include <stdio.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "compat.h"
#include "fuse.h"
#include "machine.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/fdd.h"
#include "peripherals/disk/upd_fdc.h"
#include "peripherals/printer.h"
#include "phantom_typist.h"
#include "settings.h"
#include "snapshot.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus3.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "utils.h"
#include "options.h"	/* needed for get combo options */

static int normal_memory_map( int rom, int page );
static void special_memory_map( int which );
static void select_special_map( int page1, int page2, int page3, int page4 );

static int specplus3_reset( void );

upd_fdc *specplus3_fdc;
static fdd_t specplus3_drives[ SPECPLUS3_NUM_DRIVES ];

static int ui_drive_is_available( void );
static const fdd_params_t *ui_drive_get_params_a( void );
static const fdd_params_t *ui_drive_get_params_b( void );
static int ui_drive_inserted( const ui_media_drive_info_t *drive, int new );
static int ui_drive_autoload( void );

static ui_media_drive_info_t ui_drives[ SPECPLUS3_NUM_DRIVES ] = {
  {
    /* .name = */ "+3 Disk A:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_PLUS3,
    /* .drive_index = */ SPECPLUS3_DRIVE_A,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3,
    /* .menu_item_top = */ UI_MENU_ITEM_INVALID,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_a,
    /* .insert_hook = */ &ui_drive_inserted,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
  {
    /* .name = */ "+3 Disk B:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_PLUS3,
    /* .drive_index = */ SPECPLUS3_DRIVE_B,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_B,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_b,
    /* .insert_hook = */ &ui_drive_inserted,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
};

int
specplus3_port_from_ula( libspectrum_word port GCC_UNUSED )
{
  /* No contended ports */
  return 0;
}

int specplus3_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PLUS3;
  machine->id = "plus3";

  machine->reset = specplus3_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = specplus3_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_76543210;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages	     = 8;

  machine->unattached_port = spectrum_unattached_port_none;

  specplus3_765_init();
  specplus3_menu_items();

  machine->shutdown = specplus3_shutdown;

  machine->memory_map = specplus3_memory_map;

  return 0;
}

void
specplus3_765_update_fdd( void )
{
  specplus3_fdc->speedlock = settings_current.plus3_detect_speedlock ? 0 : -1;
}

void
specplus3_765_init( void )
{
  int i;

  specplus3_fdc = upd_fdc_alloc_fdc( UPD765A, UPD_CLOCK_4MHZ );
  /*!!!! the plus3 only use the US0 pin to select drives,
   so drive 2 := drive 0 and drive 3 := drive 1 !!!!*/
  specplus3_fdc->drive[0] = &specplus3_drives[ 0 ];
  specplus3_fdc->drive[1] = &specplus3_drives[ 1 ];
  specplus3_fdc->drive[2] = &specplus3_drives[ 0 ];
  specplus3_fdc->drive[3] = &specplus3_drives[ 1 ];

  for( i = 0; i < SPECPLUS3_NUM_DRIVES; i++ ) {
    specplus3_drives[ i ].disk.flag = DISK_FLAG_PLUS3_CPC;
  }
					/* builtin drive 1 head 42 track */
  fdd_init( &specplus3_drives[ 0 ], FDD_SHUGART, &fdd_params[ 1 ], 0 );
  fdd_init( &specplus3_drives[ 1 ], FDD_SHUGART, NULL, 0 );	/* drive geometry 'autodetect' */
  specplus3_fdc->set_intrq = NULL;
  specplus3_fdc->reset_intrq = NULL;
  specplus3_fdc->set_datarq = NULL;
  specplus3_fdc->reset_datarq = NULL;

  specplus3_765_update_fdd();

  for( i = 0; i < SPECPLUS3_NUM_DRIVES; i++ ) {
    ui_drives[ i ].fdd = &specplus3_drives[ i ];
    ui_media_drive_register( &ui_drives[ i ] );
  }
}

void
specplus3_765_reset( void )
{
  const fdd_params_t *dt;

  upd_fdc_master_reset( specplus3_fdc );
  dt = &fdd_params[ option_enumerate_diskoptions_drive_plus3a_type() + 1 ];	/* +1 => there is no `Disabled' */
  fdd_init( &specplus3_drives[ 0 ], FDD_SHUGART, dt, 1 );

  dt = &fdd_params[ option_enumerate_diskoptions_drive_plus3b_type() ];
  fdd_init( &specplus3_drives[ 1 ], dt->enabled ? FDD_SHUGART : FDD_TYPE_NONE,
            dt, 1 );
}

static int
specplus3_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_plus3_0,
                            settings_default.rom_plus3_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_plus3_1,
                            settings_default.rom_plus3_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_plus3_2,
                            settings_default.rom_plus3_2, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 3, settings_current.rom_plus3_3,
                            settings_default.rom_plus3_3, 0x4000 );
  if( error ) return error;

  error = specplus3_plus2a_common_reset();
  if( error ) return error;

  periph_clear();
  machines_periph_plus3();

  periph_set_present( PERIPH_TYPE_UPD765, PERIPH_PRESENT_ALWAYS );

  periph_update();

  specplus3_765_reset();
  specplus3_menu_items();

  spec48_common_display_setup();

  return 0;
}

int
specplus3_plus2a_common_reset( void )
{
  int error;
  size_t i;

  machine_current->ram.current_page=0; machine_current->ram.current_rom=0;
  machine_current->ram.locked=0;
  machine_current->ram.special=0;
  machine_current->ram.last_byte=0;
  machine_current->ram.last_byte2=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  /* All memory comes from the home bank */
  for( i = 0; i < MEMORY_PAGES_IN_64K; i++ )
    memory_map_read[i].source = memory_map_write[i].source = memory_source_ram;

  /* RAM pages 4, 5, 6 and 7 contended */
  for( i = 0; i < 8; i++ )
    memory_ram_set_16k_contention( i, i >= 4 );

  error = normal_memory_map( 0, 0 ); if( error ) return error;

  return 0;
}

static int
normal_memory_map( int rom, int page )
{
  /* ROM as specified */
  memory_map_16k( 0x0000, memory_map_rom, rom );
  /* RAM 5 */
  memory_map_16k( 0x4000, memory_map_ram, 5 );
  /* RAM 2 */
  memory_map_16k( 0x8000, memory_map_ram, 2 );
  /* RAM 0 */
  memory_map_16k( 0xc000, memory_map_ram, page );

  return 0;
}

static void
special_memory_map( int which )
{
  switch( which ) {
  case 0: select_special_map( 0, 1, 2, 3 ); break;
  case 1: select_special_map( 4, 5, 6, 7 ); break;
  case 2: select_special_map( 4, 5, 6, 3 ); break;
  case 3: select_special_map( 4, 7, 6, 3 ); break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown +3 special configuration %d", which );
    fuse_abort();
  }
}

static void
select_special_map( int page1, int page2, int page3, int page4 )
{
  memory_map_16k( 0x0000, memory_map_ram, page1 );
  memory_map_16k( 0x4000, memory_map_ram, page2 );
  memory_map_16k( 0x8000, memory_map_ram, page3 );
  memory_map_16k( 0xc000, memory_map_ram, page4 );
}

void
specplus3_memoryport2_write_internal( libspectrum_word port GCC_UNUSED,
                                      libspectrum_byte b )
{
  /* Let the parallel printer code know about the strobe bit */
  printer_parallel_strobe_write( b & 0x10 );

  /* If this was called by a machine which has a +3-style disk, set
     the state of both floppy drive motors */
  if( machine_current->capabilities &&
      LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) {

    fdd_motoron( &specplus3_drives[0], b & 0x08 );
    fdd_motoron( &specplus3_drives[1], b & 0x08 );
  }

  /* Store the last byte written in case we need it */
  machine_current->ram.last_byte2 = b;

  machine_current->memory_map();
}

void
specplus3_memoryport2_write( libspectrum_word port, libspectrum_byte b )
{
  /* Do nothing else if we've locked the RAM configuration */
  if( machine_current->ram.locked ) return;

  specplus3_memoryport2_write_internal( port, b );
}

int
specplus3_memory_map( void )
{
  int page, rom, screen;

  page = machine_current->ram.last_byte & 0x07;
  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  rom =
    ( ( machine_current->ram.last_byte  & 0x10 ) >> 4 ) |
    ( ( machine_current->ram.last_byte2 & 0x04 ) >> 1 );

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  /* Check whether we want a special RAM configuration */
  if( machine_current->ram.last_byte2 & 0x01 ) {

    /* If so, select it */
    machine_current->ram.special = 1;
    special_memory_map( ( machine_current->ram.last_byte2 & 0x06 ) >> 1 );

  } else {

    /* If not, we're selecting the high bit of the current ROM */
    machine_current->ram.special = 0;
    normal_memory_map( rom, page );

  }

  machine_current->ram.current_page = page;
  machine_current->ram.current_rom  = rom;

  memory_romcs_map();

  return 0;
}

void
specplus3_menu_items( void )
{
  ui_media_drive_update_menus( &ui_drives[ SPECPLUS3_DRIVE_A ],
			       UI_MEDIA_DRIVE_UPDATE_ALL );
  ui_media_drive_update_menus( &ui_drives[ SPECPLUS3_DRIVE_B ],
			       UI_MEDIA_DRIVE_UPDATE_ALL );
}

libspectrum_byte
specplus3_fdc_status( libspectrum_word port GCC_UNUSED,
                      libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return upd_fdc_read_status( specplus3_fdc );
}

libspectrum_byte
specplus3_fdc_read( libspectrum_word port GCC_UNUSED,
                    libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return upd_fdc_read_data( specplus3_fdc );
}

void
specplus3_fdc_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  upd_fdc_write_data( specplus3_fdc, data );
}

/* FDC UI related functions */

int
specplus3_disk_insert( specplus3_drive_number which, const char *filename,
		   int autoload )
{
  if( which >= SPECPLUS3_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &ui_drives[ which ], filename, autoload );
}

fdd_t *
specplus3_get_fdd( specplus3_drive_number which )
{
  return &( specplus3_drives[ which ] );
}

static int
ui_drive_is_available( void )
{
  return machine_current->capabilities &
          LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK;
}

static const fdd_params_t *
ui_drive_get_params_a( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_plus3a_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_b( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_plus3b_type() ];
}

static int
ui_drive_inserted( const ui_media_drive_info_t *drive, int new )
{
  /* Did real +3 hardware also have problems formatting disks unformatted
     disks? */
  if( new )
    disk_preformat( &drive->fdd->disk );	/* pre-format disk for +3 ??? */
  return 0;
}

int
specplus3_shutdown( void )
{
  return 0;
}

static int
ui_drive_autoload( void )
{
  machine_reset( 0 );
  phantom_typist_activate_disk();
  return 0;
}
