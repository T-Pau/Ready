/* beta.c: Routines for handling the Beta disk interface
   Copyright (c) 2004-2016 Stuart Brady, Philip Kendall, Gergely Szasz

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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>            /* Needed for strncasecmp() on QNX6 */
#endif                          /* #ifdef HAVE_STRINGS_H */
#include <limits.h>
#include <sys/stat.h>

#include <libspectrum.h>

#include "beta.h"
#include "compat.h"
#include "debugger/debugger.h"
#include "event.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "unittests/unittests.h"
#include "utils.h"
#include "wd_fdc.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"
#include "options.h"	/* needed for get combo options */

/* A 16KB memory chunk accessible by the Z80 when /ROMCS is low */
memory_page beta_memory_map_romcs[MEMORY_PAGES_IN_16K];
static int beta_memory_source;

int beta_available = 0;
int beta_active = 0;
int beta_builtin = 0;

static libspectrum_byte beta_system_register; /* FDC system register */

libspectrum_word beta_pc_mask;
libspectrum_word beta_pc_value;

static wd_fdc *beta_fdc;
static fdd_t beta_drives[ BETA_NUM_DRIVES ];
static ui_media_drive_info_t beta_ui_drives[ BETA_NUM_DRIVES ];

static const periph_port_t beta_ports[] = {
  { 0x00ff, 0x001f, beta_sr_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00ff, beta_sp_read, beta_sp_write },
  { 0, 0, NULL, NULL }
};

static const periph_t beta_peripheral = {
  /* .option = */ &settings_current.beta128,  
  /* .ports = */ beta_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

/* Debugger events */
static const char * const event_type_string = "beta128";
static int page_event, unpage_event;

static void beta_reset( int hard_reset );
static void beta_memory_map( void );
static void beta_enabled_snapshot( libspectrum_snap *snap );
static void beta_from_snapshot( libspectrum_snap *snap );
static void beta_to_snapshot( libspectrum_snap *snap );

/* 16KB ROM */
#define ROM_SIZE 0x4000

static module_info_t beta_module_info = {

  /* .reset = */ beta_reset,
  /* .romcs = */ beta_memory_map,
  /* .snapshot_enabled = */ beta_enabled_snapshot,
  /* .snapshot_from = */ beta_from_snapshot,
  /* .snapshot_to = */ beta_to_snapshot,

};

void
beta_page( void )
{
  beta_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
beta_unpage( void )
{
  beta_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
beta_memory_map( void )
{
  if( !beta_active ) return;

  memory_map_romcs_full( beta_memory_map_romcs );
}

static void
beta_select_drive( int i )
{
  if( beta_fdc->current_drive != &beta_drives[ i & 0x03 ] ) {
    if( beta_fdc->current_drive != NULL )
      fdd_select( beta_fdc->current_drive, 0 );
    beta_fdc->current_drive = &beta_drives[ i & 0x03 ];
    fdd_select( beta_fdc->current_drive, 1 );
  }
}

static int
beta_init( void *context )
{
  int i;
  fdd_t *d;

  beta_fdc = wd_fdc_alloc_fdc( FD1793, 0, WD_FLAG_BETA128 );
  beta_fdc->current_drive = NULL;

  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    d = &beta_drives[ i ];
    fdd_init( d, FDD_SHUGART, NULL, 0 );	/* drive geometry 'autodetect' */
    d->disk.flag = DISK_FLAG_NONE;
  }
  beta_select_drive( 0 );

  beta_fdc->dden = 1;
  beta_fdc->set_intrq = NULL;
  beta_fdc->reset_intrq = NULL;
  beta_fdc->set_datarq = NULL;
  beta_fdc->reset_datarq = NULL;

  module_register( &beta_module_info );

  beta_memory_source = memory_source_register( "Betadisk" );
  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    beta_memory_map_romcs[i].source = beta_memory_source;

  periph_register( PERIPH_TYPE_BETA128, &beta_peripheral );

  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    beta_ui_drives[ i ].fdd = &beta_drives[ i ];
    ui_media_drive_register( &beta_ui_drives[ i ] );
  }

  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
beta_reset( int hard_reset GCC_UNUSED )
{
  int i;

  if( !(periph_is_active( PERIPH_TYPE_BETA128 ) ||
        periph_is_active( PERIPH_TYPE_BETA128_PENTAGON ) ||
        periph_is_active( PERIPH_TYPE_BETA128_PENTAGON_LATE )) ) {
    beta_active = 0;
    beta_available = 0;
    return;
  }

  beta_available = 1;

  beta_pc_mask = 0xff00;
  beta_pc_value = 0x3d00;

  wd_fdc_master_reset( beta_fdc );

  if( !beta_builtin ) {
    if( machine_load_rom_bank( beta_memory_map_romcs, 0,
			       settings_current.rom_beta128,
			       settings_default.rom_beta128, ROM_SIZE ) ) {
      beta_active = 0;
      beta_available = 0;
      periph_activate_type( PERIPH_TYPE_BETA128, 0 );
      settings_current.beta128 = 0;
      return;
    }

    beta_active = 0;

    if( !( machine_current->capabilities &
           LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {
      beta_pc_mask = 0xfe00;
      beta_pc_value = 0x3c00;

      /* For 48K type machines, the Beta 128 is supposed to be configured
         to start with the Beta ROM paged in (System switch in centre position)
         but we also allow the settion where the Beta does not auto-boot (System
         switch is in the off position 3)
         */
      if ( settings_current.beta128_48boot )
        beta_page();
    }
  }

  for( i = 0; i < BETA_NUM_DRIVES; i++ ) {
    ui_media_drive_update_menus( &beta_ui_drives[ i ],
                                 UI_MEDIA_DRIVE_UPDATE_ALL );
  }

  beta_select_drive( 0 );
  machine_current->memory_map();

}

static void
beta_end( void )
{
  beta_available = 0;
  libspectrum_free( beta_fdc );
}

void
beta_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_BETA, dependencies,
                            ARRAY_SIZE( dependencies ), beta_init, NULL,
                            beta_end );
}

libspectrum_byte
beta_sr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 0xff;
  return wd_fdc_sr_read( beta_fdc );
}

void
beta_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_cr_write( beta_fdc, b );
}

