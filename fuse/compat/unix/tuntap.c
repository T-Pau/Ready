/* tuntap.c: TUN/TAP compatability layer
   Copyright (c) 2009-2010 Patrik Persson, Philip Kendall
   Copyright (c) 2016 Sergio Baldoví

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

#include "config.h"

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "ui/ui.h"

int
compat_get_tap( const char *interface_name )
{
  int fd = -1;

  if ( (fd = open( "/dev/net/tun", O_RDWR | O_NONBLOCK )) < 0 )
    ui_error( UI_ERROR_ERROR, "couldn't open TUN/TAP device '/dev/net/tun'" );
  else {
    struct ifreq ifr;
    memset( &ifr, 0, sizeof( ifr ) );
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy( ifr.ifr_name, interface_name, IFNAMSIZ );
    ifr.ifr_name[ IFNAMSIZ - 1 ] = '\0';

    if ( ioctl( fd, TUNSETIFF, (void *) &ifr ) < 0 ) {
      ui_error( UI_ERROR_ERROR, "couldn't select TAP interface '%s'",
                ifr.ifr_name );
    }
  }

  return fd;
}
