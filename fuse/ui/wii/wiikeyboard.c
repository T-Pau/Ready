/* wiikeyboard.c: routines for dealing with the Wii USB keyboard
   Copyright (c) 2008 Bjoern Giesler

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

#define _POSIX_THREADS

#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui/ui.h"

#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/ipc.h>

typedef struct {
  u32 msgtype;
  u32 unknown1;
  u8 modifiers;
  u8 unknown2;
  u8 keys[6];
} wii_kbd_event;

static s32 kbd = -1;

static lwp_t kbdthread;
static mutex_t kbdmutex;
#define QUEUELEN 100
static input_event_t queue[QUEUELEN];
static int queuepos = 0;
static input_event_t releasequeue[QUEUELEN];
static int releasequeuepos = 0;

static int
post_modifier( input_event_t* event,
	       u8 old, u8 new, u8 modifier, input_key spectrum_key )
{
  if( (old & modifier) && !(new & modifier) ) {
    event->type = INPUT_EVENT_KEYRELEASE;
    event->types.key.native_key = modifier;
    event->types.key.spectrum_key = spectrum_key;
    return 0;
  } else if( !(old & modifier) && (new & modifier) ) {
    event->type = INPUT_EVENT_KEYPRESS;
    event->types.key.native_key = modifier;
    event->types.key.spectrum_key = spectrum_key;
    return 0;
  }

  return 1;
}

static void*
kbdthread_fn( void *arg )
{
  wii_kbd_event event, oldevent;
  int i, j, found;

  memset( &event, 0, sizeof(event) );
  memset( &oldevent, 0, sizeof(oldevent) );

  while( kbd != -1 ) {
    IOS_Ioctl( kbd, 0, NULL, 0, &event, sizeof(event) );

    /* skip connect (0) and disconnect (1) events */
    if( event.msgtype != 2 ) continue;

    input_event_t fuse_event;

    LWP_MutexLock( kbdmutex );

#define POSTMODIFIER(wiikey, speckey) \
      if(queuepos < QUEUELEN) \
	if(post_modifier(queue + queuepos,		   \
			 oldevent.modifiers, event.modifiers,	\
			 wiikey, speckey) == 0) \
	  queuepos++

    POSTMODIFIER( 0x01, INPUT_KEY_Control_L );
    POSTMODIFIER( 0x02, INPUT_KEY_Shift_L );
    POSTMODIFIER( 0x04, INPUT_KEY_Alt_L );
    POSTMODIFIER( 0x08, INPUT_KEY_Super_L );
    POSTMODIFIER( 0x10, INPUT_KEY_Control_R );
    POSTMODIFIER( 0x20, INPUT_KEY_Shift_R );
    POSTMODIFIER( 0x40, INPUT_KEY_Alt_R );
    POSTMODIFIER( 0x80, INPUT_KEY_Super_R );

#undef POSTMODIFIER

    /* post keyreleases: old keys that have no new keys */
    fuse_event.type = INPUT_EVENT_KEYRELEASE;
    for(i=0; i<6; i++) {
      if(oldevent.keys[i] == 0) break;
      found = 0;
      for( j=0; j<6; j++ ) {
	if(event.keys[j] == oldevent.keys[i]) {
	  found = 1;
	  break;
	}
      }
      if( !found ) {
        fuse_event.types.key.spectrum_key = keysyms_remap(oldevent.keys[i]);
        fuse_event.types.key.native_key = fuse_event.types.key.spectrum_key;
	      
        if( queuepos >= QUEUELEN ) {
	  ui_error( UI_ERROR_WARNING, "%s: keyboard queue full", __func__ );
	  continue;
	}
        queue[queuepos++] = fuse_event;
        continue;
      }
    }

    fuse_event.type = INPUT_EVENT_KEYPRESS;
    for( i=0; i<6; i++ ) {
      if(event.keys[i] == 0) break;
      found = 0;
      for( j=0; j<6; j++ ) {
        if(oldevent.keys[j] == event.keys[i]) {
	  found = 1;
	  break;
	}
      }
      if( !found ) {
        fuse_event.types.key.spectrum_key = keysyms_remap( event.keys[i] );
        fuse_event.types.key.native_key = fuse_event.types.key.spectrum_key;

        if( queuepos >= QUEUELEN ) {
	  ui_error( UI_ERROR_WARNING, "%s: keyboard queue full", __func__ );
	  continue;
	}
        queue[queuepos++] = fuse_event;
      }
    }

    LWP_MutexUnlock( kbdmutex );
    oldevent = event;
  }

  return NULL;
}

int
wiikeyboard_init( void )
{
  kbd = IOS_Open( "/dev/usb/kbd", IPC_OPEN_RW );
  if( kbd < 0 ) return kbd;

  if( LWP_MutexInit(&kbdmutex, 0) != 0 ||
      LWP_CreateThread(&kbdthread, kbdthread_fn, NULL, NULL, 0, 80) != 0) {
    IOS_Close(kbd);
    return 1;
  }
  
  return 0;
}

int wiikeyboard_end(void)
{
  if( kbd >= 0 ) IOS_Close(kbd);
  kbd = -1;
  return 0;
}

void
keyboard_update( void )
{
  int i;

  for( i=0; i<releasequeuepos; i++ ) input_event(&(releasequeue[i]));
  releasequeuepos = 0;
  
  LWP_MutexLock( kbdmutex );
  for(i=0; i<queuepos; i++) {
    if( queue[i].type == INPUT_EVENT_KEYRELEASE )
      releasequeue[releasequeuepos++] = queue[i];
    else
      input_event( &(queue[i]) );
  }
  queuepos = 0;
  LWP_MutexUnlock( kbdmutex );
}
