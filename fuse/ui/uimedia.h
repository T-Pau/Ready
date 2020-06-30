/* uimedia.h: Disk media UI routines
   Copyright (c) 2013 Alex Badea
   Copyright (c) 2015 Gergely Szasz

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

#ifndef FUSE_UIMEDIA_H
#define FUSE_UIMEDIA_H

struct fdd_t;
struct fdd_params_t;
struct disk_t;
struct ui_media_drive_info_t;

typedef int (*ui_media_drive_is_available_fn)( void );
typedef const struct fdd_params_t* (*ui_media_drive_get_params_fn) ( void );
typedef int (*ui_media_drive_insert_hook_fn)(
  const struct ui_media_drive_info_t *drive, int new );
typedef int (*ui_media_drive_autoload_hook_fn)( void );

typedef struct ui_media_drive_info_t
{
  const char *name;
  int controller_index;
  int drive_index;
  int menu_item_parent;
  int menu_item_top;
  int menu_item_eject;
  int menu_item_flip;
  int menu_item_wp;
  ui_media_drive_is_available_fn is_available;
  ui_media_drive_get_params_fn get_params;
  ui_media_drive_insert_hook_fn insert_hook;
  ui_media_drive_autoload_hook_fn autoload_hook;
  struct fdd_t *fdd;
} ui_media_drive_info_t;

int ui_media_drive_register( ui_media_drive_info_t *drive );
void ui_media_drive_end( void );
ui_media_drive_info_t *ui_media_drive_find( int controller, int drive );

#define UI_MEDIA_DRIVE_UPDATE_ALL	(~0)
#define UI_MEDIA_DRIVE_UPDATE_TOP	(1 << 0)
#define UI_MEDIA_DRIVE_UPDATE_EJECT	(1 << 1)
#define UI_MEDIA_DRIVE_UPDATE_FLIP	(1 << 2)
#define UI_MEDIA_DRIVE_UPDATE_WP	(1 << 3)

int ui_media_drive_any_available( void );
void ui_media_drive_update_parent_menus( void );
void ui_media_drive_update_menus( const ui_media_drive_info_t *drive,
                                  unsigned flags );
int ui_media_drive_eject_all( void );

int ui_media_drive_insert( const ui_media_drive_info_t *drive,
                           const char *filename, int autoload );
int ui_media_drive_save( int controller, int which, int saveas );
int ui_media_drive_eject( int controller, int which );
int ui_media_drive_flip( int controller, int which, int flip );
int ui_media_drive_writeprotect( int controller, int which, int wrprot );

/* These are (also) used in media menu items */
typedef enum ui_media_controller {
  UI_MEDIA_CONTROLLER_PLUS3,
  UI_MEDIA_CONTROLLER_BETA,
  UI_MEDIA_CONTROLLER_PLUSD,
  UI_MEDIA_CONTROLLER_MDR,
  UI_MEDIA_CONTROLLER_OPUS,
  UI_MEDIA_CONTROLLER_DISCIPLE,
  UI_MEDIA_CONTROLLER_DIDAKTIK,
} ui_media_controller;

#endif			/* #ifndef FUSE_UIMEDIA_H */
