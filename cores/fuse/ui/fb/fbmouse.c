/* fbmouse.c: Linux mouse handling code (requires kernel input layer)
   Copyright (c) 2004 Darren Salt

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include "fuse.h"
#include "fbmouse.h"
#include "ui/ui.h"

#include <dlfcn.h>

#ifdef HAVE_GPM_H
#include <gpm.h>
static void *libgpm = NULL;
static typeof (Gpm_GetEvent) *gpm_getevent = 0;
static typeof (gpm_fd) gpmfd = 0;
static const typeof (gpm_flag) zero = 0;
static typeof (&zero) gpmflag = &zero;
#endif

static int mouse_fd = -1;
static enum { PS2, ImPS2, ExPS2 } mouse_mode = PS2;
static int packet_size = 3;

static int try_open( const char * );
static int try_mouse_mode( unsigned char, unsigned char );

int
fbmouse_init(void)
{
  int i;
#ifdef HAVE_GPM_H
  static Gpm_Connect conn = { ~0/*GPM_MOVE | GPM_DOWN | GPM_UP*/, 0, 0, ~0 };

  libgpm = dlopen( "libgpm.so", RTLD_LAZY );

  if( libgpm ) {
    typeof (Gpm_Open) *gpm_open = dlsym( libgpm, "Gpm_Open" );
    i = 1;
    if( gpm_open( &conn, 0 ) != -1 ) {
      gpmfd = *( typeof( &gpm_fd ) ) dlsym( libgpm, "gpm_fd" );
      gpmflag = ( typeof( &gpm_flag ) ) dlsym( libgpm, "gpm_flag" );
      gpm_getevent = dlsym( libgpm, "Gpm_GetEvent" );
      ui_mouse_present = 1;
      return 0;
    }
  } else
    fprintf( stderr, "%s: couldn't init libgpm: %s\n", fuse_progname,
	     dlerror() );
#endif

  /* No libgpm support, no libgpm or gpm is not running - use our own code */
  if( try_open ("/dev/input/mice") &&
      try_open ("/dev/mouse") &&
      try_open ("/dev/psaux") ) {
    if( errno == ENOENT || errno == ENODEV || errno == EACCES ) {
      fprintf( stderr, "%s: warning: no mouse, or not accessible\n",
	       fuse_progname );
      return 0;
    }
    fprintf( stderr, "%s: couldn't open mouse device: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  /* Try to select plain PS/2 (just in case it's not already selected).
   * Ignore any error.
   */
  i = -1; /* account for big-endian */
  write( mouse_fd, &i, 1 );

  /* Try to set the mouse mode. We prefer ExPS/2 but will accept ImPS/2.
   * The main reason is so that the side buttons don't appear as duplicate
   * buttons 2 and 3, and so that the wheel is available should it be wanted
   * somewhere.
   */
  if( !try_mouse_mode( 100, 3 ) ) {
    packet_size = 4;
    mouse_mode = try_mouse_mode( 200, 4 ) ? ImPS2 : ExPS2;
  }

  /* *Now* we want non-blocking I/O... */
  i = 1;
  if( ioctl( mouse_fd, FIONBIO, &i ) )
    return fbmouse_end(); /* couldn't get it */

  ui_mouse_present = 1;
  return 0;
}

int
fbmouse_end(void)
{
#ifdef HAVE_GPM_H
  if( *gpmflag ) {
    typeof (Gpm_Close) *gpm_close = dlsym( libgpm, "Gpm_Close" );
    gpm_close();
  }
#endif
  if( mouse_fd != -1 ) {
    close( mouse_fd );
    mouse_fd = -1;
  }
  return 0;
}

#ifdef HAVE_GPM_H
static void mouse_update_gpm( void );
static void mouse_update_ps2( void );

void
mouse_update( void )
{
  if( *gpmflag )
    mouse_update_gpm();
  else
    mouse_update_ps2();
}

static void
mouse_update_gpm( void )
{
  Gpm_Event event;
  int db;
  static int oldbuttons = 0;

  if( gpmfd < 0 ) {
    static int t = 0;
    if (!t) fprintf (stderr, "gpm not there?\n");
    t = 1;
    return;
  }

  for (;;) {
    struct pollfd ufd = { gpmfd, POLLIN | POLLPRI };
    if( poll( &ufd, 1, 0 ) < 1 ) break;

    gpm_getevent( &event );

    if( !ui_mouse_grabbed ) continue;

    if( event.dx | event.dy ) ui_mouse_motion( event.dx, event.dy );

    db = event.buttons ^ oldbuttons;
    if( db ) {
      oldbuttons = event.buttons;
      if( db & GPM_B_LEFT  ) ui_mouse_button( 1, event.buttons & GPM_B_LEFT  );
      if( db & GPM_B_RIGHT ) ui_mouse_button( 3, event.buttons & GPM_B_RIGHT );
    }
  }
}
#endif

/* Simple PS/2 mouse handler.
 * Should cope with any similar 3- and 4-byte protocol which is a superset of
 * plain PS/2: e.g. ImPS/2, ExPS/2 but not PS2++.
 */
#ifdef HAVE_GPM_H
static void
mouse_update_ps2( void )
#else
void
mouse_update( void )
#endif
{
  static int btn_state = 0;
  unsigned char mousebuf[60]; /* max. events: 20 (PS/2) or 15 (other) */
  ssize_t xoff = 0, yoff = 0;
  int btn_changed = 0, btn_new = btn_state, btn_mod = btn_state;

  if( mouse_fd == -1 ) return;

  while( 1 ) {
    ssize_t available;
    const unsigned char *i;

    available = read( mouse_fd, &mousebuf, sizeof( mousebuf ) );
    if( available <= 0 ) break;

    if( !ui_mouse_grabbed ) continue;

    for( i = mousebuf; i < mousebuf + available; i += packet_size ) {
      btn_changed |= btn_mod ^= btn_new = i[0] & 7;
      btn_changed |= btn_mod ^= btn_new = i[0] & 7;
      xoff += i[1]; if( i[0] & 16 ) xoff -= 256;
      yoff += i[2]; if( i[0] & 32 ) yoff -= 256;
    }
  }

  btn_state = btn_new;
  if( btn_changed & 1 ) ui_mouse_button( 1, btn_new & 1 );
  if( btn_changed & 2 ) ui_mouse_button( 3, btn_new & 2 );

  if( xoff || yoff ) ui_mouse_motion( xoff, -yoff );
}

/* Try to open the specified device. Return 0 on success. */
static int
try_open( const char *dev )
{
  mouse_fd = open( dev, O_RDWR );
  if( mouse_fd == -1 && errno == EACCES )
    mouse_fd = open( dev, O_RDONLY );
  return mouse_fd == -1;
}

/* Try to set the mouse to the requested mode (100 = ImPS/2, 200 = ExPS/2).
 * Return 0 on success.
 */
static int
try_mouse_mode( unsigned char mode, unsigned char response )
{
  struct pollfd ufd = { mouse_fd, POLLIN | POLLPRI };
  int ret = 0;
  unsigned char buf[] = { 0xF3, 200, 0xF3, mode, 0xF3, 80, 0xF2 };
  if( write( mouse_fd, buf, sizeof( buf ) ) < sizeof( buf ) )
    return 1;
  while( poll( &ufd, 1, 10 ) == 1)
    ret = read( mouse_fd, buf, sizeof( buf ) );
  return ret < 1 || buf[ret - 1] != response;
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
