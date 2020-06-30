/* gtkkeyboard.c: GTK+ routines for dealing with the keyboard
   Copyright (c) 2000-2008 Philip Kendall, Russell Marks

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

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "input.h"
#include "keyboard.h"
#include "ui/ui.h"

/* Given a hardware keycode, return the keyval which would have been returned if
   the key were unshifted */
static guint
unshift_keysym( guint keycode, gint group )
{
  GdkKeymapKey *maps;
  guint *keyvals, i, r = GDK_KEY_VoidSymbol, r2 = GDK_KEY_VoidSymbol;
  gint count;

  gdk_keymap_get_entries_for_keycode( gdk_keymap_get_default(), keycode,
                                      &maps, &keyvals, &count );

  for( i = 0; i < count; i++ ) {
    if( maps[i].group == group && maps[i].level == 0 ) {
      r = keyvals[i];
      break;
    }
    if( maps[i].group == 0 && maps[i].level == 0 ) {
      r2 = keyvals[i];
    }
  }
  if (i == count)
	  r = r2;

  g_free( keyvals ); g_free( maps );

  return r;
}

static void
get_keysyms( input_event_t *event, guint keycode, guint keysym, gint group )
{
  guint unshifted;

  /* The GTK+ UI doesn't actually use the native keysym for anything,
     but we may as well set it up anyway as we've got it */
  event->types.key.native_key = keysyms_remap( keysym );

  unshifted = unshift_keysym( keycode, group );
  event->types.key.spectrum_key = keysyms_remap( unshifted );
}

int
gtkkeyboard_keypress( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		      gpointer data GCC_UNUSED )
{
  input_event_t fuse_event;

  if( event->key.keyval == GDK_KEY_F1 && event->key.state == 0 )
    ui_mouse_suspend();

  fuse_event.type = INPUT_EVENT_KEYPRESS;
  get_keysyms( &fuse_event, event->key.hardware_keycode, event->key.keyval, event->key.group );

  return input_event( &fuse_event );

  /* FIXME: handle F1 to deal with the pop-up menu */
}

int
gtkkeyboard_keyrelease( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
			gpointer data GCC_UNUSED )
{
  input_event_t fuse_event;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  get_keysyms( &fuse_event, event->key.hardware_keycode, event->key.keyval, event->key.group );

  return input_event( &fuse_event );
}
