/* query.c: The query widgets
   Copyright (c) 2004-2008 Darren Salt, Fredrick Meunier
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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fuse.h"
#include "utils.h"
#include "widget_internals.h"

widget_query_t widget_query;

static const char * const title = "Fuse - Confirm";

struct widget_query_entry;

/* A generic click function */
typedef void (*widget_query_click_fn)( void );

/* A general menu */
typedef struct widget_query_entry {
  const char *text;
  int index;
  input_key key;		/* Which key to activate this query */

  widget_query_click_fn click;
} widget_query_entry;

static void widget_save_click( void );
static void widget_dont_save_click( void );
static void widget_cancel_click( void );
static void widget_yes_click( void );
static void widget_no_click( void );
static void widget_query_line_draw( int left_edge, int width,
                                    struct widget_query_entry *menu,
                                    const char *label );

static widget_query_entry query_save[] = {
  { "\012S\001ave", 0, INPUT_KEY_s, widget_save_click },
  { "\012D\001on't save", 1, INPUT_KEY_d, widget_dont_save_click },
  { "\012C\001ancel", 2, INPUT_KEY_c, widget_cancel_click },
  { NULL }
};

static widget_query_entry query_confirm[] = {
  { "\012Y\001es", 0, INPUT_KEY_y, widget_yes_click },
  { "\012N\001o", 1, INPUT_KEY_n, widget_no_click },
  { NULL }
};

static void
widget_save_click( void )
{
  widget_query.save = UI_CONFIRM_SAVE_SAVE;
}

static void
widget_dont_save_click( void )
{
  widget_query.save = UI_CONFIRM_SAVE_DONTSAVE;
}

static void
widget_cancel_click( void )
{
  widget_query.save = UI_CONFIRM_SAVE_CANCEL;
}

static void
widget_yes_click( void )
{
  widget_query.confirm = 1;
}

static void
widget_no_click( void )
{
  widget_query.confirm = 0;
}

static size_t highlight_line = 0;

static char **message_lines;
static size_t num_message_lines;

static void
widget_query_line_draw( int left_edge, int width, struct widget_query_entry *menu,
                        const char *label )
{
  int colour = WIDGET_COLOUR_BACKGROUND;
  int y = (menu->index + num_message_lines) * 8 + 24;

  if( menu->index == highlight_line ) colour = WIDGET_COLOUR_HIGHLIGHT;
  widget_rectangle( left_edge*8+1, y, width*8-2, 1*8, colour );
  widget_printstring( left_edge*8+8, y, WIDGET_COLOUR_FOREGROUND,
                      menu->text );
  widget_display_rasters( y, 8 );
}

const int query_vert_external_margin = 8;

static int
widget_calculate_query_width( const char *title, widget_query_entry *menu,
			      char **lines, int num_lines )
{
  widget_query_entry *ptr;
  int max_width=0;
  int i;

  if (!menu) {
    return 64;
  }

  max_width = widget_stringwidth( title )+5*8;

  for( ptr = menu; ptr->text; ptr++ ) {
    int total_width = widget_stringwidth( ptr->text )+3*8;

    if (total_width > max_width)
      max_width = total_width;
  }

  for( i=0; i<num_lines; i++) {
    int total_width = widget_stringwidth( lines[i] )+2*8;

    if( total_width > max_width )
      max_width = total_width;
  }

  return ( max_width + query_vert_external_margin * 2 ) / 8;
}

static int
internal_query_draw( widget_query_entry *query, int save, const char *message )
{
  widget_query_entry *ptr;
  size_t height = 0;
  int menu_width;
  int menu_left_edge_x;
  int i;

  if( split_message( message, &message_lines, &num_message_lines, 28 ) ) {
    return 1;
  }

  menu_width = widget_calculate_query_width( title, query, message_lines,
					     num_message_lines );

  height = num_message_lines;

  /* How many options do we have? */
  for( ptr = query; ptr->text; ptr++ )
    height ++;

  menu_left_edge_x = DISPLAY_WIDTH_COLS/2-menu_width/2;

  /* Draw the dialog box */
  widget_dialog_with_border( menu_left_edge_x, 2, menu_width, 2 + height );

  widget_printstring( menu_left_edge_x*8+2, 16, WIDGET_COLOUR_TITLE, title );

  for( i=0; i<num_message_lines; i++ ) {
    widget_printstring( menu_left_edge_x*8+8, i*8+24,
                        WIDGET_COLOUR_FOREGROUND, message_lines[i] );
  }

  for( ptr = query; ptr->text; ptr++ ) {
    widget_query_line_draw( menu_left_edge_x, menu_width, ptr, ptr->text );
  }

  widget_display_lines( 2, 2 + height );

  return 0;
}

int
widget_query_draw( void *data )
{
  highlight_line = 0;
  widget_query.confirm = 0;
  return internal_query_draw( query_confirm, 0, (const char *) data );
}

int
widget_query_save_draw( void *data )
{
  highlight_line = 0;
  widget_query.save = UI_CONFIRM_SAVE_CANCEL;
  return internal_query_draw( query_save, 1, (const char *) data );
}

static void
widget_query_generic_keyhandler( widget_query_entry *query, int num_entries,
                                 input_key key )
{
  int new_highlight_line = 0;
  int cursor_pressed = 0;
  widget_query_entry *ptr;
  int menu_width = widget_calculate_query_width( title, query, message_lines,
						 num_message_lines );
  int menu_left_edge_x = DISPLAY_WIDTH_COLS/2-menu_width/2;

  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_dialog_with_border( 1, 2, 30, 2 + 20 );
    widget_general_show_all( &widget_options_settings );
    break;
#endif
    
  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;

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
    if ( highlight_line < num_entries - 2 ) {
      new_highlight_line = highlight_line + 1;
      cursor_pressed = 1;
    }
    break;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    query[highlight_line].click();
    widget_end_all( WIDGET_FINISHED_OK );
    display_refresh_all();
    return;

  default:	/* Keep gcc happy */
    break;

  }

  if( cursor_pressed ) {
    int old_highlight_line = highlight_line;
    highlight_line = new_highlight_line;
    widget_query_line_draw( menu_left_edge_x, menu_width,
                            query + old_highlight_line,
                            query[old_highlight_line].text );
    widget_query_line_draw( menu_left_edge_x, menu_width,
                            query + highlight_line,
                            query[highlight_line].text );
    return;
  }

  for( ptr=query; ptr->text != NULL; ptr++ ) {
    if( key == ptr->key ) {
      int old_highlight_line = highlight_line;
      ptr->click();
      highlight_line = ptr->index;
      widget_query_line_draw( menu_left_edge_x, menu_width,
                              query + old_highlight_line,
                              query[old_highlight_line].text );
      widget_query_line_draw( menu_left_edge_x, menu_width, ptr,
                              query[highlight_line].text );
      break;
    }
  }
}

void
widget_query_keyhandler( input_key key )
{
  widget_query_generic_keyhandler( query_confirm,
                                   ARRAY_SIZE(query_confirm),
                                   key );
}

void
widget_query_save_keyhandler( input_key key )
{
  widget_query_generic_keyhandler( query_save,
                                   ARRAY_SIZE(query_save),
                                   key );
}

int
widget_query_finish( widget_finish_state finished )
{
  int i;
  for( i=0; i<num_message_lines; i++ ) {
    free( message_lines[i] );
  }
  free( message_lines );
  message_lines = NULL;
  num_message_lines = 0;

  return 0;
}
