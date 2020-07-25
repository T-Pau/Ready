/* ide.c: Generic routines shared between the various IDE devices
   Copyright (c) 2005-2017 Philip Kendall

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
#include "ui/ui.h"
#include "settings.h"

static int
ide_insert_file( libspectrum_ide_channel *channel, libspectrum_ide_unit unit,
		 const char *filename, ui_menu_item menu_item )
{
  int error;

  error = libspectrum_ide_insert( channel, unit, filename );
  if( error ) return error;
  return ui_menu_activate( menu_item, 1 );
}

int
ide_init( libspectrum_ide_channel *channel,
	  char *master_setting, ui_menu_item master_menu_item,
	  char *slave_setting, ui_menu_item slave_menu_item )
{
  int error;

  ui_menu_activate( master_menu_item, 0 );
  ui_menu_activate( slave_menu_item, 0 );

  if( master_setting ) {
    error = ide_insert_file( channel, LIBSPECTRUM_IDE_MASTER, master_setting,
		             master_menu_item );
    if( error ) return error;
  }

  if( slave_setting ) {
    error = ide_insert_file( channel, LIBSPECTRUM_IDE_SLAVE, slave_setting,
                             slave_menu_item );
    if( error ) return error;
  }

  return 0;
}

int
ide_master_slave_insert(
  libspectrum_ide_channel *channel, libspectrum_ide_unit unit,
  const char *filename,
  char **master_setting, ui_menu_item master_menu_item,
  char **slave_setting, ui_menu_item slave_menu_item )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {

  case LIBSPECTRUM_IDE_MASTER:
    setting = master_setting;
    item = master_menu_item;
    break;

  case LIBSPECTRUM_IDE_SLAVE:
    setting = slave_setting;
    item = slave_menu_item;
    break;

    default: return 1;
  }

  return ide_insert( filename, channel, unit, setting, item );
}

int
ide_insert( const char *filename, libspectrum_ide_channel *chn,
	    libspectrum_ide_unit unit, char **setting, ui_menu_item item )
{
  int error;

  /* Remove any currently inserted disk; abort if we want to keep the current
     disk */
  if( *setting )
    if( ide_eject( chn, unit, setting, item ) )
      return 0;

  settings_set_string( setting, filename );

  error = ide_insert_file( chn, unit, filename, item );
  if( error ) return error;

  return 0;
}

int
ide_master_slave_eject(
  libspectrum_ide_channel *channel, libspectrum_ide_unit unit,
  char **master_setting, ui_menu_item master_menu_item,
  char **slave_setting, ui_menu_item slave_menu_item )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    setting = master_setting;
    item = master_menu_item;
    break;

  case LIBSPECTRUM_IDE_SLAVE:
    setting = slave_setting;
    item = slave_menu_item;
    break;

  default: return 1;
  }

  return ide_eject( channel, unit, setting, item );
}

int
ide_eject_mass_storage(
    int (*is_dirty_fn)( void *context ),
    libspectrum_error (*commit_fn)( void *context ),
    libspectrum_error (*eject_fn)( void *context ),
    void *context, const char *message, char **setting, ui_menu_item item )
{
  int error;

  if( is_dirty_fn( context ) ) {
    
    ui_confirm_save_t confirm = ui_confirm_save( "%s", message );
  
    switch( confirm ) {

    case UI_CONFIRM_SAVE_SAVE:
      error = commit_fn( context ); if( error ) return error;
      break;

    case UI_CONFIRM_SAVE_DONTSAVE: break;
    case UI_CONFIRM_SAVE_CANCEL: return 1;

    }
  }

  libspectrum_free( *setting ); *setting = NULL;
  
  error = eject_fn( context );
  if( error ) return error;

  error = ui_menu_activate( item, 0 );
  if( error ) return error;

  return 0;
}

struct eject_fn_context {
  libspectrum_ide_channel *chn;
  libspectrum_ide_unit unit;
};

static int
dirty_fn_wrapper( void *context )
{
  struct eject_fn_context *ctx = (struct eject_fn_context*)context;
  return libspectrum_ide_dirty( ctx->chn, ctx->unit );
}

static libspectrum_error
commit_fn_wrapper( void *context )
{
  struct eject_fn_context *ctx = (struct eject_fn_context*)context;
  return libspectrum_ide_commit( ctx->chn, ctx->unit );
}

static libspectrum_error
eject_fn_wrapper( void *context )
{
  struct eject_fn_context *ctx = (struct eject_fn_context*)context;
  return libspectrum_ide_eject( ctx->chn, ctx->unit );
}

int
ide_eject( libspectrum_ide_channel *chn, libspectrum_ide_unit unit,
	   char **setting, ui_menu_item item )
{
  struct eject_fn_context ctx = { chn, unit };
  return ide_eject_mass_storage( dirty_fn_wrapper, commit_fn_wrapper,
      eject_fn_wrapper, &ctx,
      "Hard disk has been modified.\nDo you want to save it?",
      setting, item );
}
