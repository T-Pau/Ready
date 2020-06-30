/* select.c: generic selection widget
   Copyright (c) 2001-2004 Philip Kendall, Witold Filipczyk
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

#include <string.h>

#include "widget_internals.h"

/* Data for drawing the cursor */
static size_t highlight_line;

const char *title;
const char * const *options;
static int finish_all;
static size_t count;
static int *result;

static void print_item (int left_edge, int width, int index, int colour)
{
  char key[] = "\x0A ";
  key[1] = 'A' + index;
  left_edge = ( left_edge + 1 ) * 8 + 1;
  left_edge = widget_printstring( left_edge, index * 8 + 24, colour, key );
  left_edge = widget_printstring( left_edge + 1, index * 8 + 24, colour, ": " );
  widget_printstring( left_edge + 1, index * 8 + 24, colour, options[index] );
}

const int select_vert_external_margin = 8;

static int
widget_calculate_select_width( const char* title )
{
  int i;
  int max_width = widget_stringwidth( title )+5*8;
  /* Leave room for the option labels we'll be adding */
  int label_width = widget_stringwidth( "A: " );

  for( i = 0; i < count; i++ ) {
    int total_width = label_width + widget_stringwidth( options[i] ) + 3 * 8;
    if( total_width > max_width )
      max_width = total_width;
  }

  return ( max_width + select_vert_external_margin * 2 ) / 8;
}

int
widget_select_draw( void *data )
{
  int i;
  size_t width;
  int menu_left_edge_x;

  if( data ) {

    widget_select_t *ptr = data;

    title = ptr->title;
    options = ptr->options;
    count = ptr->count;
    result = &( ptr->result );
    finish_all = ptr->finish_all;

    highlight_line = ptr->current;

  }

  width = widget_calculate_select_width( title );
  menu_left_edge_x = DISPLAY_WIDTH_COLS/2-width/2;

  /* Blank the main display area */
  widget_dialog_with_border( menu_left_edge_x, 2, width, count + 2 );

  widget_printstring( menu_left_edge_x*8+2, 16, WIDGET_COLOUR_TITLE, title );

  for( i = 0; i < count; i++ ) {
    if( i == highlight_line ) {
      widget_rectangle( menu_left_edge_x*8+1, i*8+24, width*8-2, 1*8, WIDGET_COLOUR_HIGHLIGHT );
    }
    print_item( menu_left_edge_x, width, i, WIDGET_COLOUR_FOREGROUND );
  }

  widget_display_lines( 2, count + 2 );

  return 0;
}

void
widget_select_keyhandler( input_key key )
{
  int new_highlight_line = 0;
  int cursor_pressed = 0;
  size_t width = widget_calculate_select_width( title );
  int menu_left_edge_x = DISPLAY_WIDTH_COLS/2-width/2;

  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_select_draw( NULL );
    break;
#endif

  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    widget_end_widget( WIDGET_FINISHED_OK );
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

  case INPUT_KEY_Home:
    if ( highlight_line ) {
      new_highlight_line = 0;
      cursor_pressed = 1;
    }
    break;

  case INPUT_KEY_End:
    if ( highlight_line + 2 < (ptrdiff_t)count ) {
      new_highlight_line = (ptrdiff_t)count - 1;
      cursor_pressed = 1;
    }
    break;

  default:	/* Keep gcc happy */
    break;

  }

  if( cursor_pressed                                  ||
      ( key >= INPUT_KEY_a && key <= INPUT_KEY_z &&
	key - INPUT_KEY_a < (ptrdiff_t)count        )
    ) {
    
    /* Remove the old highlight */
    widget_rectangle( menu_left_edge_x * 8 + 1, highlight_line * 8 + 24,
                      width * 8 - 2, 1 * 8, WIDGET_COLOUR_BACKGROUND );
    print_item( menu_left_edge_x, width, highlight_line,
                WIDGET_COLOUR_FOREGROUND );

    /*  draw the new one */
    if( cursor_pressed ) {
      highlight_line = new_highlight_line;
    } else {
      highlight_line = key - INPUT_KEY_a;
    }
    
    widget_rectangle( menu_left_edge_x * 8 + 1, highlight_line * 8 + 24,
                      width * 8 - 2, 1 * 8, WIDGET_COLOUR_HIGHLIGHT );
    print_item( menu_left_edge_x, width, highlight_line, WIDGET_COLOUR_FOREGROUND );

    widget_display_lines( 2, count + 2 );
  }
}

int widget_select_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {
    *result = highlight_line;
    if( finish_all )
      widget_end_all( WIDGET_FINISHED_OK );
  } else {
    *result = -1;
  }

  return 0;
}
