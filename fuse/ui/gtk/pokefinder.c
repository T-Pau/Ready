/* pokefinder.c: GTK+ interface to the poke finder
   Copyright (c) 2003-2015 Philip Kendall
   Copyright (c) 2014 Sergio Baldoví
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "menu.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"

static int create_dialog( void );
static void gtkui_pokefinder_incremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_decremented( GtkWidget *widget,
					  gpointer user_data GCC_UNUSED );
static void gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_reset( GtkWidget *widget, gpointer user_data );
static void gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data );
static gboolean delete_dialog( GtkWidget *widget, GdkEvent *event,
			       gpointer user_data );
static void update_pokefinder( void );
static void possible_click( GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *col, gpointer user_data );

static int dialog_created = 0;

static GtkWidget
  *dialog,			/* The dialog box itself */
  *count_label,			/* The number of possible locations */
  *location_list;		/* The list view of possible locations */

static GtkTreeModel *location_model; /* The data of possible locations */

/* The possible locations */

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];

/* List columns */
enum
{
  COL_PAGE = 0,
  COL_OFFSET,
  NUM_COLS
};

void
menu_machine_pokefinder( GtkAction *gtk_action GCC_UNUSED,
                         gpointer data GCC_UNUSED )
{
  int error;

  if( !dialog_created ) {
    error = create_dialog(); if( error ) return;
  }

  gtk_widget_show_all( dialog );
  update_pokefinder();
}

static GtkWidget *
create_location_list( void )
{
  GtkWidget *view;
  GtkCellRenderer *renderer;
  GtkListStore *store;

  view = gtk_tree_view_new();

  /* Add columns */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Page",
                                               renderer,
                                               "text", COL_PAGE,
                                               NULL );

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
                                               -1,
                                               "Offset",
                                               renderer,
                                               "text", COL_OFFSET,
                                               NULL );

  /* Create data model */
  store = gtk_list_store_new( NUM_COLS, G_TYPE_STRING, G_TYPE_STRING );

  location_model = GTK_TREE_MODEL( store );
  gtk_tree_view_set_model( GTK_TREE_VIEW( view ), location_model );
  g_object_unref( location_model );

  /* Activate breakpoints */
  g_signal_connect( G_OBJECT( view ), "row-activated",
                    G_CALLBACK( possible_click ), NULL );

  return view;
}

static int
create_dialog( void )
{
  GtkWidget *hbox, *vbox, *label, *entry, *content_area;
  GtkAccelGroup *accel_group;

  dialog = gtkstock_dialog_new( "Fuse - Poke Finder",
				G_CALLBACK( delete_dialog ) );

  hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 0 );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), hbox, TRUE, TRUE, 0 );

  label = gtk_label_new( "Search for:" );
  gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 5 );

  entry = gtk_entry_new();
  g_signal_connect( G_OBJECT( entry ), "activate",
		    G_CALLBACK( gtkui_pokefinder_search ), NULL );
  gtk_box_pack_start( GTK_BOX( hbox ), entry, TRUE, TRUE, 5 );

  vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 5 );

  count_label = gtk_label_new( "" );
  gtk_box_pack_start( GTK_BOX( vbox ), count_label, TRUE, TRUE, 5 );

  location_list = create_location_list();
  gtk_box_pack_start( GTK_BOX( vbox ), location_list, TRUE, TRUE, 5 );

  {
    static gtkstock_button btn[] = {
      { "Incremented", G_CALLBACK( gtkui_pokefinder_incremented ), NULL, NULL, 0, 0, 0, 0, GTK_RESPONSE_NONE },
      { "Decremented", G_CALLBACK( gtkui_pokefinder_decremented ), NULL, NULL, 0, 0, 0, 0, GTK_RESPONSE_NONE },
      { "!Search", G_CALLBACK( gtkui_pokefinder_search ), NULL, NULL, GDK_KEY_Return, 0, 0, 0, GTK_RESPONSE_NONE },
      { "Reset", G_CALLBACK( gtkui_pokefinder_reset ), NULL, NULL, 0, 0, 0, 0, GTK_RESPONSE_NONE }
    };
    btn[2].actiondata = G_OBJECT( entry );
    accel_group = gtkstock_create_buttons( dialog, NULL, btn,
					   ARRAY_SIZE( btn ) );
    gtkstock_create_close( dialog, accel_group,
			   G_CALLBACK( gtkui_pokefinder_close ), TRUE );
  }

  /* Users shouldn't be able to resize this window */
  gtk_window_set_resizable( GTK_WINDOW( dialog ), FALSE );

  dialog_created = 1;

  return 0;
}

