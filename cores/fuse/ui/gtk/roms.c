/* roms.c: ROM selector dialog box
   Copyright (c) 2003-2004 Philip Kendall
   Copyright (c) 2015 Stuart Brady

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
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "menu.h"
#include "settings.h"
#include "ui/ui.h"

static void add_rom( GtkBox *parent, size_t start, gint row,
		     int is_peripheral );
static void select_new_rom( GtkWidget *widget, gpointer data );
static void roms_done( GtkButton *button, gpointer data );

/* The labels used to display the current ROMs */
static GtkWidget *rom[ SETTINGS_ROM_COUNT ];

struct callback_info {

  size_t start, n;
  int is_peripheral;

};

int
menu_select_roms_with_title( const char *title, size_t start, size_t n,
			     int is_peripheral )
{
  GtkWidget *dialog;
  GtkBox *vbox;

  struct callback_info info;

  char buffer[ 256 ];
  size_t i;

  /* Firstly, stop emulation */
  fuse_emulation_pause();

  /* Give me a new dialog box */
  snprintf( buffer, 256, "Fuse - Select ROMs - %s", title );
  dialog = gtkstock_dialog_new( buffer, NULL );

  info.start = start;
  info.n = n;
  info.is_peripheral = is_peripheral;

  /* Create the OK and Cancel buttons */
  gtkstock_create_ok_cancel( dialog, NULL, G_CALLBACK( roms_done ), &info,
                             DEFAULT_DESTROY, DEFAULT_DESTROY );

  /* And the current values of each of the ROMs */
  vbox = GTK_BOX( gtk_dialog_get_content_area( GTK_DIALOG( dialog ) ) );

  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );
  for( i = 0; i < n; i++ ) add_rom( vbox, start, i, is_peripheral );

  /* Users shouldn't be able to resize this window */
  gtk_window_set_resizable( GTK_WINDOW( dialog ), FALSE );

  /* Display the window */
  gtk_widget_show_all( dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return 0;
}

static void
add_rom( GtkBox *parent, size_t start, gint row, int is_peripheral )
{
  GtkWidget *frame, *hbox, *change_button;
  char buffer[ 80 ], **setting;

  snprintf( buffer, 80, "ROM %d", row );
  frame = gtk_frame_new( buffer );
  gtk_box_pack_start( parent, frame, FALSE, FALSE, 2 );

  hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( hbox ), 4 );
  gtk_container_add( GTK_CONTAINER( frame ), hbox );

  setting = settings_get_rom_setting( &settings_current, start + row,
				      is_peripheral );
  rom[ row ] = gtk_entry_new();
  gtk_entry_set_text( GTK_ENTRY( rom[ row ] ), *setting );
  gtk_box_pack_start( GTK_BOX( hbox ), rom[ row ], FALSE, FALSE, 2 );

  change_button = gtk_button_new_with_label( "Select..." );
  g_signal_connect( G_OBJECT( change_button ), "clicked",
		    G_CALLBACK( select_new_rom ),
		    rom[ row ] );
  gtk_box_pack_start( GTK_BOX( hbox ), change_button, FALSE, FALSE, 2 );
}

static void
select_new_rom( GtkWidget *widget GCC_UNUSED, gpointer data )
{
  char *filename;

  GtkWidget *entry = data;

  filename = ui_get_open_filename( "Fuse - Select ROM" );
  if( !filename ) return;

  gtk_entry_set_text( GTK_ENTRY( entry ), filename );
}

static void
roms_done( GtkButton *button GCC_UNUSED, gpointer data )
{
  size_t i;
  
  char **setting; const char *string;

  struct callback_info *info = data;

  for( i = 0; i < info->n; i++ ) {

    setting = settings_get_rom_setting( &settings_current, info->start + i,
				        info->is_peripheral );
    string = gtk_entry_get_text( GTK_ENTRY( rom[i] ) );

    settings_set_string( setting, string );
  }
}
