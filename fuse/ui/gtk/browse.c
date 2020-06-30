/* browse.c: tape browser dialog box
   Copyright (c) 2002-2004 Philip Kendall
   Copyright (c) 2015 Sergio Baldoví
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

#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "menu.h"
#include "tape.h"
#include "ui/ui.h"

static int create_dialog( void );
static void add_block_details( libspectrum_tape_block *block,
			       void *user_data );
static void select_row( GtkTreeView *treeview, GtkTreePath *path,
                        GtkTreeViewColumn *col, gpointer user_data );
void mark_row( GtkTreeModel *model, int row );
static void browse_done( GtkWidget *widget, gpointer data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );

static GdkPixbuf *tape_marker_pixbuf;

static GtkWidget
  *dialog,			/* The dialog box itself */
  *blocks,			/* The list of blocks */
  *modified_label;		/* The label saying if the tape has been
				   modified */

static int dialog_created;	/* Have we created the dialog box yet? */

/* List columns */
enum
{
  COL_PIX = 0,       /* Pixmap */
  COL_BLOCK,         /* Block type */
  COL_DATA,          /* Data detail */
  NUM_COLS
};

void
menu_media_tape_browse( GtkAction *gtk_action GCC_UNUSED,
                        gpointer data GCC_UNUSED )
{
  /* Firstly, stop emulation */
  fuse_emulation_pause();

  if( !dialog_created )
    if( create_dialog() ) { fuse_emulation_unpause(); return; }

  if( ui_tape_browser_update( UI_TAPE_BROWSER_NEW_TAPE, NULL ) ) {
    fuse_emulation_unpause();
    return;
  }

  gtk_widget_show_all( dialog );

  /* Carry on with emulation */
  fuse_emulation_unpause();
}

static GtkWidget *
create_block_list( void )
{
  GtkWidget *view;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;
  GtkListStore *store;

  view = gtk_tree_view_new();

  /* Add columns */
  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               NULL,
                                               renderer,
                                               "pixbuf", COL_PIX,
                                               NULL );

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Block type",
                                               renderer,
                                               "text", COL_BLOCK,
                                               NULL );

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Data",
                                               renderer,
                                               "text", COL_DATA,
                                               NULL );

  /* Create data model */
  store = gtk_list_store_new( NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                              G_TYPE_STRING );

  model = GTK_TREE_MODEL( store );
  gtk_tree_view_set_model( GTK_TREE_VIEW( view ), model );
  g_object_unref( model );

  /* Fast move tape */
  g_signal_connect( G_OBJECT( view ), "row-activated",
                    G_CALLBACK( select_row ), model );

  return view;
}

static int
create_dialog( void )
{
  GtkWidget *scrolled_window, *content_area;

  /* Give me a new dialog box */
  dialog = gtkstock_dialog_new( "Fuse - Browse Tape",
				G_CALLBACK( delete_dialog ) );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );

  /* And a scrolled window to pack the list into */
  scrolled_window = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
  gtk_box_pack_start( GTK_BOX( content_area ), scrolled_window, TRUE, TRUE, 0 );

  /* The tape marker pixbuf */
  tape_marker_pixbuf = gdk_pixbuf_new_from_xpm_data( gtkpixmap_tape_marker );
  /* FIXME: unref this at exit */

  /* And the list itself */
  blocks = create_block_list();
  gtk_container_add( GTK_CONTAINER( scrolled_window ), GTK_WIDGET( blocks ) );

  /* And the "tape modified" label */
  modified_label = gtk_label_new( "" );
  gtk_box_pack_start( GTK_BOX( content_area ), modified_label, FALSE, FALSE, 0 );

  /* Create the OK button */
  gtkstock_create_close( dialog, NULL, G_CALLBACK( browse_done ), FALSE );

  /* Make the window big enough to show at least some data */
  gtk_window_set_default_size( GTK_WINDOW( dialog ), -1, 250 );

  dialog_created = 1;

  return 0;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change GCC_UNUSED,
                        libspectrum_tape_block *block GCC_UNUSED )
{
  int error, current_block;
  GtkTreeModel *model;    

  if( !dialog_created ) return 0;

  fuse_emulation_pause();

  model = gtk_tree_view_get_model( GTK_TREE_VIEW( blocks ) );
  gtk_list_store_clear( GTK_LIST_STORE( model ) );

  error = tape_foreach( add_block_details, model );
  if( error ) {
    fuse_emulation_unpause();
    return 1;
  }

  current_block = tape_get_current_block();

  if( current_block != -1 )  
    mark_row( model, current_block );

  if( tape_modified ) {
    gtk_label_set_text( GTK_LABEL( modified_label ), "Tape modified" );
  } else {
    gtk_label_set_text( GTK_LABEL( modified_label ), "Tape not modified" );
  }

  fuse_emulation_unpause();

  return 0;
}

static void
add_block_details( libspectrum_tape_block *block, void *user_data )
{
  gchar block_type[80];
  gchar data_detail[80];
  GtkTreeIter iter;
  GtkTreeModel *model = user_data;

  libspectrum_tape_block_description( block_type, 80, block );
  tape_block_details( data_detail, 80, block );

  /* Append a new row and fill data */
  gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
  gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                      COL_PIX, NULL,
                      COL_BLOCK, block_type,
                      COL_DATA, data_detail,
                      -1 );
}

/* Called when a row is selected */
static void
select_row( GtkTreeView *treeview GCC_UNUSED, GtkTreePath *path,
            GtkTreeViewColumn *col GCC_UNUSED, gpointer user_data )
{
  int current_block;
  int *indices;
  int row;
  GtkTreeIter iter;
  GtkTreeModel *model = user_data;

  /* Get selected row */
  row = -1;
  indices = gtk_tree_path_get_indices( path );
  if( indices ) row = indices[0];

  /* Don't do anything if the current block was clicked on */
  current_block = tape_get_current_block();
  if( row == current_block ) return;

  /* Otherwise, select the new block */
  tape_select_block_no_update( row );

  /* Mark selected block */
  if( current_block != -1 ) {
    gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &iter, path );

    gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                        COL_PIX, tape_marker_pixbuf,
                        -1 );
  }

  /* Unmark former block */
  if( current_block != -1 ) {
    path = gtk_tree_path_new_from_indices( current_block, -1 );

    if( !gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &iter, path ) ) {
      gtk_tree_path_free( path );
      return;
    }

    gtk_tree_path_free( path );

    gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                        COL_PIX, NULL,
                        -1 );
  }
}

void
mark_row( GtkTreeModel *model, int row )
{    
  GtkTreeIter iter;
  GtkTreePath *path;

  path = gtk_tree_path_new_from_indices( row, -1 );

  if( !gtk_tree_model_get_iter( model, &iter, path ) ) {
    gtk_tree_path_free( path );
    return;
  }

  gtk_tree_path_free( path );

  gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                      COL_PIX, tape_marker_pixbuf,
                      -1 );
}

/* Called if the OK button is clicked */
static void
browse_done( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  dialog_created = 0;
  gtk_widget_destroy( dialog );
}

/* Catch attempts to delete the window and just hide it instead */
static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data GCC_UNUSED )
{
  dialog_created = 0;
  gtk_widget_destroy( dialog );
  return TRUE;
}
