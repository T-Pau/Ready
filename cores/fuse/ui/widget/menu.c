/* menu.c: general menu widget
   Copyright (c) 2001-2015 Philip Kendall
   Copyright (c) 2015 Sergio Baldoví

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
#include <string.h>

#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "machines/specplus3.h"
#include "menu.h"
#include "peripherals/dck.h"
#include "peripherals/disk/beta.h"
#include "peripherals/joystick.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "ui/uidisplay.h"
#include "utils.h"
#include "widget_internals.h"
#include "widget.h"

widget_menu_entry *menu;
static size_t highlight_line = 0;
static size_t count;
static int *current_settings[ 16 ];

#define GET_SET_KEY_FUNCTIONS( which ) \
\
static void \
set_key_for_button_ ## which ( int action ) \
{ \
  *current_settings[ which ] = action; \
  widget_end_all( WIDGET_FINISHED_OK ); \
} \
\
static const char* \
get_key_name_for_button_ ## which ( void ) \
{ \
  return keyboard_key_text( *current_settings[ which ] ); \
}

GET_SET_KEY_FUNCTIONS( 1 )
GET_SET_KEY_FUNCTIONS( 2 )
GET_SET_KEY_FUNCTIONS( 3 )
GET_SET_KEY_FUNCTIONS( 4 )
GET_SET_KEY_FUNCTIONS( 5 )
#ifdef USE_JOYSTICK
GET_SET_KEY_FUNCTIONS( 6 )
GET_SET_KEY_FUNCTIONS( 7 )
GET_SET_KEY_FUNCTIONS( 8 )
#ifndef GEKKO
GET_SET_KEY_FUNCTIONS( 9 )
GET_SET_KEY_FUNCTIONS( 10 )
GET_SET_KEY_FUNCTIONS( 11 )
GET_SET_KEY_FUNCTIONS( 12 )
GET_SET_KEY_FUNCTIONS( 13 )
GET_SET_KEY_FUNCTIONS( 14 )
GET_SET_KEY_FUNCTIONS( 15 )
#endif  /* #ifndef GEKKO */
#endif  /* #ifdef USE_JOYSTICK */

