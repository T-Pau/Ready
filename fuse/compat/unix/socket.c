/* socket.c: Socket-related compatibility routines
   Copyright (c) 2011-2015 Philip Kendall

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
#include "fuse.h"
#include "ui/ui.h"

const compat_socket_t compat_socket_invalid = -1;
const int compat_socket_EBADF = EBADF;

struct compat_socket_selfpipe_t {
  int read_fd;
  int write_fd;
};

void
compat_socket_networking_init( void )
{
  /* No action necessary */
}

void
compat_socket_networking_end( void )
{
  /* No action necessary */
}

int
compat_socket_close( compat_socket_t fd )
{
  return close( fd );
}

int compat_socket_get_error( void )
{
  return errno;
}

const char *
compat_socket_get_strerror( void )
{
  return strerror( errno );
}

compat_socket_selfpipe_t* compat_socket_selfpipe_alloc( void )
{
  int error;
  int pipefd[2];

  compat_socket_selfpipe_t *self =
    libspectrum_new( compat_socket_selfpipe_t, 1 );

  error = pipe( pipefd );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s: %d: error %d creating pipe", __FILE__, __LINE__, error );
    fuse_abort();
  }

  self->read_fd = pipefd[0];
  self->write_fd = pipefd[1];

  return self;
}

void compat_socket_selfpipe_free( compat_socket_selfpipe_t *self )
{
  close( self->read_fd );
  close( self->write_fd );
  libspectrum_free( self );
}

compat_socket_t compat_socket_selfpipe_get_read_fd( compat_socket_selfpipe_t *self )
{
  return self->read_fd;
}

void compat_socket_selfpipe_wake( compat_socket_selfpipe_t *self )
{
  const char dummy = 0;
  write( self->write_fd, &dummy, 1 );
}

void compat_socket_selfpipe_discard_data( compat_socket_selfpipe_t *self )
{
  char bitbucket;
  ssize_t bytes_read;

  do {
    bytes_read = read( self->read_fd, &bitbucket, 1 );
    if( bytes_read == -1 && errno != EINTR ) {
      ui_error( UI_ERROR_ERROR,
                "%s: %d: unexpected error %d (%s) reading from pipe", __FILE__,
                __LINE__, errno, strerror(errno) );
    }
  } while( bytes_read < 0 );
}
