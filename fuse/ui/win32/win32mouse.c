/* win32mouse.c: Win32 routines for emulating Spectrum mice
   Copyright (c) 2008 Marek Januszewski

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

#include <windows.h>
#include <windowsx.h> /* for GET_X_LPARAM and GET_Y_LPARAM */

#include "ui/ui.h"
#include "win32internals.h"

static void
win32mouse_reset_pointer( void )
{
  RECT rect;
  POINT point;
  
  GetClientRect( fuse_hWnd, &rect );

  point.x = rect.left + 128;
  point.y = rect.top + 128;

  ClientToScreen( fuse_hWnd, &point ); 
  SetCursorPos( point.x, point.y ); 
}

void
win32mouse_position( LPARAM lParam )
{
  int x,y;
  
  if( !ui_mouse_grabbed ) return;

  x = GET_X_LPARAM( lParam ); 
  y = GET_Y_LPARAM( lParam ); 

  if( x != 128 || y != 128 )
    win32mouse_reset_pointer();
  ui_mouse_motion( x - 128, y - 128 );
  return;
}

void
win32mouse_button( int button, int down )
/* button is: 1 - left, 2 - middle, 3 - right, down is: 1 - down, 0 - up */
{
  ui_mouse_button( button, down );
  return;
}

int
ui_mouse_grab( int startup )
{
  if( startup ) return 0;

  SetCursor( NULL );
  SetCapture( fuse_hWnd );
  ui_statusbar_update( UI_STATUSBAR_ITEM_MOUSE, UI_STATUSBAR_STATE_ACTIVE );
  return 1;

/* doesn't seem like SetCapture can return an error */
/*
  ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
  return 0;
*/
}

int
ui_mouse_release( int suspend )
{
  ReleaseCapture();
  SetCursor( LoadCursor( NULL, IDC_ARROW ) );
  ui_statusbar_update( UI_STATUSBAR_ITEM_MOUSE, UI_STATUSBAR_STATE_INACTIVE );
  return 0;
}
