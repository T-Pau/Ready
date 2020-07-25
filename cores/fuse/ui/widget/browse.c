/* browse.c: tape browser widget
   Copyright (c) 2002-2004 Philip Kendall

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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#else				/* #ifdef HAVE_LIB_GLIB */
#include <libspectrum.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include "fuse.h"
#include "tape.h"
#include "widget_internals.h"

#define MAX_BLOCK_DESC 30

/* The descriptions of the blocks */
static GSList *blocks;

/* How many blocks we have in total */
size_t block_count;

/* Which block is shown on the top line of the widget */
static int top_line;

/* Which block is currently highlighted */
static int highlight;

static void show_blocks( void );
static void add_block_description( libspectrum_tape_block *block,
				   void *user_data );
static void free_description( gpointer data, gpointer user_data );

int
widget_browse_draw( void *data GCC_UNUSED )
{
  int error;

  blocks = NULL; block_count = 0;
  error = tape_foreach( add_block_description, &blocks );
  if( error ) return error;

  widget_dialog_with_border( 1, 2, 30, 20 );

  widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "Browse Tape" );
  widget_display_lines( 2, 1 );

  highlight = tape_get_current_block();
  top_line = highlight - 8; if( top_line < 0 ) top_line = 0;

  show_blocks();

  return 0;
}

static void
add_block_description( libspectrum_tape_block *block, void *user_data )
{
  char data_detail[MAX_BLOCK_DESC];
  GSList **ptr = user_data;

  char *buffer;

  tape_block_details( data_detail, MAX_BLOCK_DESC, block );
  buffer = malloc( MAX_BLOCK_DESC ); if( !buffer ) return;
  libspectrum_tape_block_description( buffer, MAX_BLOCK_DESC, block );
  if( strlen( data_detail ) ) strcpy( buffer, data_detail );

  (*ptr) = g_slist_append( *ptr, buffer );

  block_count++;
}

static void
show_blocks( void )
{
  size_t i; char buffer[64];
  GSList *ptr;
  int numpos = g_slist_length( blocks );

  if( numpos < 10 ) {
    numpos = 24;
  } else if( numpos < 100 ) {
    numpos = 32;
  } else {
    numpos = 40;
  }

  widget_rectangle( 9, 2*8, 30*8-2, 19*8, WIDGET_COLOUR_BACKGROUND );

  for( i = 0, ptr = g_slist_nth( blocks, top_line );
       i < 18 && ptr;
       i++, ptr = ptr->next ) {

    int colour = WIDGET_COLOUR_FOREGROUND;

    if( top_line + i == highlight )
      widget_rectangle( 9, i*8+24, 30*8-2, 1*8, WIDGET_COLOUR_HIGHLIGHT );

    sprintf( buffer, "%lu", (unsigned long)( top_line + i + 1 ) );
    widget_printstring_right( numpos, i*8+24, colour, buffer );
    snprintf( buffer, sizeof( buffer ), ": %s", (char *)ptr->data );
    widget_printstring( numpos + 1, i*8+24, colour, buffer );
  }

  widget_display_lines( 3, 19 );
}

void
widget_browse_keyhandler( input_key key )
{
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_browse_draw( NULL );
    break;
#endif

  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
  case INPUT_KEY_j:
  case INPUT_JOYSTICK_DOWN:
    if( highlight < block_count - 1 ) {
      highlight++;
      if( highlight >= top_line + 18 ) top_line += 18;
      show_blocks();
    }
    break;
    
  case INPUT_KEY_Up:
  case INPUT_KEY_7:
  case INPUT_KEY_k:
  case INPUT_JOYSTICK_UP:
    if( highlight > 0 ) { 
      highlight--;
      if( highlight < top_line )
	{
	  top_line -= 18;
	  if( top_line < 0 ) top_line = 0;
	}
      show_blocks();
    }
    break;

  case INPUT_KEY_Page_Up:
    highlight -= 18; if( highlight < 0 ) highlight = 0;
    top_line  -= 18; if( top_line  < 0 ) top_line = 0;
    show_blocks();
    break;

  case INPUT_KEY_Page_Down:
    highlight += 18;
    if( highlight >= block_count ) highlight = block_count - 1;
    top_line += 18;
    if( top_line >= block_count ) {
      top_line = block_count - 18;
      if( top_line < 0 ) top_line = 0;
    }
    show_blocks();
    break;

  case INPUT_KEY_Home:
    highlight = top_line = 0;
    show_blocks();
    break;

  case INPUT_KEY_End:
    highlight = block_count - 1;
    top_line = block_count - 18; if( top_line < 0 ) top_line = 0;
    show_blocks();
    break;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }
}

int
widget_browse_finish( widget_finish_state finished )
{
  g_slist_foreach( blocks, free_description, NULL );
  g_slist_free( blocks );

  if( finished == WIDGET_FINISHED_OK ) {
    if( highlight != -1 ) tape_select_block( highlight );
    widget_end_all( WIDGET_FINISHED_OK );
  }
    
  return 0;
}

static void 
free_description( gpointer data, gpointer user_data )
{
  free( data );
}
