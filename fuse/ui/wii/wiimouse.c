/* wiimouse.c: routines for dealing with the Wiimote as a mouse
   Copyright (c) 2008-2009 Bjoern Giesler, Marek Januszewski

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

#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define GEKKO

#include <wiiuse/wpad.h>

#include "fuse.h"
#include "keyboard.h"
#include "ui/ui.h"
#include "ui/wii/wiidisplay.h"

static WPADData paddata;
static WPADData oldpaddata;

/* The below is to remember last nunchuck position when navigating the menus.
   It is used so the user needs to release nunchuck stick and then press it
   again to go to the next menu item (holding the direction will to next menu
   item just once, no matter how long the direction is being held). */
#define LEFT  1
#define RIGHT 2
#define UP    3
#define DOWN  4
static u8 last_nunchuck[ 2 ]; /* for 2 controllers */

int
wiimouse_init( void )
{
  WPAD_Init();
  WPAD_SetDataFormat( WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR );
  WPAD_SetIdleTimeout( 60 );

  memset( &paddata, 0, sizeof(paddata) );
  memset( &oldpaddata, 0, sizeof(oldpaddata) );
  ui_mouse_present = 1;
  return 0;
}

int
wiimouse_end( void )
{
  return 0;
}

void
wiimouse_get_position( int *x, int *y )
{
  if( paddata.ir.state == 0 ) *x = *y = -1;

  *x = paddata.ir.x;
  *y = paddata.ir.y;
}

void
mouse_update( void )
{
  /* do this ONLY here. wiijoystick depends on it as well, but
     mouse_update is called regardless of whether the emulation is
     running or not, ui_joystick_poll only if running. So we do this
     here and risk lagging 1 frame behind on the joystick
     FIXME: A function that does this only once depending on the
     current frame counter would be better */

  int ctrlr; /* Which controller */
  u32 wm_down; /* Wii Remote buttons that are down */
  WPADData *wpad;
  joystick_t js;

  WPAD_ScanPads();

#define POST_KEYPRESS(pressed) do {		\
    input_event_t fuse_event; \
    fuse_event.type = INPUT_EVENT_KEYPRESS; \
    fuse_event.types.key.native_key = pressed; \
    fuse_event.types.key.spectrum_key = pressed; \
    input_event(&fuse_event); \
  } while(0)
  
#define POST_KEYRELEASE(pressed) do {	    \
    input_event_t fuse_event; \
    fuse_event.type = INPUT_EVENT_KEYRELEASE; \
    fuse_event.types.key.native_key = pressed; \
    fuse_event.types.key.spectrum_key = pressed; \
    input_event(&fuse_event); \
  } while(0)

  for( ctrlr = 0; ctrlr < 2; ctrlr++ ) {

    wpad = WPAD_Data( ctrlr );
    if( !wpad ) continue;

    wm_down = wpad->btns_d;

    /* we don't bother with key releases here; this is only for entering
       and/or using the menu, which seems to at best disregard
       keyreleases and at worst react badly to them. */
  
    if(fuse_emulation_paused) {
      if( wm_down & WPAD_BUTTON_DOWN )
        POST_KEYPRESS( INPUT_JOYSTICK_RIGHT );
      else if( wm_down & WPAD_BUTTON_UP )
        POST_KEYPRESS( INPUT_JOYSTICK_LEFT );
      else if( wm_down & WPAD_BUTTON_LEFT )
        POST_KEYPRESS( INPUT_JOYSTICK_DOWN );
      else if( wm_down & WPAD_BUTTON_RIGHT )
        POST_KEYPRESS( INPUT_JOYSTICK_UP );
      else if( ( wm_down & WPAD_BUTTON_A ) || ( wm_down & WPAD_BUTTON_2 ) )
        POST_KEYPRESS( INPUT_JOYSTICK_FIRE_1 );
      else if( ( wm_down & WPAD_BUTTON_B ) || ( wm_down & WPAD_BUTTON_1 )
           || ( wm_down & WPAD_BUTTON_HOME ) )
        POST_KEYPRESS( INPUT_JOYSTICK_FIRE_2 );
  
      if( wpad->exp.type == EXP_NUNCHUK ) {
  
        js = wpad->exp.nunchuk.js;

        if( js.mag >= 0.5 ) {
          /* left */
          if( js.ang >= 270-45 && js.ang <= 270+45 ) {
            if( last_nunchuck[ ctrlr ] != LEFT ) {
              POST_KEYPRESS( INPUT_JOYSTICK_LEFT );
              last_nunchuck[ ctrlr ] = LEFT;
            }
          }
          /* right */
          else if( js.ang >= 90-45 && js.ang <= 90+45 ) {
            if( last_nunchuck[ ctrlr ] != RIGHT ) {
              POST_KEYPRESS( INPUT_JOYSTICK_RIGHT );
              last_nunchuck[ ctrlr ] = RIGHT;
            }
          }
          /* up */
          else if( js.ang >= 360-45 || js.ang <= 45 ) {
            if( last_nunchuck[ ctrlr ] != UP ) {
              POST_KEYPRESS( INPUT_JOYSTICK_UP );
              last_nunchuck[ ctrlr ] = UP;
            }
          }
          /* down */
          else if( js.ang >= 180-45 && js.ang <= 180+45 ) {
            if( last_nunchuck[ ctrlr ] != DOWN ) {
              POST_KEYPRESS( INPUT_JOYSTICK_DOWN );
              last_nunchuck[ ctrlr ] = DOWN;
            }
          }
        }
        else
          last_nunchuck[ ctrlr ] = 0;
      }
    } else {
      if( wm_down & WPAD_BUTTON_HOME )
        POST_KEYPRESS( INPUT_KEY_F1 );
    }
  }

  WPAD_ReadEvent( 0, &paddata );

  if( paddata.ir.state == 0 )
    wiidisplay_showmouse( -1, -1 );
  else
    wiidisplay_showmouse( paddata.ir.x/560.0f, paddata.ir.y/420.0f );  
}

int
ui_mouse_grab( int startup GCC_UNUSED )
{
  return 1;
}

int
ui_mouse_release( int suspend )
{
  return !suspend;
}
