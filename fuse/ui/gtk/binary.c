/* binary.c: GTK+ routines to load/save chunks of binary data
   Copyright (c) 2003-2013 Philip Kendall
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

#include <errno.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <libspectrum.h>

#include "fuse.h"
#include "gtkinternals.h"
#include "memory_pages.h"
#include "menu.h"
#include "ui/ui.h"
#include "utils.h"

struct binary_info {

  char *filename;
  utils_file file;

  int load;
  GCallback activate_data;
  GCallback change_filename;

  GtkWidget *dialog;
  GtkWidget *filename_widget, *start_widget, *length_widget;
};

static void change_load_filename( GtkButton *button, gpointer user_data );
static void load_data( GtkEntry *entry, gpointer user_data );

static void change_save_filename( GtkButton *button, gpointer user_data );
static void save_data( GtkEntry *entry, gpointer user_data );

static void
create_binary_dialog( struct binary_info *info, const char *title )
{
  GtkWidget *table, *button, *content_area;
  GtkWidget *label_filename, *label_start, *label_length;

  char buffer[80];

  info->dialog = gtkstock_dialog_new( title, NULL );

  content_area = gtk_dialog_get_content_area( GTK_DIALOG( info->dialog ) );

  label_filename = gtk_label_new( "Filename" );

  info->filename_widget = gtk_label_new( info->filename );

  button = gtk_button_new_with_label( "Browse..." );
  g_signal_connect( G_OBJECT( button ), "clicked", info->change_filename,
                    info );

  label_start = gtk_label_new( "Start" );

  info->start_widget = gtk_entry_new();
  g_signal_connect( G_OBJECT( info->start_widget ), "activate",
                    info->activate_data, info );

  label_length = gtk_label_new( "Length" );

  info->length_widget = gtk_entry_new();
  if( info->load ) {
    snprintf( buffer, 80, "%lu", (unsigned long)info->file.length );
    gtk_entry_set_text( GTK_ENTRY( info->length_widget ), buffer );
  }

  g_signal_connect( G_OBJECT( info->length_widget ), "activate",
                    info->activate_data, info );

#if GTK_CHECK_VERSION( 3, 0, 0 )

  table = gtk_grid_new();
  gtk_grid_set_column_homogeneous( GTK_GRID( table ), FALSE );
  gtk_grid_set_column_spacing( GTK_GRID( table ), 6 );
  gtk_grid_set_row_spacing( GTK_GRID( table ), 6 );
  gtk_container_set_border_width( GTK_CONTAINER( table ), 6 );
  gtk_container_add( GTK_CONTAINER( content_area ), table );

  gtk_grid_attach( GTK_GRID( table ), label_filename, 0, 0, 1, 1 );
  gtk_widget_set_hexpand( info->filename_widget, TRUE );
  gtk_grid_attach( GTK_GRID( table ), info->filename_widget,
                   1, 0, 1, 1 );
  gtk_grid_attach( GTK_GRID( table ), button, 2, 0, 1, 1 );
  gtk_grid_attach( GTK_GRID( table ), label_start, 0, 1, 1, 1 );
  gtk_grid_attach( GTK_GRID( table ), info->start_widget, 1, 1, 2, 1 );
  gtk_grid_attach( GTK_GRID( table ), label_length, 0, 2, 1, 1 );
  gtk_grid_attach( GTK_GRID( table ), info->length_widget, 1, 2, 2, 1 );

#else                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */

  table = gtk_table_new( 3, 3, FALSE );
  gtk_box_pack_start( GTK_BOX( content_area ), table, TRUE, TRUE, 0 );

  gtk_table_attach( GTK_TABLE( table ), label_filename, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), info->filename_widget, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), button, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), label_start, 0, 1, 1, 2, 0, 0, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), info->start_widget, 1, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), label_length, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 3, 3 );

  gtk_table_attach( GTK_TABLE( table ), info->length_widget, 1, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 3, 3 );

#endif

  GtkAccelGroup *accel_group;
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( info->dialog ), accel_group );

  /* Command buttons */
  gtkstock_create_ok_cancel( info->dialog, NULL, info->activate_data, info,
                             DEFAULT_DESTROY, DEFAULT_DESTROY );
}

void
menu_file_loadbinarydata( GtkAction *gtk_action GCC_UNUSED,
                          gpointer data GCC_UNUSED )
{
  struct binary_info info;

  int error;

  fuse_emulation_pause();

  info.filename = ui_get_open_filename( "Fuse - Load Binary Data" );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  error = utils_read_file( info.filename, &info.file );
  if( error ) { free( info.filename ); fuse_emulation_unpause(); return; }

  /* Information display */
  info.load = 1;
  info.activate_data = G_CALLBACK( load_data );
  info.change_filename = G_CALLBACK( change_load_filename );

  create_binary_dialog( &info, "Fuse - Load Binary Data" );

  /* Process the dialog */
  gtk_widget_show_all( info.dialog );
  gtk_main();

  free( info.filename );
  utils_close_file( &info.file );

  fuse_emulation_unpause();
}

