/* pokemem.c: GTK+ interface that handles pok files
   Copyright (c) 2011-2015 Philip Kendall, Sergio Baldoví

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
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "menu.h"
#include "pokefinder/pokemem.h"
#include "ui/ui.h"

enum
{
  COL_CHECK = 0,
  COL_NAME,
  COL_VALUE,
  COL_TRAINER,
  COL_ENABLED,
  COL_EDITABLE,
  COL_INCONSISTENT,
  NUM_COLS
};

static GtkWidget
  *dialog,        /* The dialog box itself */
  *poke_list,     /* The list of possible pokes */
  *bank,          /* Entries for custom poke */
  *address,
  *value;

void create_dialog( void );
void create_and_fill_treeview( void );
static void pokemem_close( GtkWidget *widget, gpointer user_data );
static void pokemem_update_list( GtkWidget *widget, gpointer user_data );
gboolean pokemem_update_trainer( GtkTreeModel *model, GtkTreePath *path,
                                 GtkTreeIter *iter, gpointer data );
static void pokemem_add_custom_poke( GtkWidget *widget, gpointer user_data );
static void trainer_add( gpointer data, gpointer user_data );

void entry_validate_digit( GtkEntry *entry, const gchar *text, gint length,
                           gint *position, gpointer data );
void entry_validate_address( GtkEntry *entry, const gchar *text, gint length,
                             gint *position, gpointer data );

void row_toggled_callback( GtkCellRendererToggle *cell, gchar *path_string,
                           gpointer user_data );

static gboolean custom_value_edit( GtkTreeView *tree );
void custom_value_changed( GtkCellRendererText *cell, gchar *path_string,
                           gchar *new_text, gpointer user_data );

void
menu_machine_pokememory( GtkAction *gtk_action GCC_UNUSED,
                         gpointer data GCC_UNUSED )
{
  fuse_emulation_pause();

  pokemem_autoload_pokfile();
  create_dialog();

  fuse_emulation_unpause();
}

void
ui_pokemem_selector( const char *filename )
{
  fuse_emulation_pause();

  pokemem_read_from_file( filename );
  create_dialog();

  fuse_emulation_unpause();
}

void
create_dialog( void )
{
  GtkWidget *hbox, *vbox, *label, *scroll;
  GtkAccelGroup *accel_group;

  dialog = gtkstock_dialog_new( "Fuse - Poke Memory",
                                G_CALLBACK( pokemem_close ) );

  vbox = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );

  /* Keyboard shortcuts */
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );    
  gtk_box_pack_start( GTK_BOX( vbox ), hbox,  TRUE, TRUE, 5 );

  /* Bank */
  label = gtk_label_new( "Bank:" );
  bank = gtk_entry_new();
  gtk_entry_set_width_chars( GTK_ENTRY( bank ), 7 );
  gtk_entry_set_max_length( GTK_ENTRY( bank ), 1 );
  g_signal_connect( G_OBJECT( bank ), "activate",
                    G_CALLBACK( pokemem_add_custom_poke ), NULL );
  g_signal_connect( G_OBJECT( bank ), "insert_text",
                    G_CALLBACK( entry_validate_digit ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );
  gtk_box_pack_start( GTK_BOX( hbox ), bank, TRUE, TRUE, 5 );

  /* Address */
  label = gtk_label_new( "Address:" );
  address = gtk_entry_new();
  gtk_entry_set_width_chars( GTK_ENTRY( address ), 7 );
  gtk_entry_set_max_length( GTK_ENTRY( address ), 6 );
  g_signal_connect( G_OBJECT( address ), "activate",
                    G_CALLBACK( pokemem_add_custom_poke ), NULL );
  g_signal_connect( G_OBJECT( address ), "insert_text",
                    G_CALLBACK( entry_validate_address ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );
  gtk_box_pack_start( GTK_BOX( hbox ), address, TRUE, TRUE, 5 );

  /* Value */
  label = gtk_label_new( "Value:" );
  value = gtk_entry_new();
  gtk_entry_set_width_chars( GTK_ENTRY( value ), 7 );
  gtk_entry_set_max_length( GTK_ENTRY( value ), 3 );
  g_signal_connect( G_OBJECT( value ), "activate",
                    G_CALLBACK( pokemem_add_custom_poke ), NULL );
  g_signal_connect( G_OBJECT( value ), "insert_text",
                    G_CALLBACK( entry_validate_digit ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );
  gtk_box_pack_start( GTK_BOX( hbox ), value, TRUE, TRUE, 5 );

  /* Create Add button for custom pokes */
  static const gtkstock_button
    add  = { "_Add", G_CALLBACK( pokemem_add_custom_poke ), NULL, NULL,
             0, 0, 0, 0, GTK_RESPONSE_NONE };
  gtkstock_create_button( GTK_WIDGET( hbox ), accel_group, &add );

  label = gtk_label_new( "Choose active POKES:" );
  gtk_box_pack_start( GTK_BOX( vbox ), label, TRUE, TRUE, 5 );

  /* Create list widget */
  create_and_fill_treeview();
  scroll = gtk_scrolled_window_new( NULL, NULL );

  /* Adjust size for list */ 
#if !GTK_CHECK_VERSION( 3, 0, 0 )
  gtk_widget_set_size_request( GTK_WIDGET( poke_list ), -1, 250 );
#else
  gtk_scrolled_window_set_min_content_height( GTK_SCROLLED_WINDOW( scroll ), 250 );
#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

  gtk_container_add( GTK_CONTAINER( scroll ), GTK_WIDGET( poke_list ) );
  gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( scroll ), TRUE, TRUE, 5 );

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog, accel_group,
                             G_CALLBACK( pokemem_update_list ),
                             (gpointer) &dialog,
                             G_CALLBACK( pokemem_close ),
                             G_CALLBACK( pokemem_close ) );

  /* Users shouldn't be able to resize this window */
  gtk_window_set_resizable( GTK_WINDOW( dialog ), FALSE );

  /* Process the dialog */
  gtk_widget_show_all( dialog );
  gtk_main();
}

