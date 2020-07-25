/* xkeyboard.c: X routines for dealing with the keyboard
   Copyright (c) 2000-2003 Philip Kendall

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
#include <X11/keysym.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui/ui.h"
#include "xkeyboard.h"

static void
get_keysyms( XKeyEvent *event, input_event_t *fuse_event )
{
  KeySym native, spectrum;

  /* Get keysyms taking into account Shift, Caps_Lock, Mode_switch modifiers */
  XLookupString( event, NULL, 0, &native, NULL );
  fuse_event->types.key.native_key = keysyms_remap( native );

  spectrum = XLookupKeysym( event, 0 );
  fuse_event->types.key.spectrum_key = keysyms_remap( spectrum );
}

void xkeyboard_keypress(XKeyEvent *event)
{
  input_event_t fuse_event;

  fuse_event.type = INPUT_EVENT_KEYPRESS;
  get_keysyms( event, &fuse_event );

  input_event( &fuse_event );
}

void xkeyboard_keyrelease(XKeyEvent *event)
{
  input_event_t fuse_event;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  get_keysyms( event, &fuse_event );

  input_event( &fuse_event );
}
