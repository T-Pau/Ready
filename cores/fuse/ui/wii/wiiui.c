/* wiiui.h: routines for dealing with the Wii FB UI
   Copyright (c) 2008 Bjoern Giesler

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

   E-mail: bjoern@giesler.de

*/

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "wiidisplay.h"
#include "wiikeyboard.h"
#include "wiimouse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

static void wii_end( void );

int
ui_init( int *argc, char ***argv )
{
  int error;

#ifdef USE_WIDGET
  if( ui_widget_init() ) return 1;
#endif				/* #ifdef USE_WIDGET */

  error = atexit( wii_end );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s: couldn't set atexit function", __func__ );
    return 1;
  }
  
  error = wiidisplay_init();
  if( error ) return error;
  error = wiikeyboard_init();
  if( error ) return error;
  error = wiimouse_init();
  if( error ) return error;

  return 0;
}

int
ui_event( void )
{
  keyboard_update();
  mouse_update();
  return 0;
}

int ui_end(void)
{
  /* Cleanup handled by atexit function */
  int error;
  
  error = wiikeyboard_end(); if( error ) return error;
  error = wiidisplay_end(); if( error ) return error;

#ifdef USE_WIDGET
  ui_widget_end();
#endif                          /* #ifdef USE_WIDGET */

  return 0;
}

static void
wii_end( void )
{
  wiikeyboard_end();
  uidisplay_end();
}
