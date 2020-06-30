/* zxmmc.c: ZXMMC interface routines
   Copyright (c) 2017 Philip Kendall, Sergio Baldoví

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

#include "ide.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "zxmmc.h"

/* Private function prototypes */

static void zxmmc_card_select( libspectrum_word port, libspectrum_byte data );
static libspectrum_byte zxmmc_mmc_read( libspectrum_word port,
                                        libspectrum_byte *attached );
static void zxmmc_mmc_write( libspectrum_word port, libspectrum_byte data );

/* Data */

static const periph_port_t zxmmc_ports[] = {
  { 0x00ff, 0x001f, NULL, zxmmc_card_select },
  { 0x00ff, 0x003f, zxmmc_mmc_read, zxmmc_mmc_write },
  { 0, 0, NULL, NULL }
};

static const periph_t zxmmc_periph = {
  /* .option = */ &settings_current.zxmmc_enabled,
  /* .ports = */ zxmmc_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

/* The card inserted into the ZXMMC. For now, we emulate only one card. */
static libspectrum_mmc_card *card;

/* The card currently selected via the "card select" call */
static libspectrum_mmc_card *current_card;

static void zxmmc_reset( int hard_reset );
static void zxmmc_enabled_snapshot( libspectrum_snap *snap );
static void zxmmc_to_snapshot( libspectrum_snap *snap );

static module_info_t zxmmc_module_info = {

  /* .reset = */ zxmmc_reset,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ zxmmc_enabled_snapshot,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ zxmmc_to_snapshot,

};

/* Eject menu item */
static const ui_menu_item eject_menu_item = UI_MENU_ITEM_MEDIA_IDE_ZXMMC_EJECT;

/* Housekeeping functions */

static int
zxmmc_init( void *context )
{
  card = libspectrum_mmc_alloc();

  ui_menu_activate( eject_menu_item, 0 );

  if( settings_current.zxmmc_file ) {
    int error;

    error =
      libspectrum_mmc_insert( card, settings_current.zxmmc_file );
    if( error ) return error;

    error = ui_menu_activate( eject_menu_item, 1 );
    if( error ) return error;
  }

  module_register( &zxmmc_module_info );

  periph_register( PERIPH_TYPE_ZXMMC, &zxmmc_periph );

  return 0;
}

static void
zxmmc_end( void )
{
  libspectrum_mmc_free( card );
}

void
zxmmc_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_ZXMMC, dependencies,
                            ARRAY_SIZE( dependencies ), zxmmc_init, NULL,
                            zxmmc_end );
}

static void
zxmmc_reset( int hard_reset )
{
  libspectrum_mmc_reset( card );
}

static int
dirty_fn_wrapper( void *context )
{
  return libspectrum_mmc_dirty( (libspectrum_mmc_card*)context );
}

static libspectrum_error
commit_fn_wrapper( void *context )
{
  libspectrum_mmc_commit( (libspectrum_mmc_card*)context );
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
eject_fn_wrapper( void *context )
{
  libspectrum_mmc_eject( (libspectrum_mmc_card*)context );
  return LIBSPECTRUM_ERROR_NONE;
}

static int
mmc_eject( libspectrum_mmc_card *card )
{
  return ide_eject_mass_storage( dirty_fn_wrapper, commit_fn_wrapper,
      eject_fn_wrapper, card,
      "Card has been modified.\nDo you want to save it?",
      &settings_current.zxmmc_file,
      eject_menu_item );
}

int
zxmmc_insert( const char *filename )
{
  int error;

  /* Remove any currently inserted card; abort if we want to keep the current
     card */
  if( settings_current.zxmmc_file )
    if( mmc_eject( card ) )
      return 0;

  settings_set_string( &settings_current.zxmmc_file, filename );

  error = libspectrum_mmc_insert( card, filename );
  if( error ) return error;
  return ui_menu_activate( eject_menu_item, 1 );
}

void
zxmmc_commit( void )
{
  libspectrum_mmc_commit( card );
}

int
zxmmc_eject( void )
{
  return mmc_eject( card );
}

/* Port read/writes */

static void
zxmmc_card_select( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  /* D0 = MMC0, D1 = MMC1, active LOW
     somehow logic prevents enabling both cards at the same time */
  switch( data & 0x03 ) {
    case 0x02:
      current_card = card;
      break;
    case 0x01:
      /* TODO: select second card */
      current_card = NULL;
      break;
    default:
      current_card = NULL;
      break;
  }
}

static libspectrum_byte
zxmmc_mmc_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff;

  return current_card ? libspectrum_mmc_read( card ) : 0xff;
}

static void
zxmmc_mmc_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( current_card ) libspectrum_mmc_write( card, data );
}

static void
zxmmc_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_zxmmc_active( snap ) )
    settings_current.zxmmc_enabled = 1;
}

static void
zxmmc_to_snapshot( libspectrum_snap *snap )
{
  if( !settings_current.zxmmc_enabled ) return;
  libspectrum_snap_set_zxmmc_active( snap, 1 );
}
