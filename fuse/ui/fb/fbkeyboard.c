/* fbkeyboard.c: routines for dealing with the linux fbdev display
   Copyright (c) 2000-2004 Philip Kendall, Matan Ziv-Av, Darren Salt

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
#include <errno.h>
#include <linux/kd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
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

#include "input.h"

static struct termios old_ts;
static int old_kbmode = K_XLATE;
static int got_old_ts = 0;

int fbkeyboard_init(void)
{
  struct termios ts;
  struct stat st;
  int i = 1;

  /* First, set up the keyboard */
  if( fstat( STDIN_FILENO, &st ) ) {
    fprintf( stderr, "%s: couldn't stat stdin: %s\n", fuse_progname,
	     strerror( errno ) );
    return 1;
  }

  /* check for character special, major 4, minor 0..63 */
  if( !isatty(STDIN_FILENO) || !S_ISCHR(st.st_mode) ||
      ( st.st_rdev & ~63 ) != 0x0400 ) {
    fprintf( stderr, "%s: stdin isn't a local tty\n", fuse_progname );
    return 1;
  }

  tcgetattr( STDIN_FILENO, &old_ts );
  ioctl( STDIN_FILENO, KDGKBMODE, &old_kbmode );
  got_old_ts = 1;

  /* We need non-blocking semi-cooked keyboard input */
  if( ioctl( STDIN_FILENO, FIONBIO, &i ) ) {
    fprintf( stderr, "%s: can't set stdin nonblocking: %s\n", fuse_progname,
	     strerror( errno ) );
    return 1;
  }
  if( ioctl( STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW ) ) {
    fprintf( stderr, "%s: can't set keyboard into medium-raw mode: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  /* Add in a bit of voodoo... */
  ts = old_ts;
  ts.c_cc[VTIME] = 0;
  ts.c_cc[VMIN] = 1;
  ts.c_lflag &= ~(ICANON | ECHO | ISIG);
  ts.c_iflag = 0;
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &ts );
  tcsetpgrp( STDIN_FILENO, getpgrp() );

  return 0;
}

int fbkeyboard_end(void)
{
  int i = 0;

  ioctl( STDIN_FILENO, FIONBIO, &i );
  if( got_old_ts ) {
    tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_ts );
    ioctl( STDIN_FILENO, KDSKBMODE, old_kbmode );
  }

  return 0;
}

void
keyboard_update( void )
{
  unsigned char keybuf[64];
  static int ignore = 0;

  while( 1 ) {
    ssize_t available, i;

    available = read( STDIN_FILENO, &keybuf, sizeof( keybuf ) );
    if( available <= 0 ) return;

    for( i = 0; i < available; i++ )
      if( ignore ) {
	ignore--;
      } else if( ( keybuf[i] & 0x7f ) == 0 ) {
	ignore = 2; /* ignore extended keysyms */
      } else {
	input_key fuse_keysym;
	input_event_t fuse_event;

	fuse_keysym = keysyms_remap( keybuf[i] & 0x7f );

	if( fuse_keysym == INPUT_KEY_NONE ) continue;

	fuse_event.type = ( keybuf[i] & 0x80 ) ?
                          INPUT_EVENT_KEYRELEASE :
                          INPUT_EVENT_KEYPRESS;
	fuse_event.types.key.native_key = fuse_keysym;
	fuse_event.types.key.spectrum_key = fuse_keysym;

	input_event( &fuse_event );
      }
  }
}
