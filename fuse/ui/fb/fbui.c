/* fbui.c: Routines for dealing with the linux fbdev user interface
   Copyright (c) 2000-2004 Philip Kendall, Matan Ziv-Av, Witold Filipczyk
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

#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "fbdisplay.h"
#include "fbkeyboard.h"
#include "fbmouse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

static void end_handler( int signo );
static void fb_end( void );

int
ui_init( int *argc, char ***argv )
{
  struct sigaction handler;
  int error;

  if( ui_widget_init() ) return 1;

  error = atexit( fb_end );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "ui_init: couldn't set atexit function" );
    return 1;
  }
  
  handler.sa_handler = end_handler;

  error = sigaction( SIGTERM, &handler, NULL );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "ui_init: couldn't set SIGTERM handler: %s",
	      strerror( errno ) );
    return 1;
  }

  error = sigaction( SIGHUP, &handler, NULL );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "ui_init: couldn't set SIGHUP handler: %s",
	      strerror( errno ) );
    return 1;
  }

  error = fbkeyboard_init();
  if( error ) return error;

  error = fbmouse_init();
  if( error ) return error;

  error = fbdisplay_init();
  if( error ) return error;

  return 0;
}

int ui_event( void )
{
  keyboard_update();
  mouse_update();
  return 0;
}

int ui_end( void )
{
  /* Cleanup handled by atexit function */
  int error;
  
  error = fbkeyboard_end(); if( error ) return error;
  error = fbdisplay_end(); if( error ) return error;

  ui_widget_end();

  return 0;
}

static void
end_handler( int signo )
{
  fb_end();
}

static void
fb_end( void )
{
  fbkeyboard_end();
  uidisplay_end();
}
