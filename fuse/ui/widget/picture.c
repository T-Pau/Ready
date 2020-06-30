/* picture.c: Keyboard picture
   Copyright (c) 2001-2004 Philip Kendall

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

#include "display.h"
#include "ui/uidisplay.h"
#include "widget_internals.h"

static widget_picture_data *ptr;

int widget_picture_draw( void* data )
{
  ptr = (widget_picture_data*)data;

  uidisplay_spectrum_screen( ptr->screen, ptr->border );
  uidisplay_frame_end();

  return 0;
}

void
widget_picture_keyhandler( input_key key )
{
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on widget resize */
    widget_picture_draw( ptr );
    break;
#endif
    
  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
    widget_end_all( WIDGET_FINISHED_OK );
    break;

  default:	/* Keep gcc happy */
    break;

  }
}
