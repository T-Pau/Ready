/* gtkmouse.c: GTK+ routines for emulating Spectrum mice
   Copyright (c) 2004 Darren Salt
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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gtkinternals.h"
#include "ui/ui.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

/* For XWarpPointer *only* - see below */
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

static GdkCursor *nullpointer = NULL;

/* The widget we base our events, grabs, warping etc on */
static GtkWidget *mouse_widget = NULL;

static void
gtkmouse_reset_pointer( void )
{
  /* Ugh. GDK doesn't have its own move-pointer function :-|

     The logic here is a bit hairy:

     * On GTK+ 2.x, we warp relative to the drawing area
     * On GTK+ 3.x on X11, we warp relative to the top-level window
     * On GTK+ 3.x on Wayland, we don't warp at all because it causes a
       segfault (see bug #435)
   */

  GdkWindow *window;

#ifdef GDK_WINDOWING_WAYLAND
  GdkDisplay *display = gdk_display_get_default();
  if( GDK_IS_WAYLAND_DISPLAY( display ) ) return;
#endif                /* #ifdef GDK_WINDOWING_WAYLAND */

  window = gtk_widget_get_window( mouse_widget );
  XWarpPointer( GDK_WINDOW_XDISPLAY( window ), None, 
                GDK_WINDOW_XID( window ), 0, 0, 0, 0, 128, 128 );
}

static gboolean
motion_event( GtkWidget *widget GCC_UNUSED, GdkEventMotion *event,
              gpointer data GCC_UNUSED )
{
  if( !ui_mouse_grabbed ) return TRUE;

  if( event->x != 128 || event->y != 128 )
    gtkmouse_reset_pointer();
  ui_mouse_motion( event->x - 128, event->y - 128 );
  return TRUE;
}

static gboolean
button_event( GtkWidget *widget GCC_UNUSED, GdkEventButton *event,
	      gpointer data GCC_UNUSED )
{
  if( event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS
      || event->type == GDK_3BUTTON_PRESS )
    ui_mouse_button( event->button, 1 );
  else
    ui_mouse_button( event->button, 0 );
  return TRUE;
}

void
gtkmouse_init( void )
{
#if GTK_CHECK_VERSION( 3, 0, 0 )
  mouse_widget = gtkui_window;
#else                 /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */
  mouse_widget = gtkui_drawing_area;
#endif                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */

  gtk_widget_add_events( GTK_WIDGET( mouse_widget ),
    GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK );
  g_signal_connect( G_OBJECT( mouse_widget ), "motion-notify-event",
		    G_CALLBACK( motion_event ), NULL );
  g_signal_connect( G_OBJECT( mouse_widget ), "button-press-event",
		    G_CALLBACK( button_event ), NULL );
  g_signal_connect( G_OBJECT( mouse_widget ), "button-release-event",
		    G_CALLBACK( button_event ), NULL );
}

int
ui_mouse_grab( int startup )
{
  GdkWindow *window;
  GdkGrabStatus status;

  if( startup ) return 0;

  window = gtk_widget_get_window( mouse_widget );

#if !GTK_CHECK_VERSION( 3, 0, 0 )

  if( !nullpointer ) {
    nullpointer = gdk_cursor_new( GDK_BLANK_CURSOR );
  }

  status = gdk_pointer_grab( window, FALSE,
                             GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK,
                             window, nullpointer, GDK_CURRENT_TIME );

#else

  GdkDisplay *display;
  GdkSeat *seat;

  display = gdk_window_get_display( window );

  if( !nullpointer ) {
    nullpointer = gdk_cursor_new_for_display( display, GDK_BLANK_CURSOR );
  }

  seat = gdk_display_get_default_seat( display );
  status = gdk_seat_grab( seat, window, GDK_SEAT_CAPABILITY_ALL_POINTING,
                          FALSE, nullpointer, NULL, NULL, NULL );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

  if( status == GDK_GRAB_SUCCESS ) {
    gtkmouse_reset_pointer();
    ui_statusbar_update( UI_STATUSBAR_ITEM_MOUSE, UI_STATUSBAR_STATE_ACTIVE );
    return 1;
  }

  ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
  return 0;
}

int
ui_mouse_release( int suspend GCC_UNUSED )
{
#if !GTK_CHECK_VERSION( 3, 0, 0 )

  gdk_pointer_ungrab( GDK_CURRENT_TIME );

#else

  GdkDisplay *display;
  GdkSeat *seat;

  display = gtk_widget_get_display( mouse_widget );
  seat = gdk_display_get_default_seat( display );
  gdk_seat_ungrab( seat );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */ 

  ui_statusbar_update( UI_STATUSBAR_ITEM_MOUSE, UI_STATUSBAR_STATE_INACTIVE );
  return 0;
}