static void
gtkui_pokefinder_incremented( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_incremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_decremented( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  pokefinder_decremented();
  update_pokefinder();
}

static void
gtkui_pokefinder_search( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  long value;
  const gchar *entry;
  char *endptr;
  int base;

  errno = 0;
  entry = gtk_entry_get_text( GTK_ENTRY( widget ) );
  base = ( g_str_has_prefix( entry, "0x" ) )? 16 : 10;
  value = strtol( entry, &endptr, base );

  if( errno != 0 || value < 0 || value > 255 || endptr == entry ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 255" );
    return;
  }

  pokefinder_search( value );
  update_pokefinder();
}

static void
gtkui_pokefinder_reset( GtkWidget *widget GCC_UNUSED,
			gpointer user_data GCC_UNUSED)
{
  pokefinder_clear();
  update_pokefinder();
}

static gboolean
delete_dialog( GtkWidget *widget, GdkEvent *event GCC_UNUSED,
	       gpointer user_data )
{
  gtkui_pokefinder_close( widget, user_data );
  return TRUE;
}

static void
gtkui_pokefinder_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide( widget );
}

static gboolean
widget_delayed_show( GtkWidget *item )
{
  gtk_widget_show( item );

  return FALSE;
}

static void
update_pokefinder( void )
{
  size_t page, offset, bank, bank_offset;
  gchar buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };
  GtkTreeIter iter;

  gtk_list_store_clear( GTK_LIST_STORE( location_model ) );

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which;

    which = 0;

    for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
      memory_page *mapping = &memory_map_ram[page];
      bank = mapping->page_num;

      for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ )
	if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
	  bank_offset = mapping->offset + offset;

	  possible_page[ which ] = bank;
	  possible_offset[ which ] = bank_offset;
	  which++;
	
	  snprintf( possible_text[0], 128, "%lu", (unsigned long)bank );
	  snprintf( possible_text[1], 128, "0x%04X", (unsigned)bank_offset );

	  /* Append a new row and fill data */
	  gtk_list_store_append( GTK_LIST_STORE( location_model ), &iter );
	  gtk_list_store_set( GTK_LIST_STORE( location_model ), &iter,
	                      COL_PAGE, possible_text[0],
	                      COL_OFFSET, possible_text[1],
	                      -1 );
	}
    }

    /* Show widget when the GtkTreeView has been filled with data. Fix an empty
       list on GTK+ 3.10 (and maybe other versions) */
    g_idle_add( (GSourceFunc)widget_delayed_show, location_list );

  } else {
    gtk_widget_hide( location_list );
  }

  snprintf( buffer, 256, "Possible locations: %lu",
	    (unsigned long)pokefinder_count );
  gtk_label_set_text( GTK_LABEL( count_label ), buffer );
}  

static void
possible_click( GtkTreeView *treeview GCC_UNUSED, GtkTreePath *path,
                GtkTreeViewColumn *col GCC_UNUSED,
                gpointer user_data GCC_UNUSED )
{
  int error, *indices, row;

  /* Get selected row via double-clicks or keyboard */
  indices = gtk_tree_path_get_indices( path );
  if( !indices ) return;
  row = indices[0];

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_WRITE, memory_source_ram, possible_page[ row ],
    possible_offset[ row ], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL
  );
  if( error ) return;

  ui_debugger_update();
}

void
gtkui_pokefinder_clear( void )
{
  pokefinder_clear();
  if( dialog_created ) update_pokefinder();
}