void
create_and_fill_treeview( void )
{
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;

  poke_list = gtk_tree_view_new();

#if !GTK_CHECK_VERSION( 3, 0, 0 )
  gtk_tree_view_set_rules_hint( GTK_TREE_VIEW( poke_list ), TRUE );
#endif

  store = gtk_list_store_new( NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN, G_TYPE_BOOLEAN );

  renderer = gtk_cell_renderer_toggle_new();

  /* First column, checkbox */
  g_signal_connect( renderer, "toggled",
                    (GCallback) row_toggled_callback,
                    (gpointer) GTK_TREE_MODEL( store ) );

  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( poke_list ),
                                               -1,
                                               "Active",
                                               renderer,
                                               "active", COL_CHECK,
                                               "activatable", COL_ENABLED,
                                               "inconsistent", COL_INCONSISTENT,
                                               NULL );

  /* Second column, name */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( poke_list ),
                                               -1,
                                               "Trainer",
                                               renderer,
                                               "text", COL_NAME,
                                               NULL );

  /* Third column, optional value */
  renderer = gtk_cell_renderer_text_new();
  g_signal_connect( renderer, "edited", (GCallback)custom_value_changed,
                    (gpointer) GTK_TREE_MODEL( store ) );

  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( poke_list ),
                                               -1,
                                               "Value",
                                               renderer,
                                               "text", COL_VALUE,
                                               "editable", COL_EDITABLE,
                                               NULL);

  /* Create and fill model */
  if( trainer_list ) {
    g_slist_foreach( trainer_list, trainer_add, store );
  }
  model = GTK_TREE_MODEL( store );

  gtk_tree_view_set_model( GTK_TREE_VIEW( poke_list ), model );

  g_object_unref( model );
}

static void
trainer_add( gpointer data, gpointer user_data )
{
  gchar *val = NULL;
  GtkListStore *store = user_data;
  GtkTreeIter iter;
  trainer_t *trainer = data;

  if( !trainer ) return;

  if( trainer->ask_value )
    val = g_strdup_printf( "%d", trainer->value );

  /* Append a new row and fill data */
  gtk_list_store_append( store, &iter );
  gtk_list_store_set( store, &iter,
                      COL_CHECK, trainer->active,
                      COL_NAME, trainer->name,
                      COL_VALUE, ( trainer->ask_value )? val : NULL,
                      COL_TRAINER, trainer,
                      COL_ENABLED, !trainer->disabled,
                      COL_EDITABLE, ( !trainer->disabled && !trainer->active
                                      && trainer->ask_value ),
                      COL_INCONSISTENT, ( trainer->disabled && !trainer->active ),
                      -1);

  g_free( val );
}

static void
pokemem_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtkui_destroy_widget_and_quit( widget, NULL );
}

static void
pokemem_update_list( GtkWidget *widget GCC_UNUSED,
                     gpointer user_data GCC_UNUSED )
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model( GTK_TREE_VIEW( poke_list ) );
  if( model ) {
    gtk_tree_model_foreach( model, pokemem_update_trainer, NULL );
  }

  gtkui_destroy_widget_and_quit( dialog, NULL );
}

