/* about.c: about dialog box
   Copyright (c) 2016 Sergio Baldoví

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

   E-mail: serbalgi@gmail.com

*/

#include <config.h>

#include <stdio.h>

#include "widget.h"
#include "widget_internals.h"

int
widget_about_draw( void *data GCC_UNUSED )
{
  char buffer[80];
  int dialog_cols, string_width, margin, x, line;

  dialog_cols = 30;
  margin = 17;
  line = 0;

  widget_dialog_with_border( 1, 2, dialog_cols, 7+2 );
  widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, "About Fuse" );

  string_width = widget_stringwidth( "the Free Unix Spectrum Emulator (Fuse)" );
  x = margin - 8 + ( dialog_cols * 8 - string_width ) / 2;
  widget_printstring( x, ++line * 8 + 24, WIDGET_COLOUR_FOREGROUND,
                      "the Free Unix Spectrum Emulator (Fuse)" );

  snprintf( buffer, 80, "Version %s", VERSION );
  string_width = widget_stringwidth( buffer );
  x = margin - 8 + ( dialog_cols * 8 - string_width ) / 2;
  widget_printstring( x, ++line * 8 + 24, WIDGET_COLOUR_FOREGROUND, buffer );

  ++line;

  string_width = widget_stringwidth( FUSE_COPYRIGHT );
  x = margin - 8 + ( dialog_cols * 8 - string_width ) / 2;
  widget_printstring( x, ++line * 8 + 24, WIDGET_COLOUR_FOREGROUND,
                      FUSE_COPYRIGHT );

  ++line;

  string_width = widget_stringwidth( PACKAGE_URL );
  x = margin - 8 + ( dialog_cols * 8 - string_width ) / 2;
  widget_printstring( x, ++line * 8 + 24, 0x09, PACKAGE_URL );

  widget_display_lines( 2, line + 3 );

  return 0;
}

void
widget_about_keyhandler( input_key key )
{
  switch( key ) {

  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }
}
