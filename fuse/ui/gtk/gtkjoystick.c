/* gtkjoystick.c: Joystick emulation
   Copyright (c) 2003-2015 Darren Salt, Philip Kendall

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

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <gtk/gtk.h>
#include <string.h>

#include "compat.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "keyboard.h"
#include "peripherals/joystick.h"
#include "menu.h"
#include "settings.h"

#if !defined USE_JOYSTICK || defined HAVE_JSW_H
/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#include "../sdl/sdljoystick.c"

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

enum
{
  COL_TEXT = 0,
  COL_KEY,
  NUM_COLS
};

struct button_info {
  int *setting;
  char name[80];
  GtkWidget *label;
  keyboard_key_name key;
};

struct joystick_info {

  int *type;
  GtkWidget *radio[ JOYSTICK_TYPE_COUNT ];

  struct button_info button[NUM_JOY_BUTTONS];
};

typedef enum key_item_t {
  ITEM = 0,
  GROUP,
  SUBITEM,
} key_item_t;

typedef struct key_menu_t {
  key_item_t item;
  const gchar *text;
  keyboard_key_name key;
} key_menu_t;

static void setup_info( struct joystick_info *info, int callback_action );
static void create_joystick_type_selector( struct joystick_info *info,
					   GtkBox *parent );
static void
create_fire_button_selector( const char *title, struct button_info *info,
                             GtkBox *parent, GtkTreeModel *model );
static void set_key_text( GtkWidget *label, keyboard_key_name key );

static void key_callback( GtkComboBox *widget, gpointer user_data );
static void joystick_done( GtkButton *button, gpointer user_data );

static key_menu_t key_menu[] = {

    { ITEM, "Joystick Fire", KEYBOARD_JOYSTICK_FIRE }, 

    { GROUP, "Numbers", KEYBOARD_NONE }, 
    { SUBITEM, "0", KEYBOARD_0 },
    { SUBITEM, "1", KEYBOARD_1 },
    { SUBITEM, "2", KEYBOARD_2 },
    { SUBITEM, "3", KEYBOARD_3 },
    { SUBITEM, "4", KEYBOARD_4 },
    { SUBITEM, "5", KEYBOARD_5 },
    { SUBITEM, "6", KEYBOARD_6 },
    { SUBITEM, "7", KEYBOARD_7 },
    { SUBITEM, "8", KEYBOARD_8 },
    { SUBITEM, "9", KEYBOARD_9 },

    { GROUP, "A - M", KEYBOARD_NONE },
    { SUBITEM, "A", KEYBOARD_a },
    { SUBITEM, "B", KEYBOARD_b },
    { SUBITEM, "C", KEYBOARD_c },
    { SUBITEM, "D", KEYBOARD_d },
    { SUBITEM, "E", KEYBOARD_e },
    { SUBITEM, "F", KEYBOARD_f },
    { SUBITEM, "G", KEYBOARD_g },
    { SUBITEM, "H", KEYBOARD_h },
    { SUBITEM, "I", KEYBOARD_i },
    { SUBITEM, "J", KEYBOARD_j },
    { SUBITEM, "K", KEYBOARD_k },
    { SUBITEM, "L", KEYBOARD_l },
    { SUBITEM, "M", KEYBOARD_m },

    { GROUP, "N - Z", KEYBOARD_NONE },
    { SUBITEM, "N", KEYBOARD_n },
    { SUBITEM, "O", KEYBOARD_o },
    { SUBITEM, "P", KEYBOARD_p },
    { SUBITEM, "Q", KEYBOARD_q },
    { SUBITEM, "R", KEYBOARD_r },
    { SUBITEM, "S", KEYBOARD_s },
    { SUBITEM, "T", KEYBOARD_t },
    { SUBITEM, "U", KEYBOARD_u },
    { SUBITEM, "V", KEYBOARD_v },
    { SUBITEM, "W", KEYBOARD_w },
    { SUBITEM, "X", KEYBOARD_x },
    { SUBITEM, "Y", KEYBOARD_y },
    { SUBITEM, "Z", KEYBOARD_z },

    { ITEM, "Space", KEYBOARD_space },
    { ITEM, "Enter", KEYBOARD_Enter },
    { ITEM, "Caps Shift", KEYBOARD_Caps },
    { ITEM, "Symbol Shift", KEYBOARD_Symbol },
    { ITEM, "Nothing", KEYBOARD_NONE },

};

static GtkTreeModel *
create_joystick_options_store( void )
{
  GtkTreeIter iter, iter2;
  GtkTreeStore *store;
  guint i;

  store = gtk_tree_store_new( NUM_COLS, G_TYPE_STRING, G_TYPE_INT );

  for( i = 0; i < ARRAY_SIZE( key_menu ); i++ ) {

    switch( key_menu[i].item ) {

      case ITEM:
      case GROUP:
        gtk_tree_store_append( store, &iter, NULL );
        gtk_tree_store_set( store, &iter,
                            COL_TEXT, key_menu[i].text,
                            COL_KEY, key_menu[i].key,
                            -1 );
        break;

      case SUBITEM:
        gtk_tree_store_append( store, &iter2, &iter );
        gtk_tree_store_set( store, &iter2,
                            COL_TEXT, key_menu[i].text,
                            COL_KEY, key_menu[i].key,
                            -1 );
        break;

    }

  }

  return GTK_TREE_MODEL( store );
}

void
menu_options_joysticks_select( GtkAction *gtk_action GCC_UNUSED,
                               guint callback_action )
{
  GtkWidget *dialog, *hbox, *vbox, *content_area;
  GtkTreeModel *model;
  struct joystick_info info;
  int i;

  fuse_emulation_pause();

  setup_info( &info, callback_action );

  dialog = gtkstock_dialog_new( "Fuse - Configure Joystick", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );

  hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( hbox ), 4 );
  gtk_box_pack_start( GTK_BOX( content_area ), hbox, FALSE, FALSE, 0 );

  create_joystick_type_selector( &info, GTK_BOX( hbox ) );

  vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 2 );
  gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 0 );

  model = create_joystick_options_store();

  for( i = 0; i < NUM_JOY_BUTTONS; i += 5 ) {
    
    int j;

    vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 2 );
    gtk_box_pack_start( GTK_BOX( hbox ), vbox, TRUE, TRUE, 0 );
    
    for( j = i; j < i + 5; j++ )
      if( info.button[j].setting ) {
        create_fire_button_selector( info.button[j].name, &( info.button[j] ),
                                     GTK_BOX( vbox ), model );
      }
  }

  g_object_unref( model );

  gtkstock_create_ok_cancel( dialog, NULL, G_CALLBACK( joystick_done ),
                             &info, DEFAULT_DESTROY, DEFAULT_DESTROY );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_OK );

  gtk_widget_show_all( dialog );
  gtk_main();

  fuse_emulation_unpause();
}

static void
setup_info( struct joystick_info *info, int callback_action )
{
  size_t i;

  switch( callback_action ) {

  case 1:
    info->type = &( settings_current.joystick_1_output );
    info->button[0].setting = &( settings_current.joystick_1_fire_1  );
    info->button[1].setting = &( settings_current.joystick_1_fire_2  );
    info->button[2].setting = &( settings_current.joystick_1_fire_3  );
    info->button[3].setting = &( settings_current.joystick_1_fire_4  );
    info->button[4].setting = &( settings_current.joystick_1_fire_5  );
    info->button[5].setting = &( settings_current.joystick_1_fire_6  );
    info->button[6].setting = &( settings_current.joystick_1_fire_7  );
    info->button[7].setting = &( settings_current.joystick_1_fire_8  );
    info->button[8].setting = &( settings_current.joystick_1_fire_9  );
    info->button[9].setting = &( settings_current.joystick_1_fire_10 );
    info->button[10].setting = &( settings_current.joystick_1_fire_11 );
    info->button[11].setting = &( settings_current.joystick_1_fire_12 );
    info->button[12].setting = &( settings_current.joystick_1_fire_13 );
    info->button[13].setting = &( settings_current.joystick_1_fire_14 );
    info->button[14].setting = &( settings_current.joystick_1_fire_15 );
    for( i = 0; i < NUM_JOY_BUTTONS; i++ )
      snprintf( info->button[i].name, 80, "Button %lu", (unsigned long)i + 1 );
    break;

  case 2:
    info->type = &( settings_current.joystick_2_output );
    info->button[0].setting = &( settings_current.joystick_2_fire_1  );
    info->button[1].setting = &( settings_current.joystick_2_fire_2  );
    info->button[2].setting = &( settings_current.joystick_2_fire_3  );
    info->button[3].setting = &( settings_current.joystick_2_fire_4  );
    info->button[4].setting = &( settings_current.joystick_2_fire_5  );
    info->button[5].setting = &( settings_current.joystick_2_fire_6  );
    info->button[6].setting = &( settings_current.joystick_2_fire_7  );
    info->button[7].setting = &( settings_current.joystick_2_fire_8  );
    info->button[8].setting = &( settings_current.joystick_2_fire_9  );
    info->button[9].setting = &( settings_current.joystick_2_fire_10 );
    info->button[10].setting = &( settings_current.joystick_2_fire_10 );
    info->button[11].setting = &( settings_current.joystick_2_fire_11 );
    info->button[12].setting = &( settings_current.joystick_2_fire_12 );
    info->button[13].setting = &( settings_current.joystick_2_fire_13 );
    info->button[14].setting = &( settings_current.joystick_2_fire_14 );
    for( i = 0; i < NUM_JOY_BUTTONS; i++ )
      snprintf( info->button[i].name, 80, "Button %lu", (unsigned long)i + 1 );
    break;

  case 3:
    info->type = &( settings_current.joystick_keyboard_output );
    info->button[0].setting = &( settings_current.joystick_keyboard_up  );
    snprintf( info->button[0].name, 80, "Button for UP" );
    info->button[1].setting = &( settings_current.joystick_keyboard_down  );
    snprintf( info->button[1].name, 80, "Button for DOWN" );
    info->button[2].setting = &( settings_current.joystick_keyboard_left  );
    snprintf( info->button[2].name, 80, "Button for LEFT" );
    info->button[3].setting = &( settings_current.joystick_keyboard_right  );
    snprintf( info->button[3].name, 80, "Button for RIGHT" );
    info->button[4].setting = &( settings_current.joystick_keyboard_fire  );
    snprintf( info->button[4].name, 80, "Button for FIRE" );
    for( i = 5; i < NUM_JOY_BUTTONS; i++ ) info->button[i].setting = NULL;
    break;

  }
}

static void
create_joystick_type_selector( struct joystick_info *info, GtkBox *parent )
{
  GtkWidget *frame, *box;
  GSList *button_group;
  int i;

  frame = gtk_frame_new( "Joystick type" );
  gtk_box_pack_start( parent, frame, FALSE, FALSE, 0 );

  box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_container_add( GTK_CONTAINER( frame ), box );

  button_group = NULL;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    info->radio[ i ] =
      gtk_radio_button_new_with_label( button_group, joystick_name[ i ] );
    button_group =
      gtk_radio_button_get_group( GTK_RADIO_BUTTON( info->radio[ i ] ) );
    gtk_box_pack_start( GTK_BOX( box ), info->radio[ i ], FALSE, FALSE, 0 );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( info->radio[ i ] ),
				  i == *( info->type ) );

  }

}

static void
set_entry_properties( GtkCellLayout *cell_layout GCC_UNUSED,
                      GtkCellRenderer *cell, GtkTreeModel *tree_model,
                      GtkTreeIter *iter, gpointer data  GCC_UNUSED )
{
  gboolean sensitive;

  sensitive = !gtk_tree_model_iter_has_child( tree_model, iter );

  g_object_set( cell, "sensitive", sensitive, NULL );

  g_object_set( cell, "xpad", 10, NULL );
}

static void
create_fire_button_selector( const char *title, struct button_info *info,
                             GtkBox *parent, GtkTreeModel *model )
{
  GtkWidget *frame, *box, *combo;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  GtkTreePath *path;
  size_t i;

  frame = gtk_frame_new( title );
  gtk_box_pack_start( parent, frame, TRUE, TRUE, 0 );

  box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 4 );
  gtk_container_set_border_width( GTK_CONTAINER( box ), 2 );
  gtk_container_add( GTK_CONTAINER( frame ), box );

  /* Create label */
  info->key = *info->setting;
  info->label = gtk_label_new( "" );

  for( i = 0; i < ARRAY_SIZE( key_menu ); i++ ) {
    
    keyboard_key_name key;

    key = key_menu[i].key;

    if( key_menu[i].item != GROUP && key == (unsigned int)*info->setting ) {
      set_key_text( info->label, key );
      break;
    }

  }

  gtk_box_pack_start( GTK_BOX( box ), info->label, TRUE, TRUE, 0 );

  /* Create combobox */
  combo = gtk_combo_box_new_with_model( model );    
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), renderer, TRUE );
  gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), renderer,
                                  "text", 0, NULL );
  gtk_cell_layout_set_cell_data_func( GTK_CELL_LAYOUT( combo ), renderer,
                                      set_entry_properties, NULL, NULL );

  /* Select first item */
  path = gtk_tree_path_new_from_indices( 0, -1 );
  gtk_tree_model_get_iter( model, &iter, path );
  gtk_tree_path_free( path );
  gtk_combo_box_set_active_iter( GTK_COMBO_BOX( combo ), &iter );

  gtk_box_pack_start( GTK_BOX( box ), combo, TRUE, TRUE, 0 );

  g_signal_connect( G_OBJECT( combo ), "changed", G_CALLBACK( key_callback ),
                    info );
}

