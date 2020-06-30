/* sdlui.c: Routines for dealing with the SDL user interface
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier
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

#include <stdio.h>
#include <SDL.h>

#include "display.h"
#include "fuse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "settings.h"
#include "sdldisplay.h"
#include "sdljoystick.h"
#include "sdlkeyboard.h"
#include "ui/scaler/scaler.h"
#include "menu.h"

static void
atexit_proc( void )
{ 
  SDL_ShowCursor( SDL_ENABLE );
  SDL_Quit();
}

int 
ui_init( int *argc, char ***argv )
{
  int error;

  if( ui_widget_init() ) return 1;

/* Comment out to Work around a bug in OS X 10.1 related to OpenGL in windowed
   mode */
  atexit(atexit_proc);

  error = SDL_Init( SDL_INIT_VIDEO );
  if ( error )
    return error;

#ifndef __MORPHOS__
  SDL_EnableUNICODE( 1 );
#endif				/* #ifndef __MORPHOS__ */

  sdlkeyboard_init();

  ui_mouse_present = 1;

  return 0;
}

int 
ui_event( void )
{
  SDL_Event event;

  while ( SDL_PollEvent( &event ) ) {
    switch ( event.type ) {
    case SDL_KEYDOWN:
      sdlkeyboard_keypress( &(event.key) );
      break;
    case SDL_KEYUP:
      sdlkeyboard_keyrelease( &(event.key) );
      break;

    case SDL_MOUSEBUTTONDOWN:
      ui_mouse_button( event.button.button, 1 );
      break;
    case SDL_MOUSEBUTTONUP:
      ui_mouse_button( event.button.button, 0 );
      break;
    case SDL_MOUSEMOTION:
      if( ui_mouse_grabbed ) {
        ui_mouse_motion( event.motion.x - 128, event.motion.y - 128 );
        if( event.motion.x != 128 || event.motion.y != 128 )
          SDL_WarpMouse( 128, 128 );
      }	
      break;

#if defined USE_JOYSTICK && !defined HAVE_JSW_H

    case SDL_JOYBUTTONDOWN:
      sdljoystick_buttonpress( &(event.jbutton) );
      break;
    case SDL_JOYBUTTONUP:
      sdljoystick_buttonrelease( &(event.jbutton) );
      break;
    case SDL_JOYAXISMOTION:
      sdljoystick_axismove( &(event.jaxis) );
      break;
    case SDL_JOYHATMOTION:
      sdljoystick_hatmove( &(event.jhat) );
      break;

#endif			/* if defined USE_JOYSTICK && !defined HAVE_JSW_H */

    case SDL_QUIT:
      fuse_emulation_pause();
      menu_file_exit(0);
      fuse_emulation_unpause();
      break;
    case SDL_VIDEOEXPOSE:
      display_refresh_all();
      break;
    case SDL_ACTIVEEVENT:
      if( event.active.state & SDL_APPINPUTFOCUS ) {
	if( event.active.gain ) ui_mouse_resume(); else ui_mouse_suspend();
      }
      break;
    default:
      break;
    }
  }

  return 0;
}

int 
ui_end( void )
{
  int error;

  error = uidisplay_end();
  if ( error )
    return error;

  sdlkeyboard_end();

  SDL_Quit();

  ui_widget_end();

  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  char buffer[15];
  const char fuse[] = "Fuse";

  snprintf( buffer, 15, "%s - %3.0f%%", fuse, speed );

  /* FIXME: Icon caption should be snapshot name? */
  SDL_WM_SetCaption( buffer, fuse );

  return 0;
}

int
ui_mouse_grab( int startup )
{
  if( settings_current.full_screen ) {
    SDL_WarpMouse( 128, 128 );
    return 1;
  }
  if( startup ) return 0;

  switch( SDL_WM_GrabInput( SDL_GRAB_ON ) ) {
  case SDL_GRAB_ON:
  case SDL_GRAB_FULLSCREEN:
    SDL_ShowCursor( SDL_DISABLE );
    SDL_WarpMouse( 128, 128 );
    return 1;
  default:
    ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
    return 0;
  }
}

int
ui_mouse_release( int suspend )
{
  if( settings_current.full_screen ) return !suspend;

  SDL_WM_GrabInput( SDL_GRAB_OFF );
  SDL_ShowCursor( SDL_ENABLE );
  return 0;
}
