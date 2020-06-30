/* wiijoystick.c: routines for dealing with the Wiimote as a joystick
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
#include "fuse.h"
#include "input.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

#define GEKKO
#include <wiiuse/wpad.h>

typedef enum {
  WII_JOYSTICK_SIDEWAYS_RH, /* sideways config (fire on the right) */
  WII_JOYSTICK_SIDEWAYS_LH, /* sideways config (fire on the left) */
  WII_JOYSTICK_NUNCHUK,     /* nunchuk for directions, A+B for fire */
  WII_JOYSTICK_CLASSIC,     /* classic controller */
} WiiJoystickConfig;

int
ui_joystick_init( void )
{
  return 4;
}

void
ui_joystick_end( void )
{
}

void
ui_joystick_poll( void )
{
  input_event_t fuse_event;
  int ctrlr; /* Which controller */
  u32 wm_down; /* Wii Remote buttons that are down */
  u32 wm_up; /* Wii Remote buttons that are up */
  ubyte nunchuck_down; /* Nunchuck buttons that are down */
  ubyte nunchuck_up; /* Nunchuck buttons that are up */
  WPADData *wpad;
  joystick_t js;

  if( fuse_emulation_paused ) return;

#define POST_JOYPRESS(number, pressed) do {	  \
    fuse_event.type = INPUT_EVENT_JOYSTICK_PRESS; \
    fuse_event.types.joystick.which = number; \
    fuse_event.types.joystick.button = pressed; \
    input_event(&fuse_event); \
  } while(0)
  
#define POST_JOYRELEASE(number, pressed) do {	    \
    fuse_event.type = INPUT_EVENT_JOYSTICK_RELEASE; \
    fuse_event.types.joystick.which = number; \
    fuse_event.types.joystick.button = pressed; \
    input_event(&fuse_event); \
  } while(0)

  for( ctrlr = 0; ctrlr < 2; ctrlr++ ) {

    wpad = WPAD_Data( ctrlr );
    if( !wpad ) continue;
  
    wm_down = wpad->btns_d;
    wm_up = wpad->btns_u;

    nunchuck_down = 0;
    nunchuck_up = 0;
    if( wpad->exp.type == EXP_NUNCHUK ) {
      nunchuck_down = wpad->exp.nunchuk.btns;
      nunchuck_up = wpad->exp.nunchuk.btns_released;
    }
  
    if( wm_down & WPAD_BUTTON_LEFT )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_DOWN );
    if( wm_down & WPAD_BUTTON_RIGHT )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_UP );
    if( wm_down & WPAD_BUTTON_UP )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_LEFT );
    if( wm_down & WPAD_BUTTON_DOWN )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_RIGHT );
  
    if( wm_down & WPAD_BUTTON_1 )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_1 );
    if( wm_down & WPAD_BUTTON_2 )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_2 );
  
    if( wm_down & WPAD_BUTTON_A )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_3 );
    if( wm_down & WPAD_BUTTON_B )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_4 );
    if( wm_down & WPAD_BUTTON_PLUS )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_5 );
    if( wm_down & WPAD_BUTTON_MINUS )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_6 );
    if( nunchuck_down & WPAD_NUNCHUK_BUTTON_Z )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_7 );
    if( nunchuck_down & WPAD_NUNCHUK_BUTTON_C )
      POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_FIRE_8 );
  
    if( wm_up & WPAD_BUTTON_LEFT )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_DOWN );
    if( wm_up & WPAD_BUTTON_RIGHT )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_UP );
    if( wm_up & WPAD_BUTTON_UP )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_LEFT );
    if( wm_up & WPAD_BUTTON_DOWN )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_RIGHT );
  
    if( wm_up & WPAD_BUTTON_1 )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_1 );
    if( wm_up & WPAD_BUTTON_2 )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_2 );
  
    if( wm_up & WPAD_BUTTON_A )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_3 );
    if( wm_up & WPAD_BUTTON_B )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_4 );
    if( wm_up & WPAD_BUTTON_PLUS )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_5 );
    if( wm_up & WPAD_BUTTON_MINUS )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_6 );
    if( nunchuck_up & WPAD_NUNCHUK_BUTTON_Z )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_7 );
    if( nunchuck_up & WPAD_NUNCHUK_BUTTON_C )
      POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_FIRE_8 );
      
    if( wpad->exp.type == EXP_NUNCHUK ) {

      js = wpad->exp.nunchuk.js;

      if( js.mag < 0.5 ) {
        /* stick tilted only halfway from the center - release all directions */
        POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_DOWN );
        POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_UP );
        POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_LEFT );
        POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_RIGHT );
      }
      else {
        int left_held = 0;
        int right_held = 0;
        int up_held = 0;
        int down_held = 0;
      	
        /* left */
        if( js.ang >= 270-60 && js.ang <= 270+60 ) {
          POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_LEFT );
          left_held = 1;
        }
        /* right */
        if( js.ang >= 90-60 && js.ang <= 90+60 ) {
          POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_RIGHT );
          right_held = 1;
        }
        /* up */
        if( js.ang >= 360-60 || js.ang <= 60 ) {
          POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_UP );
          up_held = 1;
        }
        /* down */
        if( js.ang >= 180-60 && js.ang <= 180+60 ) {
          POST_JOYPRESS( ctrlr, INPUT_JOYSTICK_DOWN );
          down_held = 1;
        }
        
        /* the below prevents an issue when user makes 180 deg circle with
           the nunchuck from for example left to right while mag > 0.5, in
           which case spectrum would get both left and right signal from
           joystick. The below is to prevent that. */
        if( !down_held ) POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_DOWN );
        if( !up_held ) POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_UP );
        if( !left_held ) POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_LEFT );
        if( !right_held ) POST_JOYRELEASE( ctrlr, INPUT_JOYSTICK_RIGHT );
      }
    }
  }
}
