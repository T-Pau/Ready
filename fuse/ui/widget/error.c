/* error.c: The error reporting widget
   Copyright (c) 2002-2005 Philip Kendall

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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fuse.h"
#include "widget_internals.h"

widget_error_t *error_info;

int
ui_error_specific( ui_error_level severity, const char *message )
{
  widget_error_t error_info;
  /* Can't output widgets if we don't have a display yet */
  if( !display_ui_initialised ) return 0;


  error_info.severity = severity;
  error_info.message  = message;

  fuse_emulation_pause();
  widget_do_error( &error_info );
  fuse_emulation_unpause();

  return 0;
}

int widget_error_draw( void *data )
{
  char **lines; size_t count;
  size_t i;

  error_info = (widget_error_t*)data;
  if( split_message( error_info->message, &lines, &count, 28 ) ) return 1;
  
  widget_dialog_with_border( 1, 2, 30, count+2 );

  switch( error_info->severity ) {
  case UI_ERROR_INFO:
    widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "Info" );
    break;
  case UI_ERROR_WARNING:
    widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "Warning" );
    break;
  case UI_ERROR_ERROR:
    widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "Error" );
    break;
  default:
    widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "(Unknown message)" );
    break;
  }

  for( i=0; i<count; i++ ) {
    widget_printstring( 17, i*8+24, WIDGET_COLOUR_FOREGROUND, lines[i] );
    free( lines[i] );
  }

  free( lines );

  widget_display_lines( 2, count + 3 );

  return 0;
}

int
split_message( const char *message, char ***lines, size_t *count,
	       size_t line_length )
{
  const char *ptr = message;
  int position;

  line_length *= 8;

  /* Setup so we'll allocate the first line as soon as we get the first
     word */
  *lines = NULL; *count = 0; position = line_length;

  while( *ptr ) {

    /* Skip any whitespace */
    while( *ptr && isspace( *ptr ) ) ptr++;
    message = ptr;

    /* Find end of word */
    while( *ptr && !isspace( *ptr ) ) ptr++;

    /* message now points to a word of length (ptr-message); if
       that's longer than an entire line (most likely filenames), just
       take the last bit */
    while( widget_substringwidth( message, ptr - message ) >= line_length )
      message++;

    /* Check we've got room for the word, plus some prefixing space */
    if( position + widget_substringwidth( message, ptr - message ) + 4
 	>= line_length ) {

      char **new_lines; size_t i;

      /* If we've filled the screen, stop */
      if( *count == 18 ) return 0;

      new_lines = realloc( (*lines), (*count + 1) * sizeof( char* ) );
      if( new_lines == NULL ) {
	for( i=0; i<*count; i++ ) free( (*lines)[i] );
	if(*lines) free( (*lines) );
	return 1;
      }
      (*lines) = new_lines;

      (*lines)[*count] = malloc( (line_length+1) );
      if( (*lines)[*count] == NULL ) {
	for( i=0; i<*count; i++ ) free( (*lines)[i] );
	free( (*lines) );
	return 1;
      }
      
      strncpy( (*lines)[*count], message, ptr - message );
      position = widget_substringwidth( message, ptr - message );
      (*lines)[*count][ptr - message] = '\0';

      (*count)++;

    } else {		/* Enough room on this line */

      strcat( (*lines)[*count-1], " " );
      (*lines)[*count-1][strlen( (*lines)[*count-1] ) + ptr - message] = '\0';
      strncat( (*lines)[*count-1], message, ptr - message );
      position += widget_substringwidth( message, ptr - message ) + 4;

    }

  }

  return 0;
}

void
widget_error_keyhandler( input_key key )
{
  switch( key ) {

  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }
}