static void
change_load_filename( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  struct binary_info *info = user_data;
  
  char *new_filename;
  utils_file new_file;

  char buffer[80];
  int error;

  new_filename = ui_get_open_filename( "Fuse - Load Binary Data" );
  if( !new_filename ) return;

  error = utils_read_file( new_filename, &new_file );
  if( error ) { free( new_filename ); return; }

  /* Remove the data for the old file */
  utils_close_file( &info->file );

  free( info->filename );

  /* Put the new data in */
  info->filename = new_filename; info->file = new_file;

  /* And update the displayed information */
  gtk_label_set_text( GTK_LABEL( info->filename_widget ), new_filename );
  
  snprintf( buffer, 80, "%lu", (unsigned long)info->file.length );
  gtk_entry_set_text( GTK_ENTRY( info->length_widget ), buffer );
}

static void
load_data( GtkEntry *entry GCC_UNUSED, gpointer user_data )
{
  struct binary_info *info = user_data;

  long start, length; size_t i;
  const gchar *nptr;
  char *endptr;
  int base;

  errno = 0;
  nptr = gtk_entry_get_text( GTK_ENTRY( info->length_widget ) );
  base = ( g_str_has_prefix( nptr, "0x" ) )? 16 : 10;
  length = strtol( nptr, &endptr, base );

  if( errno || length < 1 || length > 0x10000 || endptr == nptr ) {
    ui_error( UI_ERROR_ERROR, "Length must be between 1 and 65536" );
    return;
  }

  if( length > info->file.length ) {
    ui_error( UI_ERROR_ERROR,
	      "'%s' contains only %lu bytes",
	      info->filename, (unsigned long)info->file.length );
    return;
  }

  errno = 0;
  nptr = gtk_entry_get_text( GTK_ENTRY( info->start_widget ) );
  base = ( g_str_has_prefix( nptr, "0x" ) )? 16 : 10;
  start = strtol( nptr, &endptr, base );

  if( errno || start < 0 || start > 0xffff || endptr == nptr ) {
    ui_error( UI_ERROR_ERROR, "Start must be between 0 and 65535" );
    return;
  }

  if( start + length > 0x10000 ) {
    ui_error( UI_ERROR_ERROR, "Block ends after address 65535" );
    return;
  }

  for( i = 0; i < length; i++ )
    writebyte_internal( start + i, info->file.buffer[ i ] );

  gtkui_destroy_widget_and_quit( info->dialog, NULL );
}
  
void
menu_file_savebinarydata( GtkAction *gtk_action GCC_UNUSED,
                          gpointer data GCC_UNUSED )
{
  struct binary_info info;

  fuse_emulation_pause();

  info.filename = ui_get_save_filename( "Fuse - Save Binary Data" );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  info.activate_data = G_CALLBACK( save_data );
  info.change_filename = G_CALLBACK( change_save_filename );
  info.load = 0;

  create_binary_dialog( &info, "Fuse - Save Binary Data" );

  /* Process the dialog */
  gtk_widget_show_all( info.dialog );
  gtk_main();

  free( info.filename );

  fuse_emulation_unpause();
}

static void
change_save_filename( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  struct binary_info *info = user_data;
  char *new_filename;

  new_filename = ui_get_save_filename( "Fuse - Save Binary Data" );
  if( !new_filename ) return;

  free( info->filename );

  info->filename = new_filename;

  gtk_label_set_text( GTK_LABEL( info->filename_widget ), new_filename );
}

static void
save_data( GtkEntry *entry GCC_UNUSED, gpointer user_data )
{
  struct binary_info *info = user_data;

  long start, length;
  const gchar *nptr;
  char *endptr;
  int base;

  int error;

  errno = 0;
  nptr = gtk_entry_get_text( GTK_ENTRY( info->length_widget ) );
  base = ( g_str_has_prefix( nptr, "0x" ) )? 16 : 10;
  length = strtol( nptr, &endptr, base );

  if( errno || length < 1 || length > 0x10000 || endptr == nptr ) {
    ui_error( UI_ERROR_ERROR, "Length must be between 1 and 65536" );
    return;
  }

  errno = 0;
  nptr = gtk_entry_get_text( GTK_ENTRY( info->start_widget ) );
  base = ( g_str_has_prefix( nptr, "0x" ) )? 16 : 10;
  start = strtol( nptr, &endptr, base );

  if( errno || start < 0 || start > 0xffff || endptr == nptr ) {
    ui_error( UI_ERROR_ERROR, "Start must be between 0 and 65535" );
    return;
  }

  if( start + length > 0x10000 ) {
    ui_error( UI_ERROR_ERROR, "Block ends after address 65535" );
    return;
  }

  error = utils_save_binary( start, length, info->filename );
  if( error ) return;

  gtkui_destroy_widget_and_quit( info->dialog, NULL );
}
