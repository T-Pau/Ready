/* rollback.c: select a rollback point
   Copyright (c) 2004 Philip Kendall

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

#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkinternals.h"
#include "ui/ui.h"

GtkWidget *dialog, *list;

static int dialog_created = 0;
static int current_block;

/* List columns */
enum
{
  COL_SECONDS = 0,
  NUM_COLS
};

static void
select_row( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  GtkTreePath *path;
  GtkTreeView *view = user_data;
  GtkTreeViewColumn *focus_column;

  current_block = -1;

  /* Get selected row */
  gtk_tree_view_get_cursor( GTK_TREE_VIEW( view ), &path, &focus_column );
  if( path ) {
    int *indices = gtk_tree_path_get_indices( path );
    if( indices ) current_block = indices[0];
    gtk_tree_path_free( path );
  }
}

static GtkWidget *
create_rollback_list( void )
{
  GtkWidget *view;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;
  GtkListStore *store;

  view = gtk_tree_view_new();

  /* Add columns */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Seconds",
                                               renderer,
                                               "text", COL_SECONDS,
                                               NULL );

  /* Create data model */
  store = gtk_list_store_new( NUM_COLS, G_TYPE_STRING );

  model = GTK_TREE_MODEL( store );
  gtk_tree_view_set_model( GTK_TREE_VIEW( view ), model );
  g_object_unref( model );

  return view;
}

static int
create_dialog( void )
{
  GtkWidget *content_area;

  dialog = gtkstock_dialog_new( "Fuse - Select Rollback Point", NULL );

  list = create_rollback_list();

  gtkstock_create_ok_cancel( dialog, NULL, G_CALLBACK( select_row ), list,
                             DEFAULT_DESTROY, DEFAULT_DESTROY );

  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), list, TRUE, TRUE, 0 );

  return 0;
}

static int
update_list( GSList *points )
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model( GTK_TREE_VIEW( list ) );

  gtk_list_store_clear( GTK_LIST_STORE( model ) );

  while( points ) {
    gchar buffer[256];

    snprintf( buffer, 256, "%.2f", GPOINTER_TO_INT( points->data ) / 50.0 );

    /* Append a new row and fill data */
    gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                        COL_SECONDS, buffer,
                        -1 );

    points = points->next;
  }

  return 0;
}

int
ui_get_rollback_point( GSList *points )
{
  fuse_emulation_pause();

  if( !dialog_created )
    if( create_dialog() ) { fuse_emulation_unpause(); return -1; }

  if( update_list( points ) ) { fuse_emulation_unpause(); return -1; }

  current_block = -1;

  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();

  return current_block;
}