#define SUBMENU_KEY_SELECTIONS( which ) \
\
static widget_menu_entry submenu_select_number_for_button_ ## which [] = { \
  { "Select a key" }, \
  { "\0120\011", INPUT_KEY_0, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_0 }, \
  { "\0121\011", INPUT_KEY_1, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_1 }, \
  { "\0122\011", INPUT_KEY_2, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_2 }, \
  { "\0123\011", INPUT_KEY_3, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_3 }, \
  { "\0124\011", INPUT_KEY_4, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_4 }, \
  { "\0125\011", INPUT_KEY_5, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_5 }, \
  { "\0126\011", INPUT_KEY_6, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_6 }, \
  { "\0127\011", INPUT_KEY_7, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_7 }, \
  { "\0128\011", INPUT_KEY_8, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_8 }, \
  { "\0129\011", INPUT_KEY_9, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_9 }, \
  { NULL } \
}; \
\
static widget_menu_entry submenu_select_letters1_for_button_ ## which [] = { \
  { "Select a key" }, \
  { "\012A\011", INPUT_KEY_a, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_a }, \
  { "\012B\011", INPUT_KEY_b, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_b }, \
  { "\012C\011", INPUT_KEY_c, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_c }, \
  { "\012D\011", INPUT_KEY_d, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_d }, \
  { "\012E\011", INPUT_KEY_e, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_e }, \
  { "\012F\011", INPUT_KEY_f, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_f }, \
  { "\012G\011", INPUT_KEY_g, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_g }, \
  { "\012H\011", INPUT_KEY_h, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_h }, \
  { "\012I\011", INPUT_KEY_i, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_i }, \
  { "\012J\011", INPUT_KEY_j, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_j }, \
  { "\012K\011", INPUT_KEY_k, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_k }, \
  { "\012L\011", INPUT_KEY_l, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_l }, \
  { "\012M\011", INPUT_KEY_m, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_m }, \
  { NULL } \
}; \
\
static widget_menu_entry submenu_select_letters2_for_button_ ## which [] = { \
  { "Select a key" }, \
  { "\012N\011", INPUT_KEY_n, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_n }, \
  { "\012O\011", INPUT_KEY_o, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_o }, \
  { "\012P\011", INPUT_KEY_p, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_p }, \
  { "\012Q\011", INPUT_KEY_q, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_q }, \
  { "\012R\011", INPUT_KEY_r, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_r }, \
  { "\012S\011", INPUT_KEY_s, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_s }, \
  { "\012T\011", INPUT_KEY_t, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_t }, \
  { "\012U\011", INPUT_KEY_u, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_u }, \
  { "\012V\011", INPUT_KEY_v, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_v }, \
  { "\012W\011", INPUT_KEY_w, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_w }, \
  { "\012X\011", INPUT_KEY_x, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_x }, \
  { "\012Y\011", INPUT_KEY_y, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_y }, \
  { "\012Z\011", INPUT_KEY_z, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_z }, \
  { NULL } \
}; \
\
static widget_menu_entry submenu_select_key_for_button_ ## which [] = { \
  { "Select a key" }, \
  { "\012J\011oystick fire", INPUT_KEY_j, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_JOYSTICK_FIRE }, \
  { "\012N\011umbers...", INPUT_KEY_n, submenu_select_number_for_button_ ## which , NULL, NULL, 0 }, \
  { "\012A\011-M...", INPUT_KEY_a, submenu_select_letters1_for_button_ ## which , NULL, NULL, 0 }, \
  { "N-\012Z\011...", INPUT_KEY_z, submenu_select_letters2_for_button_ ## which , NULL, NULL, 0 }, \
  { "\012S\011pace", INPUT_KEY_s, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_space }, \
  { "\012E\011nter", INPUT_KEY_e, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_Enter }, \
  { "\012C\011aps Shift", INPUT_KEY_c, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_Caps }, \
  { "S\012y\011mbol Shift", INPUT_KEY_y, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_Symbol }, \
  { "N\012o\011thing", INPUT_KEY_o, NULL, set_key_for_button_ ## which, NULL, KEYBOARD_NONE }, \
  { NULL } \
};

SUBMENU_KEY_SELECTIONS( 1 )
SUBMENU_KEY_SELECTIONS( 2 )
SUBMENU_KEY_SELECTIONS( 3 )
SUBMENU_KEY_SELECTIONS( 4 )
SUBMENU_KEY_SELECTIONS( 5 )
#ifdef USE_JOYSTICK
SUBMENU_KEY_SELECTIONS( 6 )
SUBMENU_KEY_SELECTIONS( 7 )
SUBMENU_KEY_SELECTIONS( 8 )
#ifndef GEKKO
SUBMENU_KEY_SELECTIONS( 9 )
SUBMENU_KEY_SELECTIONS( 10 )
SUBMENU_KEY_SELECTIONS( 11 )
SUBMENU_KEY_SELECTIONS( 12 )
SUBMENU_KEY_SELECTIONS( 13 )
SUBMENU_KEY_SELECTIONS( 14 )
SUBMENU_KEY_SELECTIONS( 15 )
#endif  /* #ifndef GEKKO */
#endif  /* #ifdef USE_JOYSTICK */

#ifdef USE_JOYSTICK
static widget_menu_entry submenu_joystick_buttons[] = {
  { "Select joystick button" },
#ifndef GEKKO
  { "Button \0121\011", INPUT_KEY_1, submenu_select_key_for_button_1, NULL, get_key_name_for_button_1, 0 },
  { "Button \0122\011", INPUT_KEY_2, submenu_select_key_for_button_2, NULL, get_key_name_for_button_2, 0 },
  { "Button \0123\011", INPUT_KEY_3, submenu_select_key_for_button_3, NULL, get_key_name_for_button_3, 0 },
  { "Button \0124\011", INPUT_KEY_4, submenu_select_key_for_button_4, NULL, get_key_name_for_button_4, 0 },
  { "Button \0125\011", INPUT_KEY_5, submenu_select_key_for_button_5, NULL, get_key_name_for_button_5, 0 },
  { "Button \0126\011", INPUT_KEY_6, submenu_select_key_for_button_6, NULL, get_key_name_for_button_6, 0 },
  { "Button \0127\011", INPUT_KEY_7, submenu_select_key_for_button_7, NULL, get_key_name_for_button_7, 0 },
  { "Button \0128\011", INPUT_KEY_8, submenu_select_key_for_button_8, NULL, get_key_name_for_button_8, 0 },
  { "Button \0129\011", INPUT_KEY_9, submenu_select_key_for_button_9, NULL, get_key_name_for_button_9, 0 },
  { "Button 1\0120\011", INPUT_KEY_0, submenu_select_key_for_button_10, NULL, get_key_name_for_button_10, 0 },
  { "Button 11(\012a\011)", INPUT_KEY_a, submenu_select_key_for_button_11, NULL, get_key_name_for_button_11, 0 },
  { "Button 12(\012b\011)", INPUT_KEY_b, submenu_select_key_for_button_12, NULL, get_key_name_for_button_12, 0 },
  { "Button 13(\012c\011)", INPUT_KEY_c, submenu_select_key_for_button_13, NULL, get_key_name_for_button_13, 0 },
  { "Button 14(\012d\011)", INPUT_KEY_d, submenu_select_key_for_button_14, NULL, get_key_name_for_button_14, 0 },
  { "Button 15(\012e\011)", INPUT_KEY_e, submenu_select_key_for_button_15, NULL, get_key_name_for_button_15, 0 },
#else  /* #ifndef GEKKO */
  { "Button \0121\011", INPUT_KEY_1, submenu_select_key_for_button_1, NULL, get_key_name_for_button_1, 0 },
  { "Button \0122\011", INPUT_KEY_2, submenu_select_key_for_button_2, NULL, get_key_name_for_button_2, 0 },
  { "Button \012A\011", INPUT_KEY_a, submenu_select_key_for_button_3, NULL, get_key_name_for_button_3, 0 },
  { "Button \012B\011", INPUT_KEY_b, submenu_select_key_for_button_4, NULL, get_key_name_for_button_4, 0 },
  { "Button \012P\011lus", INPUT_KEY_p, submenu_select_key_for_button_5, NULL, get_key_name_for_button_5, 0 },
  { "Button \012M\011inus", INPUT_KEY_m, submenu_select_key_for_button_6, NULL, get_key_name_for_button_6, 0 },
  { "Button \012Z\011 on Nunchuck", INPUT_KEY_z, submenu_select_key_for_button_7, NULL, get_key_name_for_button_7, 0 },
  { "Button \012C\011 on Nunchuck", INPUT_KEY_c, submenu_select_key_for_button_8, NULL, get_key_name_for_button_8, 0 },
#endif  /* #ifndef GEKKO */
  { NULL }
};
#endif  /* #ifdef USE_JOYSTICK */

static widget_menu_entry submenu_keyboard_buttons[] = {
  { "Select keyboard key" },
  { "Button \012U\011p", INPUT_KEY_u, submenu_select_key_for_button_1, NULL, get_key_name_for_button_1, 0 },
  { "Button \012D\011own", INPUT_KEY_d, submenu_select_key_for_button_2, NULL, get_key_name_for_button_2, 0 },
  { "Button \012L\011eft", INPUT_KEY_l, submenu_select_key_for_button_3, NULL, get_key_name_for_button_3, 0 },
  { "Button \012R\011ight", INPUT_KEY_r, submenu_select_key_for_button_4, NULL,	get_key_name_for_button_4, 0 },
  { "Button \012F\011ire", INPUT_KEY_f, submenu_select_key_for_button_5, NULL, get_key_name_for_button_5, 0 },
  { NULL }
};

#define MAX_JOYSTICK_TYPES 8
/* joystick types + title of the window + NULL */
static widget_menu_entry submenu_types[ MAX_JOYSTICK_TYPES + 2 ];
static char joystick_names[ MAX_JOYSTICK_TYPES ][ 100 ];
void set_joystick_type( int action );

#define SUBMENU_DEVICE_SELECTIONS( device ) \
\
static widget_menu_entry submenu_type_and_mapping_for_ ## device [] = { \
  { "Select type or map buttons" }, \
  { "\012T\011ype", INPUT_KEY_t, submenu_types, NULL, NULL, 0 }, \
  { "\012B\011utton Mapping", INPUT_KEY_b, submenu_ ## device ## _buttons, NULL, NULL, 0 }, \
  { NULL } \
};

#ifdef USE_JOYSTICK
SUBMENU_DEVICE_SELECTIONS( joystick )
#endif  /* #ifdef USE_JOYSTICK */
SUBMENU_DEVICE_SELECTIONS( keyboard )

static void
print_items( void )
{
  int i;
  char buffer[128];
  size_t height = 24;
  int width = widget_calculate_menu_width(menu);
  int menu_left_edge_x = (DISPLAY_WIDTH_COLS/2-width/2)*8+1;

  for( i = 0; i < count; i++ ) {
    int colour;
    if( !menu[i+1].text[0] ) { height += 4; continue; }

    snprintf( buffer, sizeof (buffer), "%s", menu[i+1].text );
    colour = menu[i+1].inactive ?
	     WIDGET_COLOUR_DISABLED :
	     WIDGET_COLOUR_FOREGROUND;

    if( i == highlight_line ) {
      widget_rectangle( menu_left_edge_x, height, width*8-2, 1*8,
                        WIDGET_COLOUR_HIGHLIGHT );
    } else {
      widget_rectangle( menu_left_edge_x, height, width*8-2, 1*8,
                        WIDGET_COLOUR_BACKGROUND );
    }

    widget_printstring( menu_left_edge_x+8, height, colour, buffer );

    if( menu[i+1].submenu ) {
      widget_draw_submenu_arrow(DISPLAY_BORDER_ASPECT_WIDTH+menu_left_edge_x+
                                width*8-9, i*8+49, colour);
    }

    if( menu[i+1].detail ) {
      size_t detail_width = widget_stringwidth( menu[i+1].detail() );
      int x = menu_left_edge_x + (width-1)*8 - detail_width - 2;
      widget_printstring( x, height, WIDGET_COLOUR_DISABLED, menu[i+1].detail() );
    }

    height += 8;
  }

  widget_display_lines( 2, count + 2 );
}

int widget_menu_draw( void *data )
{
  widget_menu_entry *ptr;
  size_t menu_entries, width, height = 0;
  int menu_left_edge_x;
  char buffer[128];
  highlight_line = 0;

  menu = (widget_menu_entry*)data;

  /* How many menu items do we have? */
  for( ptr = &menu[1]; ptr->text; ptr++ )
    height += ptr->text[0] ? 2 : 1;
  menu_entries = ptr - &menu[1];
  count = menu_entries;
  width = widget_calculate_menu_width(menu);
  menu_left_edge_x = DISPLAY_WIDTH_COLS/2-width/2;
  widget_dialog_with_border( menu_left_edge_x, 2, width, 2 + height / 2 );

  snprintf( buffer, sizeof( buffer ), "%s", menu->text );
  widget_printstring( menu_left_edge_x*8+2, 16, WIDGET_COLOUR_TITLE, buffer );

  print_items();
  return 0;
}

void
widget_menu_keyhandler( input_key key )
{
  widget_menu_entry *ptr;
  int new_highlight_line = 0;
  int cursor_pressed = 0;

  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_menu_draw( menu );
    break;
#endif
    
  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    ptr=&menu[1 + highlight_line];
    if(!ptr->inactive) {
      if( ptr->submenu ) {
        widget_do_menu( ptr->submenu );
      } else {
        ptr->callback( ptr->action );
      }
    }
    return;

  case INPUT_KEY_Up:
  case INPUT_KEY_7:
  case INPUT_JOYSTICK_UP:
    if ( highlight_line ) {
      new_highlight_line = highlight_line - 1;
      cursor_pressed = 1;
    }
    break;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
  case INPUT_JOYSTICK_DOWN:
    if ( highlight_line + 1 < (ptrdiff_t)count ) {
      new_highlight_line = highlight_line + 1;
      cursor_pressed = 1;
    }
    break;

  default:	/* Keep gcc happy */
    break;

  }

  if( cursor_pressed ) {
    highlight_line = new_highlight_line;
    print_items();
    return;
  }

  for( ptr=&menu[1]; ptr->text; ptr++ ) {
    if( !ptr->inactive && key == ptr->key ) {

      if( ptr->submenu ) {
        widget_do_menu( ptr->submenu );
      } else {
        ptr->callback( ptr->action );
      }

      break;
    }
  }
}

/* General callbacks */

scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  size_t count, i;
  const char *options[ SCALER_NUM ];
  widget_select_t info;
  int error;

  count = 0; info.current = 0;

  for( i = 0; i < SCALER_NUM; i++ )
    if( selector( i ) ) {
      if( current_scaler == i ) info.current = count;
      options[ count++ ] = scaler_name( i );
    }

  info.title = "Select scaler";
  info.options = options;
  info.count = count;
  info.finish_all = 1;

  error = widget_do_select( &info );
  if( error ) return SCALER_NUM;

  if( info.result == -1 ) return SCALER_NUM;

  for( i = 0; i < SCALER_NUM; i++ )
    if( selector( i ) && !info.result-- ) return i;

  ui_error( UI_ERROR_ERROR, "widget_select_scaler: ran out of scalers" );
  fuse_abort();
}

void
menu_file_exit( int action )
{
  static int menu_exit_open = 0;

  if( menu_exit_open ) return;

  menu_exit_open = 1;
  if( widget_do_query( "Exit Fuse?" ) || !widget_query.confirm ) {
    menu_exit_open = 0;
    return;
  }

  if( menu_check_media_changed() ) {
    menu_exit_open = 0;
    return;
  }

  fuse_exiting = 1;
  menu_exit_open = 0;

  widget_end_all( WIDGET_FINISHED_OK );
}

void
menu_options_general( int action )
{
  widget_do_general();
}

void
menu_options_media( int action )
{
  widget_do_media();
}

void
menu_options_peripherals_general( int action )
{
  widget_do_peripherals_general();
}

void
menu_options_peripherals_disk( int action )
{
  widget_do_peripherals_disk();
}

void
menu_options_sound( int action )
{
  widget_do_sound();
}

void
menu_options_rzx( int action )
{
  widget_do_rzx();
}

void
menu_options_movie( int action )
{
  widget_do_movie();
}

void
menu_options_diskoptions( int action )
{
  widget_do_diskoptions();
}

void
menu_options_joysticks_select( int action )
{
  int error = 0;
  int i;

  switch( action - 1 ) {

#ifdef USE_JOYSTICK
  case 0:
    current_settings[ 0 ] = &( settings_current.joystick_1_output );
    current_settings[ 1 ] = &( settings_current.joystick_1_fire_1 );
    current_settings[ 2 ] = &( settings_current.joystick_1_fire_2 );
    current_settings[ 3 ] = &( settings_current.joystick_1_fire_3 );
    current_settings[ 4 ] = &( settings_current.joystick_1_fire_4 );
    current_settings[ 5 ] = &( settings_current.joystick_1_fire_5 );
    current_settings[ 6 ] = &( settings_current.joystick_1_fire_6 );
    current_settings[ 7 ] = &( settings_current.joystick_1_fire_7 );
    current_settings[ 8 ] = &( settings_current.joystick_1_fire_8 );
    current_settings[ 9 ] = &( settings_current.joystick_1_fire_9 );
    current_settings[ 10 ] = &( settings_current.joystick_1_fire_10 );
    current_settings[ 11 ] = &( settings_current.joystick_1_fire_11 );
    current_settings[ 12 ] = &( settings_current.joystick_1_fire_12 );
    current_settings[ 13 ] = &( settings_current.joystick_1_fire_13 );
    current_settings[ 14 ] = &( settings_current.joystick_1_fire_14 );
    current_settings[ 15 ] = &( settings_current.joystick_1_fire_15 );
    submenu_type_and_mapping_for_joystick[ 1 ].detail = menu_joystick_1_detail;
    break;
  case 1:
    current_settings[ 0 ] = &( settings_current.joystick_2_output );
    current_settings[ 1 ] = &( settings_current.joystick_2_fire_1 );
    current_settings[ 2 ] = &( settings_current.joystick_2_fire_2 );
    current_settings[ 3 ] = &( settings_current.joystick_2_fire_3 );
    current_settings[ 4 ] = &( settings_current.joystick_2_fire_4 );
    current_settings[ 5 ] = &( settings_current.joystick_2_fire_5 );
    current_settings[ 6 ] = &( settings_current.joystick_2_fire_6 );
    current_settings[ 7 ] = &( settings_current.joystick_2_fire_7 );
    current_settings[ 8 ] = &( settings_current.joystick_2_fire_8 );
    current_settings[ 9 ] = &( settings_current.joystick_2_fire_9 );
    current_settings[ 10 ] = &( settings_current.joystick_2_fire_10 );
    current_settings[ 11 ] = &( settings_current.joystick_2_fire_11 );
    current_settings[ 12 ] = &( settings_current.joystick_2_fire_12 );
    current_settings[ 13 ] = &( settings_current.joystick_2_fire_13 );
    current_settings[ 14 ] = &( settings_current.joystick_2_fire_14 );
    current_settings[ 15 ] = &( settings_current.joystick_2_fire_15 );
    submenu_type_and_mapping_for_joystick[ 1 ].detail = menu_joystick_2_detail;
    break;
#endif  /* #ifdef USE_JOYSTICK */

  case JOYSTICK_KEYBOARD:
    current_settings[ 0 ] = &( settings_current.joystick_keyboard_output );
    current_settings[ 1 ] = &( settings_current.joystick_keyboard_up );
    current_settings[ 2 ] = &( settings_current.joystick_keyboard_down );
    current_settings[ 3 ] = &( settings_current.joystick_keyboard_left );
    current_settings[ 4 ] = &( settings_current.joystick_keyboard_right );
    current_settings[ 5 ] = &( settings_current.joystick_keyboard_fire );
    submenu_type_and_mapping_for_keyboard[ 1 ].detail = menu_keyboard_joystick_detail;
    break;
  }

  /* Populate joystick names */
  if( JOYSTICK_TYPE_COUNT > MAX_JOYSTICK_TYPES )
    ui_error( UI_ERROR_ERROR, "Not all joystick types are displayed" );

  submenu_types[ 0 ].text = "Select joystick type";
  for( i = 0; ( i < JOYSTICK_TYPE_COUNT ) && ( i < MAX_JOYSTICK_TYPES ); i++ ) {
    char shortcut[ 2 ] = { 'A' + i, '\0' };
    snprintf( ( char * ) joystick_names[ i ], 100, "\012%s\011 %s", shortcut,
              joystick_name[ i ] );
    submenu_types[ i + 1 ].text = joystick_names[ i ];
    submenu_types[ i + 1 ].key = INPUT_KEY_a + i;
    submenu_types[ i + 1 ].callback = set_joystick_type;
    submenu_types[ i + 1 ].action = i;
  }
  submenu_types[ i + 1 ].text = NULL;

  if( action - 1 == JOYSTICK_KEYBOARD ) 
    error = widget_do_menu( submenu_type_and_mapping_for_keyboard );

#ifdef USE_JOYSTICK
  else
    error = widget_do_menu( submenu_type_and_mapping_for_joystick );
#endif  /* #ifdef USE_JOYSTICK */

  if( error ) return;
}

void
set_joystick_type( int action )
{
  *current_settings[ 0 ] = action;
  widget_end_all( WIDGET_FINISHED_OK );
}

/* Options/Select ROMs/<type> */
int
menu_select_roms_with_title( const char *title, size_t start, size_t count,
			     int is_peripheral )
{
  widget_roms_info info;

  info.title = title;
  info.start = start;
  info.count = count;
  info.is_peripheral = is_peripheral;
  info.initialised = 0;

  return widget_do_rom( &info );
}

void
menu_machine_reset( int action )
{
  int hard_reset = action;
  const char *message = "Reset?";

  if( hard_reset )
    message = "Hard reset?";

  if( widget_do_query( message ) ||
      !widget_query.confirm )
    return;

  widget_end_all( WIDGET_FINISHED_OK );

  /* Stop any ongoing RZX */
  rzx_stop_recording();
  rzx_stop_playback( 1 );

  machine_reset( hard_reset );
}

void
menu_machine_select( int action )
{
  widget_select_t info;
  char **options, *buffer;
  size_t i;
  int error;
  libspectrum_machine new_machine;

  options = malloc( machine_count * sizeof( const char * ) );
  if( !options ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  buffer = malloc( 40 * machine_count );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    free( options );
    return;
  }

  for( i = 0; i < machine_count; i++ ) {
    options[i] = &buffer[ i * 40 ];
    snprintf( options[i], 40, "%s",
              libspectrum_machine_name( machine_types[i]->machine ) );
    if( machine_current->machine == machine_types[i]->machine )
      info.current = i;
  }

  info.title = "Select machine";
  info.options = (const char**)options;
  info.count = machine_count;
  info.finish_all = 1;

  error = widget_do_select( &info );
  free( buffer ); free( options );
  if( error ) return;

  if( info.result == -1 ) return;

  new_machine = machine_types[ info.result ]->machine;

  if( machine_current->machine != new_machine ) machine_select( new_machine );
}

void
menu_machine_debugger( int action )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  widget_do_debugger();
}

