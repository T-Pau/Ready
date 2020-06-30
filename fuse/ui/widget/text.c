/* text.c: simple text entry widget
   Copyright (c) 2002-2005 Philip Kendall
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
#include <ctype.h>

#include "widget_internals.h"

char *widget_text_text = NULL;	/* What we return the text in */

static const char *title;	/* The window title */
static widget_text_input_allow allow;
static unsigned int max_length;

#define WIDGET_TEXT_LENGTH 64
static char text[WIDGET_TEXT_LENGTH];	/* The current entry text */

static int widget_text_draw_text( void );
static void delete_character( void );
static void append_character( char c );

int
widget_text_draw( void *data )
{
  widget_text_t* text_data = data;

  if( data ) {
    title = text_data->title;
    allow = text_data->allow;
    max_length = text_data->max_length;
    snprintf( text, sizeof( text ), "%s", text_data->text );
  }

  widget_dialog_with_border( 1, 2, 30, 3 );

  widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, title );
  widget_printstring_right( 12, 28, 5, "[" );
  widget_printstring( 244, 28, 5, "]" );

  widget_display_lines( 2, 3 );

  return widget_text_draw_text();
}

static int
widget_text_draw_text( void )
{
  int width;
  const char *tptr;

  widget_rectangle( 12, 28, 232, 8, WIDGET_COLOUR_BACKGROUND );

  tptr = text - 1;
  do {
    width = widget_stringwidth (++tptr);
  } while (width > 28 * 8 - 4);

  if( tptr != text )
    widget_rectangle( 14, 29, 1, 6, 5 );

  widget_printstring( 16, 28, WIDGET_COLOUR_FOREGROUND, tptr );
  widget_rectangle( 17 + width, 35, 4, 1, 5 );

  widget_display_rasters( 28, 8 );
  return 0;
}

void
widget_text_keyhandler( input_key key )
{
  switch( key ) {

  case INPUT_KEY_BackSpace:	/* Backspace generates DEL which is Caps + 0 */
    delete_character(); widget_text_draw_text();
    return;

  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:			/* Keep gcc happy */
    break;

  }

  /* Input validation.
   * We rely on various INPUT_KEY_* being mapped directly onto ASCII.
   */
 
  /* FIXME: we *don't* want keypresses filtered through the input layer */
 
  /* First, return if the character isn't printable ASCII. */
  if( key < ' ' || key > '~' ) return;
 
  /* Return if the key isn't valid. */
  switch( allow ) {
  case WIDGET_INPUT_ASCII:
    break;
  case WIDGET_INPUT_DIGIT:
    if( !isdigit( key ) ) return;
    break;
  case WIDGET_INPUT_ALPHA:
    if( !isalpha( key ) ) return;
    break;
  case WIDGET_INPUT_ALNUM:
    if( !isdigit( key ) && !isalpha( key ) ) return;
    break;
  }
  
  /* If we've got this far, we have a valid key */
  append_character( key );
 
  widget_text_draw_text();
}

static void 
delete_character( void )
{
  size_t length = strlen( text );

  if( length ) text[ length - 1 ] = '\0';
}

static void
append_character( char c )
{
  size_t length = strlen( text );

  if( length < WIDGET_TEXT_LENGTH - 1 && length < max_length ) {
    text[ length ] = c; text[ length + 1 ] = '\0';
  }
}

int
widget_text_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {

    widget_text_text =
      libspectrum_renew( char, widget_text_text, strlen( text ) + 1 );

    strcpy( widget_text_text, text );
  } else {
    free( widget_text_text );
    widget_text_text = NULL;
  }

  return 0;
}
