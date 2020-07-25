/* uimedia.c: Disk media UI routines
   Copyright (c) 2013 Alex Badea
   Copyright (c) 2015-2017 Gergely Szasz

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

   E-mail: vamposdecampos@gmail.com

*/

#include <config.h>
#include <string.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif

#include "fuse.h"
#include "options.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "utils.h"

#define DISK_TRY_MERGE(heads) \
  ( option_enumerate_diskoptions_disk_try_merge() == 2 || \
    ( option_enumerate_diskoptions_disk_try_merge() == 1 && heads == 1 ) )

static GSList *registered_drives = NULL;

static inline int
menu_item_valid( ui_menu_item item )
{
  return item != UI_MENU_ITEM_INVALID;
}

int
ui_media_drive_register( ui_media_drive_info_t *drive )
{
  registered_drives = g_slist_append( registered_drives, drive );
  return 0;
}

void
ui_media_drive_end( void )
{
  g_slist_free( registered_drives );
  registered_drives = NULL;
}

struct find_info {
  int controller;
  int drive;
};

static gint
find_drive( gconstpointer data, gconstpointer user_data )
{
  const ui_media_drive_info_t *drive = data;
  const struct find_info *info = user_data;

  return !( drive->controller_index == info->controller &&
            drive->drive_index == info->drive );
}

ui_media_drive_info_t *
ui_media_drive_find( int controller, int drive )
{
  struct find_info info = {
    /* .controller = */ controller,
    /* .drive = */ drive,
  };
  GSList *item;
  item = g_slist_find_custom( registered_drives, &info, find_drive );
  return item ? item->data : NULL;
}


static gint
any_available( gconstpointer data, gconstpointer user_data )
{
  const ui_media_drive_info_t *drive = data;

  return !( drive->is_available && drive->is_available() );
}

int
ui_media_drive_any_available( void )
{
  GSList *item;
  item = g_slist_find_custom( registered_drives, NULL, any_available );
  return item ? 1 : 0;
}


static void
update_parent_menus( gpointer data, gpointer user_data )
{
  const ui_media_drive_info_t *drive = data;

  if( drive->is_available && menu_item_valid( drive->menu_item_parent ) )
    ui_menu_activate( drive->menu_item_parent, drive->is_available() );
}

void
ui_media_drive_update_parent_menus( void )
{
  g_slist_foreach( registered_drives, update_parent_menus, NULL );
}

static int
maybe_menu_activate( int id, int activate )
{
  if( !menu_item_valid( id ) )
    return 0;
  return ui_menu_activate( id, activate );
}

void
ui_media_drive_update_menus( const ui_media_drive_info_t *drive,
                             unsigned flags )
{
  if( !drive->fdd )
    return;

  if( flags & UI_MEDIA_DRIVE_UPDATE_TOP )
    maybe_menu_activate( drive->menu_item_top,
                         drive->fdd->type != FDD_TYPE_NONE );
  if( flags & UI_MEDIA_DRIVE_UPDATE_EJECT )
    maybe_menu_activate( drive->menu_item_eject, drive->fdd->loaded );
  if( flags & UI_MEDIA_DRIVE_UPDATE_FLIP )
    maybe_menu_activate( drive->menu_item_flip, !drive->fdd->upsidedown );
  if( flags & UI_MEDIA_DRIVE_UPDATE_WP )
    maybe_menu_activate( drive->menu_item_wp, !drive->fdd->wrprot );
}


int
ui_media_drive_flip( int controller, int which, int flip )
{
  ui_media_drive_info_t *drive;

  drive = ui_media_drive_find( controller, which );
  if( !drive )
    return -1;
  if( !drive->fdd->loaded )
    return 1;

  fdd_flip( drive->fdd, flip );
  ui_media_drive_update_menus( drive, UI_MEDIA_DRIVE_UPDATE_FLIP );
  return 0;
}

int
ui_media_drive_writeprotect( int controller, int which, int wrprot )
{
  ui_media_drive_info_t *drive;

  drive = ui_media_drive_find( controller, which );
  if( !drive )
    return -1;
  if( !drive->fdd->loaded )
    return 1;

  fdd_wrprot( drive->fdd, wrprot );
  ui_media_drive_update_menus( drive, UI_MEDIA_DRIVE_UPDATE_WP );
  return 0;
}