void
menu_machine_pokememory( int action )
{
  widget_do_pokemem();
}

void
menu_machine_pokefinder( int action )
{
  widget_do_pokefinder();
}

void
menu_machine_memorybrowser( int action )
{
  widget_do_memorybrowser();
}

void
menu_media_tape_browse( int action )
{
  widget_do_browse();
}

void
menu_help_keyboard( int action )
{
  utils_file screen;
  widget_picture_data info;

  static const char * const filename = "keyboard.scr";

  if( utils_read_screen( filename, &screen ) ) {
    return;
  }

  info.filename = filename;
  info.screen = screen.buffer;
  info.border = 0;

  widget_do_picture( &info );

  utils_close_file( &screen );
}

void
menu_help_about( int action )
{
  widget_do_about();
}

static int
set_active( struct widget_menu_entry *menu, const char *path, int active )
{
  if( *path == '/' ) path++;

  /* Skip the menu title */
  menu++;

  for( ; menu->text; menu++ ) {

    const char *p = menu->text, *q = path;

    /* Compare the two strings, but skip hotkey-delimiter characters */
    do {
      if( *p == 9 || *p == 10 ) p++;
    } while( *p && *p++ == *q++ );

    if( *p ) continue;		/* not matched */

    /* match, but with a submenu */
    if( *q == '/' ) return set_active( menu->submenu, q, active );

    if( *q ) continue;		/* not matched */

    /* we have a match */
    menu->inactive = !active;
    return 0; 
  }

  return 1;
}

int
ui_menu_item_set_active( const char *path, int active )
{
  return set_active( widget_menu, path, active );
}
