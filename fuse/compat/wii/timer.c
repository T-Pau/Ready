/* unix.c: UNIX speed routines for Fuse
   Copyright (c) 1999-2007 Philip Kendall, Marek Januszewski, Fredrick Meunier

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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "compat.h"
#include "ui/ui.h"

/* FIXME: where should we get this prototype from? */
extern int clock_gettime(struct timespec *tp);

double
compat_timer_get_time( void )
{
  int error;
  struct timespec tp;

  error = clock_gettime(&tp);
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s: error getting time: %s", __func__, strerror( errno ) );
    return -1;
  }

  return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}

void
compat_timer_sleep( int ms )
{
  usleep( ms * 1000 );
}