static void
set_key_text( GtkWidget *label, keyboard_key_name key )
{
  const char *text;
  char buffer[40];

  text = keyboard_key_text( key );

  snprintf( buffer, 40, "%s", text );

  gtk_label_set_text( GTK_LABEL( label ), buffer );
}

static void
key_callback( GtkComboBox *widget, gpointer user_data )
{
  GtkTreeIter iter;
  GValue value;
  struct button_info *info = user_data;
  keyboard_key_name key;
  GtkTreeModel *model;

  /* Get current selection */
  gtk_combo_box_get_active_iter( GTK_COMBO_BOX( widget ), &iter );
  memset( &value, 0, sizeof( value ) );
  model = gtk_combo_box_get_model( GTK_COMBO_BOX( widget ) );
  gtk_tree_model_get_value( model, &iter, COL_KEY, &value );
  key = g_value_get_int( &value );
  g_value_unset( &value );

  /* Store and display selection */
  info->key = key;
  set_key_text( info->label, info->key );
}

static void
joystick_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  struct joystick_info *info = user_data;

  int i;
  GtkToggleButton *toggle;

  for( i = 0; i < NUM_JOY_BUTTONS; i++ )
    if( info->button[i].setting )
      *info->button[i].setting = info->button[i].key;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    toggle = GTK_TOGGLE_BUTTON( info->radio[ i ] );

    if( gtk_toggle_button_get_active( toggle ) ) {
      *( info->type ) = i;
      return;
    }

  }

}