gboolean
pokemem_update_trainer( GtkTreeModel *model, GtkTreePath *path GCC_UNUSED,
                        GtkTreeIter *iter, gpointer data GCC_UNUSED )
{
  gboolean selected;
  trainer_t *trainer;

  gtk_tree_model_get( model, iter,
                      COL_CHECK, &selected,
                      COL_TRAINER, &trainer,
                      -1);

  if( selected ) {
    pokemem_trainer_activate( trainer );
  } else {
    pokemem_trainer_deactivate( trainer );
  }

  return FALSE;
}

static void
pokemem_add_custom_poke( GtkWidget *widget GCC_UNUSED,
                         gpointer user_data GCC_UNUSED )
{
  long b, a, v;
  const gchar *entry;
  char *endptr;
  int base;
  trainer_t *trainer;
  GtkListStore *store;

  if( gtk_entry_get_text_length( GTK_ENTRY( address ) ) == 0 &&
      gtk_entry_get_text_length( GTK_ENTRY( value ) ) == 0 ) {
     return;
  }

  /* Parse bank */
  entry = gtk_entry_get_text( GTK_ENTRY( bank ) );
  errno = 0;
  b = strtol( entry, &endptr, 10 );
  if( errno || b < 0 || b > 8 ) {
    ui_error( UI_ERROR_ERROR, "Invalid bank: use an integer from 0 to 8" );
    gtk_widget_grab_focus( bank );
    return;
  }

  if( endptr == entry ) b = 8; /* ignore bank by default */

  /* Parse address */
  entry = gtk_entry_get_text( GTK_ENTRY( address ) );
  errno = 0;
  base = ( g_str_has_prefix( entry, "0x" ) )? 16 : 10;
  a = strtol( entry, &endptr, base );

  if( errno || a < 0 || a > 65535 || endptr == entry ) {
    ui_error( UI_ERROR_ERROR,
              "Invalid address: use an integer from 0 to 65535" );
    gtk_widget_grab_focus( address );
    return;
  }

  if( b == 8 && a < 16384 ) {
    ui_error( UI_ERROR_ERROR,
              "Invalid address: use an integer from 16384 to 65535" );
    gtk_widget_grab_focus( address );
    return;
  }

  /* Parse value */
  entry = gtk_entry_get_text( GTK_ENTRY( value ) );
  errno = 0;
  v = strtol( entry, &endptr, 10 );

  if( errno || v < 0 || v > 256 || endptr == entry ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 256" );
    gtk_widget_grab_focus( value );
    return;
  }

  /* Update store and view */
  trainer = pokemem_trainer_list_add( b, a, v );
  if( !trainer ) {
    ui_error( UI_ERROR_ERROR, "Cannot add trainer" );
    return;
  }

  store = GTK_LIST_STORE( gtk_tree_view_get_model(
                          GTK_TREE_VIEW( poke_list ) ) );
  trainer_add( trainer, store );

  /* Mark custom trainer for activate */
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  int rows;

  if( !trainer->active && !trainer->disabled ) {
    model = gtk_tree_view_get_model( GTK_TREE_VIEW( poke_list ) );
    rows = gtk_tree_model_iter_n_children( model, NULL );
    path = gtk_tree_path_new_from_indices( rows - 1, -1 );
    gtk_tree_model_get_iter( model, &iter, path );
    gtk_list_store_set( GTK_LIST_STORE( model ), &iter, COL_CHECK, TRUE, -1 );
    gtk_tree_path_free( path );
  }

  /* Clear custom fields */
  gtk_editable_delete_text( GTK_EDITABLE( bank ), 0, -1 );
  gtk_editable_delete_text( GTK_EDITABLE( address ), 0, -1 );
  gtk_editable_delete_text( GTK_EDITABLE( value ), 0, -1 );
  gtk_widget_grab_focus( address );
}

void
entry_validate_digit( GtkEntry *entry, const gchar *text, gint length,
                      gint *position, gpointer data )
{
  int i, count = 0;
  GtkEditable *editable = GTK_EDITABLE( entry );
  gchar *result = g_new( gchar, length );

  /* Validate decimal and fill buffer to insert */
  for( i = 0; i < length; i++ ) {
    if( !isdigit( text[i] ) ) continue;

    result[ count++ ] = text[i];
  }

  /* Insert only validated text */
  if( count > 0 ) {
    g_signal_handlers_block_by_func( G_OBJECT( editable ),
                                     G_CALLBACK( entry_validate_digit ),
                                     data );
    gtk_editable_insert_text( editable, result, count, position );
    g_signal_handlers_unblock_by_func( G_OBJECT( editable ),
                                       G_CALLBACK( entry_validate_digit ),
                                       data );
  }
  g_signal_stop_emission_by_name( G_OBJECT( editable ), "insert_text" );

  g_free( result );
}

