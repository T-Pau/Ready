/* socket.c: Socket-related compatibility routines
   Copyright (c) 2011-2015 Sergio Baldoví, Philip Kendall

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

#include <winsock2.h>
#include <ws2tcpip.h>

#include "compat.h"
#include "fuse.h"
#include "ui/ui.h"

const compat_socket_t compat_socket_invalid = INVALID_SOCKET;
const int compat_socket_EBADF = WSAENOTSOCK;

struct compat_socket_selfpipe_t {
  SOCKET self_socket;
  libspectrum_word port;
};

int
compat_socket_close( compat_socket_t fd )
{
  return closesocket( fd );
}

int compat_socket_get_error( void )
{
  return WSAGetLastError();
}

const char *
compat_socket_get_strerror( void )
{
  static TCHAR buffer[256];
  TCHAR *ptr;
  DWORD msg_size;

  /* get description of last winsock error */
  msg_size = FormatMessage( 
               FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
               NULL, WSAGetLastError(),
               MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
               buffer, ARRAY_SIZE( buffer ), NULL );

  if( !msg_size ) return NULL;

  /* skip 'new line' like chars */
  for( ptr = buffer; *ptr; ptr++ ) {
    if( ( *ptr == '\r' ) || ( *ptr == '\n' ) ) {
      *ptr = '\0';
      break;
    }
  }

  return (const char *)buffer;
}

static int
selfpipe_test( compat_socket_selfpipe_t *self )
{
  fd_set readfds;
  int active;
  struct timeval tv = { 1, 0 };

  /* Send testing packet */
  compat_socket_selfpipe_wake( self );

  /* Safe reading from control socket */
  FD_ZERO( &readfds );
  FD_SET( self->self_socket, &readfds );
  active = select( 0, &readfds, NULL, NULL, &tv );
  if( active == 0 || active == compat_socket_invalid ) {
    return -1;
  }

  /* Discard testing packet */
  if( FD_ISSET( self->self_socket, &readfds ) ) {
    compat_socket_selfpipe_discard_data( self );
  }

  return 0;
}

compat_socket_selfpipe_t *
compat_socket_selfpipe_alloc( void )
{
  unsigned long mode = 1;
  struct sockaddr_in sa;
  socklen_t sa_len = sizeof(sa);

  compat_socket_selfpipe_t *self =
    libspectrum_new( compat_socket_selfpipe_t, 1 );
  
  self->self_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
  if( self->self_socket == compat_socket_invalid ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to open socket; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  /* Set nonblocking mode */
  if( ioctlsocket( self->self_socket, FIONBIO, &mode ) == -1 ) {
    ui_error( UI_ERROR_ERROR, 
              "%s: %d: failed to set socket nonblocking; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

  if( bind( self->self_socket, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to bind socket; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  /* Get ephemeral port number */
  if( getsockname( self->self_socket, (struct sockaddr *)&sa, &sa_len ) == -1 ) {
    ui_error( UI_ERROR_ERROR,
              "%s: %d: failed to get socket name; errno %d: %s\n",
              __FILE__, __LINE__, compat_socket_get_error(),
              compat_socket_get_strerror() );
    fuse_abort();
  }

  self->port = ntohs( sa.sin_port );
 
  /* Test communications in order to detect blocking firewalls */
  if( selfpipe_test( self ) == -1 ) {
    ui_error( UI_ERROR_ERROR,
              "Networking: failed to test internal communications" );
    fuse_abort();
  }

  return self;
}

void
compat_socket_selfpipe_free( compat_socket_selfpipe_t *self )
{
  compat_socket_close( self->self_socket );
  libspectrum_free( self );
}

compat_socket_t
compat_socket_selfpipe_get_read_fd( compat_socket_selfpipe_t *self )
{
  return self->self_socket;
}

void
compat_socket_selfpipe_wake( compat_socket_selfpipe_t *self )
{
  struct sockaddr_in sa;

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
  sa.sin_port = htons( self->port );

  sendto( self->self_socket, NULL, 0, 0, (struct sockaddr*)&sa, sizeof(sa) );
}

void
compat_socket_selfpipe_discard_data( compat_socket_selfpipe_t *self )
{
  ssize_t bytes_read;
  struct sockaddr_in sa;
  socklen_t sa_length = sizeof(sa);
  static char bitbucket[0x100];

  do {
    /* Socket is non blocking, so we can do this safely */
    bytes_read = recvfrom( self->self_socket, bitbucket, sizeof( bitbucket ),
                           0, (struct sockaddr*)&sa, &sa_length );
  } while( bytes_read != -1 );
}


void
compat_socket_networking_init( void )
{
  WORD wVersionRequested;
  WSADATA wsaData;
  int error;

  wVersionRequested = MAKEWORD( 2, 2 );
  error = WSAStartup( wVersionRequested, &wsaData );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s:%d: error %d from WSAStartup()", __FILE__,
              __LINE__, error );
    fuse_abort();
  }

  if( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) {
    ui_error( UI_ERROR_ERROR,
              "%s:%d: unexpected version 0x%02x from WSAStartup()",
              __FILE__, __LINE__, wsaData.wVersion );
    fuse_abort();
  }
}

void
compat_socket_networking_end( void )
{
  WSACleanup();
}

