/* svgaui.c: Routines for dealing with the svgalib user interface
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Russell Marks

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

#include <vga.h>

#if defined USE_JOYSTICK && !defined HAVE_JSW_H
#include <vgajoystick.h>
#endif

#include <vgakeyboard.h>
#include <vgamouse.h>

#include "display.h"
#include "fuse.h"
#include "svgadisplay.h"
#include "svgakeyboard.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

static int oldbutton = 0, oldx = 0, oldy = 0;

int ui_init(int *argc, char ***argv)
{
  int error;

  if( ui_widget_init() ) return 1;

  error = svgadisplay_init();
  if(error) return error;

  error = svgakeyboard_init();
  if(error) return error;

  vga_setmousesupport( 1 );
  mouse_setxrange( 0, 255);
  mouse_setyrange( 0, 255);
  mouse_setscale( 64 );
  mouse_setwrap( MOUSE_WRAPX | MOUSE_WRAPY );
  mouse_setposition( 128, 128 );

  ui_mouse_present = 1;

  return 0;
}

int
ui_event( void )
{
  int x, y, b, bd;

  keyboard_update();
  mouse_update();
#if defined USE_JOYSTICK && !defined HAVE_JSW_H
  joystick_update();
#endif

  x = mouse_getx();
  y = mouse_gety();
  b = mouse_getbutton();

  bd = b ^ oldbutton;
  if( bd & MOUSE_LEFTBUTTON ) ui_mouse_button( 1, b & MOUSE_LEFTBUTTON );
  if( bd & MOUSE_MIDDLEBUTTON ) ui_mouse_button( 2, b & MOUSE_MIDDLEBUTTON );
  if( bd & MOUSE_RIGHTBUTTON ) ui_mouse_button( 3, b & MOUSE_RIGHTBUTTON );
  oldbutton = b;

  if( x != oldx || y != oldy ) {
    ui_mouse_motion( x - oldx, y - oldy );
    oldx = x; oldy = y;
  }

  return 0;
}

int ui_end(void)
{
  int error;

  error = svgakeyboard_end();
  if(error) return error;

  error = svgadisplay_end();
  if(error) return error;

  ui_widget_end();

  return 0;
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