libspectrum_byte
beta_tr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 0xff;
  return wd_fdc_tr_read( beta_fdc );
}

void
beta_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_tr_write( beta_fdc, b );
}

libspectrum_byte
beta_sec_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 0xff;
  return wd_fdc_sec_read( beta_fdc );
}

void
beta_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_sec_write( beta_fdc, b );
}

libspectrum_byte
beta_dr_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  if( !beta_active ) return 0xff;

  *attached = 0xff;
  return wd_fdc_dr_read( beta_fdc );
}

void
beta_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;

  wd_fdc_dr_write( beta_fdc, b );
}

void
beta_sp_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !beta_active ) return;
  
  /* reset 0x04 and then set it to reset controller */
  beta_select_drive( b & 0x03 );
  /* 0x08 = block hlt, normally set */
  wd_fdc_set_hlt( beta_fdc, ( ( b & 0x08 ) ? 1 : 0 ) );
  fdd_set_head( beta_fdc->current_drive, ( ( b & 0x10 ) ? 0 : 1 ) );
  /* 0x20 = density, reset = FM, set = MFM */
  beta_fdc->dden = b & 0x20 ? 1 : 0;

  beta_system_register = b;
}

libspectrum_byte
beta_sp_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  libspectrum_byte b;

  if( !beta_active ) return 0xff;

  *attached = 0xff; /* TODO: check this */
  b = 0;

  if( beta_fdc->intrq )
    b |= 0x80;

  if( beta_fdc->datarq )
    b |= 0x40;

/* we should reset beta_datarq, but we first need to raise it for each byte
 * transferred in wd_fdc.c */

  return b;
}

