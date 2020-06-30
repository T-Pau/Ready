/* xui.c: Routines for dealing with the Xlib user interface
   Copyright (c) 2000-2015 Philip Kendall

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "menu.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "xdisplay.h"
#include "xkeyboard.h"
#include "xui.h"

Display *display;		/* Which display are we connected to */
int xui_screenNum;		/* Which screen are we using on our
				   X server? */
Window xui_mainWindow;		/* Window ID for the main Fuse window */

Cursor nullpointer;

static Atom delete_window_atom;

int
ui_init( int *argc, char ***argv )
{
  char *displayName=NULL;	/* Use default display */
  XWMHints *wmHints;
  XSizeHints *sizeHints;
  XClassHint *classHint;
  char *windowNameList=(char *)"Fuse",*iconNameList=(char *)"Fuse";
  XTextProperty windowName, iconName;
  unsigned long windowFlags;
  XSetWindowAttributes windowAttributes;

  /* Allocate memory for various things */

  if( ui_widget_init() ) return 1;

  if(!(wmHints = XAllocWMHints())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(!(sizeHints = XAllocSizeHints())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(!(classHint = XAllocClassHint())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(XStringListToTextProperty(&windowNameList,1,&windowName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for windowName failed\n",
	    fuse_progname);
    return 1;
  }

  if(XStringListToTextProperty(&iconNameList,1,&iconName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for iconName failed\n",
	    fuse_progname);
    return 1;
  }

  /* Open a connection to the X server */

  if ( ( display=XOpenDisplay(displayName)) == NULL ) {
    fprintf(stderr,"%s: cannot connect to X server %s\n", fuse_progname,
	    XDisplayName(displayName));
    return 1;
  }

  /* Set up our error handler */
  xerror_expecting = xerror_error = 0;
  XSetErrorHandler( xerror_handler );

  xui_screenNum = DefaultScreen(display);

  /* Create the main window */

  xui_mainWindow = XCreateSimpleWindow(
    display, RootWindow( display, xui_screenNum ), 0, 0,
    DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT, 0,
    BlackPixel( display, xui_screenNum ), WhitePixel( display, xui_screenNum )
  );

  /* Set standard window properties */

  sizeHints->flags = PBaseSize | PResizeInc | PMaxSize | PMinSize;

  sizeHints->base_width = 0;
  sizeHints->base_height = 0;

  sizeHints->min_width    =     DISPLAY_ASPECT_WIDTH;
  sizeHints->min_height   =     DISPLAY_SCREEN_HEIGHT;
  sizeHints->width_inc    =     DISPLAY_ASPECT_WIDTH;
  sizeHints->height_inc   =     DISPLAY_SCREEN_HEIGHT;
  sizeHints->max_width    = 3 * DISPLAY_ASPECT_WIDTH;
  sizeHints->max_height   = 3 * DISPLAY_SCREEN_HEIGHT;

  if( settings_current.aspect_hint ) {
    sizeHints->flags |= PAspect;
    sizeHints->min_aspect.x = 4;
    sizeHints->min_aspect.y = 3;
    sizeHints->max_aspect.x = 4;
    sizeHints->max_aspect.y = 3;
  }

  wmHints->flags=StateHint | InputHint;
  wmHints->initial_state=NormalState;
  wmHints->input=True;

  classHint->res_name=(char *)fuse_progname;
  classHint->res_class=(char *)"Fuse";

  XSetWMProperties(display, xui_mainWindow, &windowName, &iconName,
		   *argv, *argc, sizeHints, wmHints, classHint);

  XFree( windowName.value );
  XFree( iconName.value );
  XFree( sizeHints );
  XFree( wmHints );
  XFree( classHint );

  /* Ask the server to use its backing store for this window */

  windowFlags=CWBackingStore;
  windowAttributes.backing_store=WhenMapped;

  XChangeWindowAttributes(display, xui_mainWindow, windowFlags,
			  &windowAttributes);

  /* Select which types of event we want to receive */

  XSelectInput(display, xui_mainWindow, ExposureMask | KeyPressMask |
	       KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
	       StructureNotifyMask | FocusChangeMask | PointerMotionMask );

  {
    static XColor dummy = { 0, 0, 0, 0, 4, 0 };
    XGCValues xgc;
    GC gc;

    Pixmap mask =
      XCreatePixmap( display, RootWindow( display, xui_screenNum ), 1, 1, 1 );

    xgc.function = GXclear;
    gc = XCreateGC( display, mask, GCFunction, &xgc );
    XFillRectangle( display, mask, gc, 0, 0, 1, 1 );
    nullpointer = XCreatePixmapCursor( display, mask,mask, &dummy,&dummy, 0,0 );
    XFreePixmap( display, mask );
    XFreeGC( display, gc );
  }

  /* Ask to be notified of window close requests */

  delete_window_atom = XInternAtom( display, "WM_DELETE_WINDOW", 0 );
  XSetWMProtocols( display, xui_mainWindow, &delete_window_atom, 1 );

  if( xdisplay_init() ) return 1;

  /* And finally display the window */
  XMapWindow(display,xui_mainWindow);

  ui_mouse_present = 1;

  return 0;
}

int ui_event(void)
{
  XEvent event;

  XFlush( display );
  while( XEventsQueued( display, QueuedAlready ) ) {
    XNextEvent( display, &event );

    switch(event.type) {
    case ConfigureNotify:
      xdisplay_configure_notify(event.xconfigure.width,
				event.xconfigure.height);
      break;
    case Expose:
      xdisplay_area( event.xexpose.x, event.xexpose.y,
		     event.xexpose.width, event.xexpose.height );
      break;
    case ButtonPress:
      ui_mouse_button( event.xbutton.button, 1 );
      break;
    case ButtonRelease:
      ui_mouse_button( event.xbutton.button, 0 );
      break;
    case MotionNotify:
      if( ui_mouse_grabbed ) {
        ui_mouse_motion( event.xmotion.x - 128, event.xmotion.y - 128 );
        if( event.xmotion.x != 128 || event.xmotion.y != 128 )
          XWarpPointer( display, None, xui_mainWindow, 0, 0, 0, 0, 128, 128 );
      }
      break;
    case FocusOut:
      keyboard_release_all();
      ui_mouse_suspend();
      break;
    case FocusIn:
      ui_mouse_resume();
      break;
    case KeyPress:
      xkeyboard_keypress(&(event.xkey));
      break;
    case KeyRelease:
      xkeyboard_keyrelease(&(event.xkey));
      break;
    case ClientMessage:
      if( event.xclient.format == 32 &&
          event.xclient.data.l[0] == delete_window_atom ) {
        fuse_emulation_pause();
        menu_file_exit(0);
        fuse_emulation_unpause();
      }
      break;
    }
  }
  return 0;
}

int ui_end(void)
{
  int error;

  /* Don't display the window whilst doing all this */
  XUnmapWindow(display,xui_mainWindow);

  /* Tidy up the low level stuff */
  error = xdisplay_end(); if( error ) return error;

  /* Now free up the window itself */
  XDestroyWindow(display,xui_mainWindow);

  /* And disconnect from the X server */
  XCloseDisplay(display);

  ui_widget_end();

  return 0;
}

int
ui_mouse_grab( int startup )
{
  if( startup ) return 0;

  switch( XGrabPointer( display, xui_mainWindow, True,
			ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync, xui_mainWindow,
			nullpointer, CurrentTime )
	) {
  case GrabSuccess:
  case GrabNotViewable:
    XWarpPointer( display, None, xui_mainWindow, 0, 0, 0, 0, 128, 128 );
    return 1;
  default:
    ui_error( UI_ERROR_WARNING, "Mouse grab failed" );
    return 0;
  }
}

int
ui_mouse_release( int suspend GCC_UNUSED )
{
  XUngrabPointer( display, CurrentTime );
  return 0;
}