static int
drive_disk_write( const ui_media_drive_info_t *drive, const char *filename )
{
  int error;

  drive->fdd->disk.type = DISK_TYPE_NONE;
  if( filename == NULL )
    filename = drive->fdd->disk.filename; /* write over original file */
  else if( compat_file_exists( filename ) ) {
    const char *filename1 = strrchr( filename, FUSE_DIR_SEP_CHR );
    filename1 = filename1 ? filename1 + 1 : filename;

    ui_confirm_save_t confirm = ui_confirm_save(
      "%s already exists.\n"
      "Do you want to overwrite it?",
      filename1
    );

    switch( confirm ) {

    case UI_CONFIRM_SAVE_SAVE:
      break;

    case UI_CONFIRM_SAVE_DONTSAVE: return 2;
    case UI_CONFIRM_SAVE_CANCEL: return -1;

    }
  }

  error = disk_write( &drive->fdd->disk, filename );

  if( error != DISK_OK ) {
    ui_error( UI_ERROR_ERROR, "couldn't write '%s' file: %s", filename,
              disk_strerror( error ) );
    return 1;
  }

  if( !drive->fdd->disk.filename ||
      strcmp( filename, drive->fdd->disk.filename ) ) {
    libspectrum_free( drive->fdd->disk.filename );
    drive->fdd->disk.filename = utils_safe_strdup( filename );
  }
  return 0;
}


static int
drive_save( const ui_media_drive_info_t *drive, int saveas )
{
  int err;
  char *filename = NULL, title[80];

  if( drive->fdd->disk.filename == NULL )
    saveas = 1;

  fuse_emulation_pause();

  snprintf( title, sizeof( title ), "Fuse - Write %s", drive->name );
  if( saveas ) {
    filename = ui_get_save_filename( title );
    if( !filename ) {
      fuse_emulation_unpause();
      return 1;
    }
  }

  err = drive_disk_write( drive, filename );

  if( saveas )
    libspectrum_free( filename );

  fuse_emulation_unpause();
  if( err )
    return 1;

  drive->fdd->disk.dirty = 0;
  return 0;
}

int
ui_media_drive_save( int controller, int which, int saveas )
{
  ui_media_drive_info_t *drive;

  drive = ui_media_drive_find( controller, which );
  if( !drive )
    return -1;
  return drive_save( drive, saveas );
}

static int
drive_eject( const ui_media_drive_info_t *drive )
{
  if( !drive->fdd->loaded )
    return 0;

  if( drive->fdd->disk.dirty ) {

    ui_confirm_save_t confirm = ui_confirm_save(
      "%s has been modified.\n"
      "Do you want to save it?",
      drive->name
    );

    switch( confirm ) {

    case UI_CONFIRM_SAVE_SAVE:
      if( drive_save( drive, 0 ) )
        return 1;   /* first save it...*/
      break;

    case UI_CONFIRM_SAVE_DONTSAVE: break;
    case UI_CONFIRM_SAVE_CANCEL: return 1;

    }
  }

  fdd_unload( drive->fdd );
  disk_close( &drive->fdd->disk );
  ui_media_drive_update_menus( drive, UI_MEDIA_DRIVE_UPDATE_EJECT );
  return 0;
}

int
ui_media_drive_eject( int controller, int which )
{
  ui_media_drive_info_t *drive;

  drive = ui_media_drive_find( controller, which );
  if( !drive )
    return -1;
  return drive_eject( drive );
}

static gint
eject_all( gconstpointer data, gconstpointer user_data )
{
  const ui_media_drive_info_t *drive = data;

  return !drive_eject( drive );
}

int
ui_media_drive_eject_all( void )
{
  GSList *item;
  item = g_slist_find_custom( registered_drives, NULL, eject_all );
  return item ? 1 : 0;
}


int
ui_media_drive_insert( const ui_media_drive_info_t *drive,
                       const char *filename, int autoload )
{
  int error;
  const fdd_params_t *dt;

  /* Eject any disk already in the drive */
  if( drive->fdd->loaded ) {
    /* Abort the insert if we want to keep the current disk */
    if( drive_eject( drive ) )
      return 0;
  }

  if( filename ) {
    error = disk_open( &drive->fdd->disk, filename, 0,
                       DISK_TRY_MERGE( drive->fdd->fdd_heads ) );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to open disk image: %s",
                disk_strerror( error ) );
      return 1;
    }
  } else {
    dt = drive->get_params();
    error = disk_new( &drive->fdd->disk, dt->heads, dt->cylinders, DISK_DENS_AUTO,
                      DISK_UDI );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to create disk image: %s",
                disk_strerror( error ) );
      return 1;
    }
  }
  if( drive->insert_hook ) {
    error = drive->insert_hook( drive, !filename );
    if( error )
      return 1;
  }

  fdd_load( drive->fdd, 0 );

  /* Set the 'eject' item active */
  ui_media_drive_update_menus( drive, UI_MEDIA_DRIVE_UPDATE_ALL );

  if( filename && autoload && drive->autoload_hook ) {
    return drive->autoload_hook();
  }

  return 0;
}
