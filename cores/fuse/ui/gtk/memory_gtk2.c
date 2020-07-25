/* memory.c: the GTK+ memory browser
   Copyright (c) 2004-2005 Philip Kendall
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
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "memory_pages.h"
#include "menu.h"

static libspectrum_word memaddr = 0x0000;

/* List columns */
enum
{
  COL_ADDRESS = 0,
  COL_HEX,
  COL_DATA,
  NUM_COLS
};

static void
update_display( GtkTreeModel *model, libspectrum_word base )
{
  size_t i, j;
  GtkTreeIter iter;

  gchar buffer[ 8 + 64 + 20 ];
  gchar *text[] = { &buffer[0], &buffer[ 8 ], &buffer[ 8 + 64 ] };
  char buffer2[ 4 ];

  memaddr = base;
  gtk_list_store_clear( GTK_LIST_STORE( model ) );

  for( i = 0; i < 20; i++ ) {
    snprintf( text[0], 8, "%04X", base );

    text[1][0] = '\0';
    for( j = 0; j < 0x10; j++, base++ ) {

      libspectrum_byte b = readbyte_internal( base );

      snprintf( buffer2, 4, "%02X ", b );
      strncat( text[1], buffer2, 4 );

      text[2][j] = ( b >= 32 && b < 127 ) ? b : '.';
    }
    text[2][ 0x10 ] = '\0';

    /* Append a new row and fill data */
    gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
    gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
                        COL_ADDRESS, text[0],
                        COL_HEX,     text[1],
                        COL_DATA,    text[2],
                        -1 );
  }

}

static void
scroller( GtkAdjustment *adjustment, gpointer user_data )
{
  libspectrum_word base;
  GtkTreeModel *model = user_data;

  /* Drop the low bits before displaying anything */
  base = gtk_adjustment_get_value( adjustment );
  base &= 0xfff0;

  update_display( model, base );
}

static GtkWidget *
create_mem_list( void )
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
                                               "Address",
                                               renderer,
                                               "text", COL_ADDRESS,
                                               NULL );

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Hex",
                                               renderer,
                                               "text", COL_HEX,
                                               NULL );

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Data",
                                               renderer,
                                               "text", COL_DATA,
                                               NULL );

  /* Create data model */
  store = gtk_list_store_new( NUM_COLS, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_STRING );

  model = GTK_TREE_MODEL( store );
  gtk_tree_view_set_model( GTK_TREE_VIEW( view ), model );
  g_object_unref( model );

  return view;
}

void
menu_machine_memorybrowser( GtkAction *gtk_action GCC_UNUSED,
                            gpointer data GCC_UNUSED )
{
  GtkWidget *dialog, *box, *content_area, *list, *scrollbar;
  GtkAdjustment *adjustment;
  GtkTreeModel *model;
  int error;
  PangoFontDescription *font;

  fuse_emulation_pause();

  error = gtkui_get_monospaced_font( &font ); if( error ) return;

  dialog = gtkstock_dialog_new( "Fuse - Memory Browser", NULL );

  gtkstock_create_close( dialog, NULL, NULL, FALSE );

  box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), box, TRUE, TRUE, 0 );

  /* The list itself */
  list = create_mem_list();
  gtk_widget_override_font( list, font );
  model = gtk_tree_view_get_model( GTK_TREE_VIEW( list ) );
  update_display( GTK_TREE_MODEL( model ), memaddr );
  gtk_box_pack_start( GTK_BOX( box ), list, TRUE, TRUE, 0 );

  adjustment = GTK_ADJUSTMENT(
    gtk_adjustment_new( memaddr, 0x0000, 0xffff, 0x10, 0xa0, 0x13f ) );
  g_signal_connect( adjustment, "value-changed", G_CALLBACK( scroller ),
                    model );

  gtkui_scroll_connect( GTK_TREE_VIEW( list ), adjustment );

  scrollbar = gtk_scrollbar_new( GTK_ORIENTATION_VERTICAL, adjustment );
  gtk_box_pack_start( GTK_BOX( box ), scrollbar, FALSE, FALSE, 0 );

  gtk_widget_show_all( dialog );
  gtk_main();

  gtkui_free_font( font );
  fuse_emulation_unpause();

  return;
}