int
beta_disk_insert( beta_drive_number which, const char *filename,
		      int autoload )
{
  if( which >= BETA_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "beta_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  return ui_media_drive_insert( &beta_ui_drives[ which ], filename, autoload );
}

static int
ui_drive_autoload( void )
{
  /* Clear AY registers (and more) from current machine */
  machine_reset(1);

  if( ( machine_current->capabilities &
        LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ||
      !settings_current.beta128_48boot ) {
    PC = 0;
    machine_current->ram.last_byte |= 0x10;   /* Select ROM 1 */
    beta_page();
  }

  return 0;
}

fdd_t *
beta_get_fdd( beta_drive_number which )
{
  return &( beta_drives[ which ] );
}

static void
beta_enabled_snapshot( libspectrum_snap *snap )
{
  if( !( machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) )
    settings_current.beta128 = libspectrum_snap_beta_active( snap );
}

static void
beta_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_beta_active( snap ) ) return;

  if( !( machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {
    settings_current.beta128_48boot = libspectrum_snap_beta_autoboot( snap );
  }

  beta_active = libspectrum_snap_beta_paged( snap );

  if( beta_active ) {
    beta_page();
  } else {
    beta_unpage();
  }

  if( libspectrum_snap_beta_custom_rom( snap ) &&
      libspectrum_snap_beta_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             beta_memory_map_romcs, 0,
                             libspectrum_snap_beta_rom( snap, 0 ),
                             ROM_SIZE, 1 ) )
    return;

  /* ignore drive count for now, there will be an issue with loading snaps where
     drives have been disabled
  libspectrum_snap_beta_drive_count( snap )
   */

  beta_fdc->direction = libspectrum_snap_beta_direction( snap );

  beta_cr_write ( 0x001f, 0 );
  beta_tr_write ( 0x003f, libspectrum_snap_beta_track ( snap ) );
  beta_sec_write( 0x005f, libspectrum_snap_beta_sector( snap ) );
  beta_dr_write ( 0x007f, libspectrum_snap_beta_data  ( snap ) );
  beta_sp_write ( 0x00ff, libspectrum_snap_beta_system( snap ) );
}

void
beta_to_snapshot( libspectrum_snap *snap )
{
  wd_fdc *f = beta_fdc;
  libspectrum_byte *buffer;
  int drive_count = 0;
  int i;

  if( !periph_is_active( PERIPH_TYPE_BETA128 ) ) return;

  libspectrum_snap_set_beta_active( snap, 1 );

  buffer = libspectrum_new( libspectrum_byte, ROM_SIZE );

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            beta_memory_map_romcs[ i ].page, MEMORY_PAGE_SIZE );

  libspectrum_snap_set_beta_rom( snap, 0, buffer );

  if( beta_memory_map_romcs[0].save_to_snapshot )
    libspectrum_snap_set_beta_custom_rom( snap, 1 );

  drive_count++; /* Drive A is not removable */
  if( option_enumerate_diskoptions_drive_beta128b_type() > 0 ) drive_count++;
  if( option_enumerate_diskoptions_drive_beta128c_type() > 0 ) drive_count++;
  if( option_enumerate_diskoptions_drive_beta128d_type() > 0 ) drive_count++;
  libspectrum_snap_set_beta_drive_count( snap, drive_count );

  libspectrum_snap_set_beta_paged ( snap, beta_active );
  if( !( machine_current->capabilities &
         LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) ) {
    libspectrum_snap_set_beta_autoboot( snap, settings_current.beta128_48boot );
  }
  libspectrum_snap_set_beta_direction( snap, beta_fdc->direction );
  libspectrum_snap_set_beta_status( snap, f->status_register );
  libspectrum_snap_set_beta_track ( snap, f->track_register );
  libspectrum_snap_set_beta_sector( snap, f->sector_register );
  libspectrum_snap_set_beta_data  ( snap, f->data_register );
  libspectrum_snap_set_beta_system( snap, beta_system_register );
}

int
beta_unittest( void )
{
  int r = 0;

  beta_page();

  r += unittests_assert_16k_page( 0x0000, beta_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  beta_unpage();

  r += unittests_paging_test_48( 2 );

  return r;
}

static int
ui_drive_is_available( void )
{
  return beta_available;
}

static const fdd_params_t *
ui_drive_get_params_a( void )
{
  /* +1 => there is no `Disabled' */
  return &fdd_params[ option_enumerate_diskoptions_drive_beta128a_type() + 1 ];
}

static const fdd_params_t *
ui_drive_get_params_b( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_beta128b_type() ];
}

static const fdd_params_t *
ui_drive_get_params_c( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_beta128c_type() ];
}

static const fdd_params_t *
ui_drive_get_params_d( void )
{
  return &fdd_params[ option_enumerate_diskoptions_drive_beta128d_type() ];
}

static ui_media_drive_info_t beta_ui_drives[ BETA_NUM_DRIVES ] = {
  {
    /* .name = */ "Beta Disk A:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_BETA,
    /* .drive_index = */ BETA_DRIVE_A,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_BETA,
    /* .menu_item_top = */ UI_MENU_ITEM_INVALID,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_a,
    /* .insert_hook = */ NULL,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
  {
    /* .name = */ "Beta Disk B:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_BETA,
    /* .drive_index = */ BETA_DRIVE_B,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_BETA,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_BETA_B,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_b,
    /* .insert_hook = */ NULL,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
  {
    /* .name = */ "Beta Disk C:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_BETA,
    /* .drive_index = */ BETA_DRIVE_C,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_BETA,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_BETA_C,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_c,
    /* .insert_hook = */ NULL,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
  {
    /* .name = */ "Beta Disk D:",
    /* .controller_index = */ UI_MEDIA_CONTROLLER_BETA,
    /* .drive_index = */ BETA_DRIVE_D,
    /* .menu_item_parent = */ UI_MENU_ITEM_MEDIA_DISK_BETA,
    /* .menu_item_top = */ UI_MENU_ITEM_MEDIA_DISK_BETA_D,
    /* .menu_item_eject = */ UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT,
    /* .menu_item_flip = */ UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
    /* .menu_item_wp = */ UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
    /* .is_available = */ &ui_drive_is_available,
    /* .get_params = */ &ui_drive_get_params_d,
    /* .insert_hook = */ NULL,
    /* .autoload_hook = */ &ui_drive_autoload,
  },
};
