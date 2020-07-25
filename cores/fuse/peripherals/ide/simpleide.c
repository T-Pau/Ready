/* simpleide.c: Simple 8-bit IDE interface routines
   Copyright (c) 2003-2017 Garry Lancaster, Philip Kendall, Fredrick Meunier
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

#include <libspectrum.h>

#include "ide.h"
#include "infrastructure/startup_manager.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "simpleide.h"
#include "ui/ui.h"

/* Private function prototypes */

static libspectrum_byte simpleide_read( libspectrum_word port, libspectrum_byte *attached );
static void simpleide_write( libspectrum_word port, libspectrum_byte data );

/* Data */

static const periph_port_t simpleide_ports[] = {
  { 0x0010, 0x0000, simpleide_read, simpleide_write },
  { 0, 0, NULL, NULL }
};

static const periph_t simpleide_periph = {
  /* .option = */ &settings_current.simpleide_active,
  /* .ports = */ simpleide_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static libspectrum_ide_channel *simpleide_idechn;

static void simpleide_snapshot_enabled( libspectrum_snap *snap );
static void simpleide_to_snapshot( libspectrum_snap *snap );

static module_info_t simpleide_module_info = {

  /* .reset = */ simpleide_reset,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ simpleide_snapshot_enabled,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ simpleide_to_snapshot,

};

/* Housekeeping functions */

static int
simpleide_init( void *context )
{
  int error;

  simpleide_idechn = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA8 );

  error = ide_init( simpleide_idechn,
		    settings_current.simpleide_master_file,
		    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
		    settings_current.simpleide_slave_file,
		    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT );
  if( error ) return error;

  module_register( &simpleide_module_info );
  periph_register( PERIPH_TYPE_SIMPLEIDE, &simpleide_periph );

  return 0;
}

static void
simpleide_end( void )
{
  libspectrum_ide_free( simpleide_idechn );
}

void
simpleide_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_SETUID
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_SIMPLEIDE, dependencies,
                            ARRAY_SIZE( dependencies ), simpleide_init, NULL,
                            simpleide_end );
}

void
simpleide_reset( int hard_reset GCC_UNUSED )
{
  libspectrum_ide_reset( simpleide_idechn );
}

int
simpleide_insert( const char *filename, libspectrum_ide_unit unit )
{
  return ide_master_slave_insert(
    simpleide_idechn, unit, filename,
    &settings_current.simpleide_master_file,
    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
    &settings_current.simpleide_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT );
}

int
simpleide_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( simpleide_idechn, unit );

  return error;
}

int
simpleide_eject( libspectrum_ide_unit unit )
{
  return ide_master_slave_eject(
    simpleide_idechn, unit,
    &settings_current.simpleide_master_file,
    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
    &settings_current.simpleide_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT );
}

/* Port read/writes */

static libspectrum_byte
simpleide_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_ide_register idereg;
  
  *attached = 0xff; /* TODO: check this */
  
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  return libspectrum_ide_read( simpleide_idechn, idereg ); 
}  

static void
simpleide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  libspectrum_ide_write( simpleide_idechn, idereg, data ); 
}

static void
simpleide_snapshot_enabled( libspectrum_snap *snap )
{
  if( libspectrum_snap_simpleide_active( snap ) )
    settings_current.simpleide_active = 1;
}

static void
simpleide_to_snapshot( libspectrum_snap *snap )
{
  if( !settings_current.simpleide_active ) return;

  libspectrum_snap_set_simpleide_active( snap, 1 );
}
