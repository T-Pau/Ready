/* win32keyboard.c: routines for dealing with the Win32 keyboard
   Copyright (c) 2003 Marek Januszewski, Philip Kendall

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
#include <stdlib.h>
#include <windows.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "utils.h"
#include "win32internals.h"

/* Map OEM unshifted keys to Fuse input layer keysym */
extern const keysyms_map_t oem_keysyms_map[];

static GHashTable *oem_keysyms_hash;

static input_key
oem_keysyms_remap( WPARAM wParam )
{
  const input_key *ptr;
  unsigned int mapped_code;
  libspectrum_dword unshifted_char;

  mapped_code = MapVirtualKey( wParam, MAPVK_VK_TO_CHAR );
  if( !mapped_code ) return INPUT_KEY_NONE;

  unshifted_char = LOWORD( mapped_code );
  ptr = g_hash_table_lookup( oem_keysyms_hash, &unshifted_char );

  return ptr ? *ptr : INPUT_KEY_NONE;
}

void
win32keyboard_init( void )
{
  keysyms_map_t *ptr3;

  oem_keysyms_hash = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr3 = (keysyms_map_t *)oem_keysyms_map; ptr3->ui; ptr3++ )
    g_hash_table_insert( oem_keysyms_hash, &( ptr3->ui ), &( ptr3->fuse ) );
}

void
win32keyboard_end( void )
{
  g_hash_table_destroy( oem_keysyms_hash );
}

void
win32keyboard_keypress( WPARAM wParam, LPARAM lParam )
{
  input_key fuse_keysym;
  input_event_t fuse_event;

  switch( wParam ) {
  case VK_OEM_1:
  case VK_OEM_2:
  case VK_OEM_3:
  case VK_OEM_4:
  case VK_OEM_5:
  case VK_OEM_6:
  case VK_OEM_7:
  case VK_OEM_8:
  case VK_OEM_102:
  case VK_OEM_COMMA:
  case VK_OEM_MINUS:
  case VK_OEM_PERIOD:
  case VK_OEM_PLUS:
    fuse_keysym = oem_keysyms_remap( wParam );
    break;
  default:
    fuse_keysym = keysyms_remap( wParam );
    break;
  }

  if( fuse_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = INPUT_EVENT_KEYPRESS;
  fuse_event.types.key.native_key = fuse_keysym;
  fuse_event.types.key.spectrum_key = fuse_keysym;

  input_event( &fuse_event );
}

void
win32keyboard_keyrelease( WPARAM wParam, LPARAM lParam )
{
  input_key fuse_keysym;
  input_event_t fuse_event;

  switch( wParam ) {
  case VK_OEM_1:
  case VK_OEM_2:
  case VK_OEM_3:
  case VK_OEM_4:
  case VK_OEM_5:
  case VK_OEM_6:
  case VK_OEM_7:
  case VK_OEM_8:
  case VK_OEM_102:
  case VK_OEM_COMMA:
  case VK_OEM_MINUS:
  case VK_OEM_PERIOD:
  case VK_OEM_PLUS:
    fuse_keysym = oem_keysyms_remap( wParam );
    break;
  default:
    fuse_keysym = keysyms_remap( wParam );
    break;
  }

  if( fuse_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  fuse_event.types.key.native_key = fuse_keysym;
  fuse_event.types.key.spectrum_key = fuse_keysym;

  input_event( &fuse_event );
}