void
entry_validate_address( GtkEntry *entry, const gchar *text, gint length,
                        gint *position, gpointer data )
{
  int i, is_valid, is_hex;
  size_t n, prev_length, max_length, count = 0;
  GtkEditable *editable = GTK_EDITABLE( entry );
  GString *full_text;
  gchar *result = NULL;

  /* Validate hex or decimal number */
  full_text = g_string_new( gtk_entry_get_text( entry ) );
  prev_length = full_text->len;
  full_text = g_string_insert_len( full_text, *position, text, length );
  is_valid = 1;
  is_hex = 0;

  for( n = 0; n < full_text->len && is_valid; n++ ) {

    switch( n ) {
    case 0:
      is_valid = isdigit( full_text->str[n] );
      break;

    case 1:
      if( full_text->str[n] == 'x' ) {
          if( full_text->str[0] == '0' ) is_hex = 1;
          is_valid = is_hex;
      } else {
        is_valid = isdigit( full_text->str[n] );
      }
      break;

    default:
      is_valid = ( is_hex )? isxdigit( full_text->str[n] ) :
                             isdigit( full_text->str[n] );
    }

  }
  g_string_free( full_text, TRUE );

  /* Fill buffer to insert */
  if( is_valid ) {
    max_length = ( is_hex )? 6 : 5;
    result = g_new( gchar, length );

    for( i = 0; i < length && i + prev_length < max_length; i++ ) {
      result[ count++ ] = text[i];
    }
  }

  /* Insert only validated text */
  if( count > 0 ) {
    g_signal_handlers_block_by_func( G_OBJECT( editable ),
                                     G_CALLBACK( entry_validate_address ),
                                     data );
    gtk_editable_insert_text( editable, result, count, position );
    g_signal_handlers_unblock_by_func( G_OBJECT( editable ),
                                       G_CALLBACK( entry_validate_address ),
                                       data );
  }

  g_signal_stop_emission_by_name( G_OBJECT( editable ), "insert_text" );
  g_free( result );
}

void
row_toggled_callback( GtkCellRendererToggle *cell GCC_UNUSED,
                      gchar *path_string, gpointer user_data )
{
  GtkTreeIter iter;
  GtkTreeModel *model = user_data;
  GValue val, gtrainer;
  trainer_t *trainer;
  gboolean flag;

  /* Toggle current selection */
  gtk_tree_model_get_iter_from_string( model, &iter, path_string );
  memset( &val, 0, sizeof( val ) );
  gtk_tree_model_get_value( model, &iter, COL_CHECK, &val );
  flag = !g_value_get_boolean( &val );
  g_value_unset( &val );
  gtk_list_store_set( GTK_LIST_STORE( model ), &iter, COL_CHECK, flag, -1 );

  /* Request user for custom value */
  if( flag ) {
    memset( &gtrainer, 0, sizeof( gtrainer ) );
    gtk_tree_model_get_value( model, &iter, COL_TRAINER, &gtrainer );
    trainer = (trainer_t *)g_value_get_pointer( &gtrainer );
    g_value_unset( &gtrainer );

    if( trainer->ask_value ) {
      g_idle_add( (GSourceFunc)custom_value_edit, GTK_TREE_VIEW( poke_list ) );
    }
  }
}

static gboolean
custom_value_edit( GtkTreeView *tree )
{
  GtkTreePath *path;
  GtkTreeViewColumn *col, *col_current;

  gtk_tree_view_get_cursor( tree, &path, &col_current );
  if( !path || !col_current ) return FALSE;

  col = gtk_tree_view_get_column( tree, COL_VALUE );
  gtk_tree_view_set_cursor( tree, path, col, TRUE );
  gtk_tree_path_free( path );

  return FALSE;
}

void
custom_value_changed( GtkCellRendererText *cell GCC_UNUSED, gchar *path_string,
                      gchar *new_text, gpointer user_data )
{
  GtkTreeModel *model = (GtkTreeModel *)user_data;
  GtkTreeIter iter;
  GValue val;
  gchar *str_value;
  trainer_t *trainer;
  long v;
  char *endptr;

  if( gtk_tree_model_get_iter_from_string( model, &iter, path_string ) ) {
    memset( &val, 0, sizeof( val ) );

    gtk_tree_model_get_value( model, &iter, COL_TRAINER, &val );
    trainer = (trainer_t *)g_value_get_pointer( &val );
    g_value_unset( &val );

    errno = 0;
    v = strtol( new_text, &endptr, 10 );

    if( errno || v < 0 || v > 255 || endptr == new_text ) {
      ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 255" );
      return;
    }

    trainer->value = v;
    str_value = g_strdup_printf( "%d", trainer->value );

    gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                        COL_VALUE, str_value, -1);
    g_free( str_value );
  }
}
