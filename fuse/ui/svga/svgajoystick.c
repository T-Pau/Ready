/* svgajoystick.c: Joystick emulation (using svgalib)
   Copyright (c) 2003-4 Darren Salt
   Copyright (c) 2015 UB880D
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

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>
#include "peripherals/joystick.h"

#if !defined USE_JOYSTICK || defined HAVE_JSW_H

/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

/* Use the svgalib joystick support */

#include <string.h>
#include <errno.h>

#include <libspectrum.h>
#include <vgajoystick.h>

#include "fuse.h"
#include "keyboard.h"
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

static int sticks = 0;
static int buttons[2];

static void joy_handler( int ev, int number, char value, int which );

static int
init_stick( int which )
{
  if( !joystick_init( which, JOY_CALIB_STDOUT ) ) {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i: %s",
	      which + 1, errno ? strerror (errno) : "not configured?" );
    return 1;
  }

  if( joystick_getnumaxes( which ) < 2    ||
      joystick_getnumbuttons( which ) < 1    ) {
    joystick_close( which );
    ui_error( UI_ERROR_ERROR, "sorry, joystick %i is inadequate!", which + 1 );
    return 1;
  }

  buttons[which] = joystick_getnumbuttons( which );
  if( buttons[which] > NUM_JOY_BUTTONS ) buttons[which] = NUM_JOY_BUTTONS;

  return 0;
}

int
ui_joystick_init( void )
{
  int i;

  /* If we can't init the first, don't try the second */
  if( init_stick( 0 ) ) {
    sticks = 0;
  } else if( init_stick( 1 ) ) {
    sticks = 1;
  } else {
    sticks = 2;
  }

  for( i = 0; i < sticks; i++ ) {
    joystick_sethandler( i, joy_handler );
  }

  return sticks;
}

void
ui_joystick_end( void )
{
  joystick_close( -1 );
}

static void
do_axis( int which, int position, input_key negative, input_key positive )
{
  input_event_t event1, event2;

  event1.types.joystick.which = event2.types.joystick.which = which;

  event1.types.joystick.button = positive;
  event2.types.joystick.button = negative;

  event1.type = position > 0 ? INPUT_EVENT_JOYSTICK_PRESS :
                               INPUT_EVENT_JOYSTICK_RELEASE;
  event2.type = position < 0 ? INPUT_EVENT_JOYSTICK_PRESS :
                               INPUT_EVENT_JOYSTICK_RELEASE;

  input_event( &event1 );
  input_event( &event2 );
}

static void
joy_handler( int ev, int number, char value, int which )
{
  input_event_t event;

  switch ( ev ) {
  case JOY_EVENTAXIS:
    if( number == 0 )
      do_axis( which, value, INPUT_JOYSTICK_LEFT, INPUT_JOYSTICK_RIGHT );
    else if( number == 1 )
      do_axis( which, value, INPUT_JOYSTICK_UP, INPUT_JOYSTICK_DOWN );
    break;
  case JOY_EVENTBUTTONDOWN:
  case JOY_EVENTBUTTONUP:
    if( number >= buttons[which] ) return;
    event.types.joystick.which = which;
    event.types.joystick.button = INPUT_JOYSTICK_FIRE_1 + number;
    event.type = ( ev == JOY_EVENTBUTTONDOWN )
               ? INPUT_EVENT_JOYSTICK_PRESS
               : INPUT_EVENT_JOYSTICK_RELEASE;
    input_event( &event );
    break;
  default:
    break;
  }
}

void
ui_joystick_poll( void )
{
}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */
