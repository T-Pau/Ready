/* w5100_socket.c: Wiznet W5100 emulation - sockets code
   
   Emulates a minimal subset of the Wiznet W5100 TCP/IP controller.

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

#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "fuse.h"
#include "ui/ui.h"
#include "w5100.h"
#include "w5100_internals.h"

enum w5100_socket_command {
  W5100_SOCKET_COMMAND_OPEN = 1 << 0,
  W5100_SOCKET_COMMAND_LISTEN = 1 << 1,
  W5100_SOCKET_COMMAND_CONNECT = 1 << 2,
  W5100_SOCKET_COMMAND_DISCON = 1 << 3,
  W5100_SOCKET_COMMAND_CLOSE = 1 << 4,
  W5100_SOCKET_COMMAND_SEND = 1 << 5,
  W5100_SOCKET_COMMAND_RECV = 1 << 6,
};

static void
w5100_socket_init_common( nic_w5100_socket_t *socket )
{
  socket->fd = compat_socket_invalid;
  socket->bind_count = 0;
  socket->socket_bound = 0;
  socket->ok_for_io = 0;
  socket->write_pending = 0;
}

void
nic_w5100_socket_init( nic_w5100_socket_t *socket, int which )
{
  socket->id = which;
  w5100_socket_init_common( socket );
  pthread_mutex_init( &socket->lock, NULL );
}

void
nic_w5100_socket_end( nic_w5100_socket_t *socket )
{
  nic_w5100_socket_reset( socket );
  pthread_mutex_destroy( &socket->lock );
}

static void
w5100_socket_acquire_lock( nic_w5100_socket_t *socket )
{
  int error = pthread_mutex_lock( &socket->lock );
  if( error ) {
    nic_w5100_error( UI_ERROR_ERROR,
                     "%s:%d: error %d locking mutex for socket %d\n",
                     __FILE__, __LINE__, error, socket->id );
    fuse_abort();
  }
}

static void
w5100_socket_release_lock( nic_w5100_socket_t *socket )
{
  int error = pthread_mutex_unlock( &socket->lock );
  if( error ) {
    nic_w5100_debug( "%s:%d: error %d unlocking mutex for socket %d\n", __FILE__, __LINE__, error, socket->id );
    fuse_abort();
  }
}

static void
w5100_socket_clean( nic_w5100_socket_t *socket )
{
  socket->ir = 0;
  memset( socket->port, 0, sizeof( socket->port ) );
  memset( socket->dip, 0, sizeof( socket->dip ) );
  memset( socket->dport, 0, sizeof( socket->dport ) );
  socket->tx_rr = socket->tx_wr = 0;
  socket->rx_rsr = 0;
  socket->old_rx_rd = socket->rx_rd = 0;

  socket->last_send = 0;
  socket->datagram_count = 0;

  if( socket->fd != compat_socket_invalid ) {
    compat_socket_close( socket->fd );
    w5100_socket_init_common( socket );
  }
}

void
nic_w5100_socket_reset( nic_w5100_socket_t *socket )
{
  w5100_socket_acquire_lock( socket );

  socket->mode = W5100_SOCKET_MODE_CLOSED;
  socket->flags = 0;
  socket->state = W5100_SOCKET_STATE_CLOSED;

  w5100_socket_clean( socket );

  w5100_socket_release_lock( socket );
}

static void
w5100_write_socket_mr( nic_w5100_socket_t *socket, libspectrum_byte b )
{
  w5100_socket_mode mode = b & 0x0f;
  libspectrum_byte flags = b & 0xf0;

  nic_w5100_debug( "w5100: writing 0x%02x to S%d_MR\n", b, socket->id );

  switch( mode ) {
    case W5100_SOCKET_MODE_CLOSED:
      break;
    case W5100_SOCKET_MODE_TCP:
      /* We support only "disable no delayed ACK" */
      if( flags != 0x20 ) {
        ui_error( UI_ERROR_WARNING, "w5100: unsupported flags 0x%02x set for TCP mode on socket %d\n", b & 0xf0, socket->id );
        flags = 0x20;
      }
      break;
    case W5100_SOCKET_MODE_UDP:
      /* We don't support multicast */
      if( flags != 0x00 ) {
        ui_error( UI_ERROR_WARNING, "w5100: unsupported flags 0x%02x set for UDP mode on socket %d\n", b & 0xf0, socket->id );
        flags = 0;
      }
      break;
    case W5100_SOCKET_MODE_IPRAW:
    case W5100_SOCKET_MODE_MACRAW:
    case W5100_SOCKET_MODE_PPPOE:
    default:
      ui_error( UI_ERROR_WARNING, "w5100: unsupported mode 0x%02x set on socket %d\n", b, socket->id );
      mode = W5100_SOCKET_MODE_CLOSED;
      break;
  }

  socket->mode = mode;
  socket->flags = flags;
}

static void
w5100_socket_open( nic_w5100_socket_t *socket_obj )
{
  if( ( socket_obj->mode == W5100_SOCKET_MODE_UDP ||
      socket_obj->mode == W5100_SOCKET_MODE_TCP ) &&
    socket_obj->state == W5100_SOCKET_STATE_CLOSED ) {

    int tcp = socket_obj->mode == W5100_SOCKET_MODE_TCP;
    int type = tcp ? SOCK_STREAM : SOCK_DGRAM;
    int protocol = tcp ? IPPROTO_TCP : IPPROTO_UDP;
    const char *description = tcp ? "TCP" : "UDP";
    int final_state = tcp ? W5100_SOCKET_STATE_INIT : W5100_SOCKET_STATE_UDP;
#ifndef WIN32
    int one = 1;
#endif

    w5100_socket_clean( socket_obj );

    socket_obj->fd = socket( AF_INET, type, protocol );
    if( socket_obj->fd == compat_socket_invalid ) {
      nic_w5100_error( UI_ERROR_ERROR,
        "w5100: failed to open %s socket for socket %d; errno %d: %s\n",
        description, socket_obj->id, compat_socket_get_error(),
        compat_socket_get_strerror() );
      return;
    }

#ifndef WIN32
    /* Windows warning: this could forcibly bind sockets already in use */
    if( setsockopt( socket_obj->fd, SOL_SOCKET, SO_REUSEADDR, &one,
      sizeof(one) ) == -1 ) {
      nic_w5100_error( UI_ERROR_ERROR,
        "w5100: failed to set SO_REUSEADDR on socket %d; errno %d: %s\n",
        socket_obj->id, compat_socket_get_error(),
        compat_socket_get_strerror() );
    }
#endif

    socket_obj->state = final_state;

    nic_w5100_debug( "w5100: opened %s fd %d for socket %d\n", description, socket_obj->fd, socket_obj->id );
  }
}

static int
w5100_socket_bind_port( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  struct sockaddr_in sa;

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  memcpy( &sa.sin_port, socket->port, 2 );
  memcpy( &sa.sin_addr.s_addr, self->sip, 4 );

  nic_w5100_debug( "w5100: attempting to bind socket %d to %s:%d\n", socket->id, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port) );
  if( bind( socket->fd, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
    nic_w5100_error( UI_ERROR_ERROR,
                     "w5100: failed to bind socket %d; errno %d: %s\n",
                     socket->id, compat_socket_get_error(),
                     compat_socket_get_strerror() );

    socket->ir |= 1 << 3;
    socket->state = W5100_SOCKET_STATE_CLOSED;
    return -1;
  }

  socket->socket_bound = 1;
  nic_w5100_debug( "w5100: successfully bound socket %d\n", socket->id );

  return 0;
}

static void
w5100_socket_listen( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->state == W5100_SOCKET_STATE_INIT ) {

    if( !socket->socket_bound )
      if( w5100_socket_bind_port( self, socket ) )
        return;

    if( listen( socket->fd, 1 ) == -1 ) {
      nic_w5100_error( UI_ERROR_ERROR, 
                       "w5100: failed to listen on socket %d; errno %d: %s\n",
                       socket->id, compat_socket_get_error(),
                       compat_socket_get_strerror() );
      return;
    }

    socket->state = W5100_SOCKET_STATE_LISTEN;

    nic_w5100_debug( "w5100: listening on socket %d\n", socket->id );

    compat_socket_selfpipe_wake( self->selfpipe );
  }
}

static void
w5100_socket_connect( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->state == W5100_SOCKET_STATE_INIT ) {
    struct sockaddr_in sa;
    
    if( !socket->socket_bound )
      if( w5100_socket_bind_port( self, socket ) )
        return;

    memset( &sa, 0, sizeof(sa) );
    sa.sin_family = AF_INET;
    memcpy( &sa.sin_port, socket->dport, 2 );
    memcpy( &sa.sin_addr.s_addr, socket->dip, 4 );

    if( connect( socket->fd, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
      nic_w5100_error( UI_ERROR_ERROR,
        "w5100: failed to connect socket %d to 0x%08x:0x%04x; errno %d: %s\n",
        socket->id, ntohl(sa.sin_addr.s_addr), ntohs(sa.sin_port),
        compat_socket_get_error(), compat_socket_get_strerror() );

      socket->ir |= 1 << 3;
      socket->state = W5100_SOCKET_STATE_CLOSED;
      return;
    }

    socket->ir |= 1 << 0;
    socket->state = W5100_SOCKET_STATE_ESTABLISHED;
  }
}

static void
w5100_socket_discon( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->state == W5100_SOCKET_STATE_ESTABLISHED ||
    socket->state == W5100_SOCKET_STATE_CLOSE_WAIT ) {
    socket->ir |= 1 << 1;
    socket->state = W5100_SOCKET_STATE_CLOSED;
    compat_socket_selfpipe_wake( self->selfpipe );

    nic_w5100_debug( "w5100: disconnected socket %d\n", socket->id );
  }
}

static void
w5100_socket_close( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->fd != compat_socket_invalid ) {
    compat_socket_close( socket->fd );
    socket->fd = compat_socket_invalid;
    socket->socket_bound = 0;
    socket->ok_for_io = 0;
    socket->state = W5100_SOCKET_STATE_CLOSED;
    compat_socket_selfpipe_wake( self->selfpipe );
    nic_w5100_debug( "w5100: closed socket %d\n", socket->id );
  }
}

static void
w5100_socket_send( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->state == W5100_SOCKET_STATE_UDP ) {

    if( !socket->socket_bound )
      if( w5100_socket_bind_port( self, socket ) )
        return;

    socket->datagram_lengths[socket->datagram_count++] =
      socket->tx_wr - socket->last_send;
    socket->last_send = socket->tx_wr;
    socket->write_pending = 1;
    compat_socket_selfpipe_wake( self->selfpipe );
  }
  else if( socket->state == W5100_SOCKET_STATE_ESTABLISHED ) {
    socket->write_pending = 1;
    compat_socket_selfpipe_wake( self->selfpipe );
  }
}

static void
w5100_socket_recv( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->state == W5100_SOCKET_STATE_UDP ||
    socket->state == W5100_SOCKET_STATE_ESTABLISHED ) {
    socket->rx_rsr -= socket->rx_rd - socket->old_rx_rd;
    socket->old_rx_rd = socket->rx_rd;
    if( socket->rx_rsr != 0 )
      socket->ir |= 1 << 2;
    compat_socket_selfpipe_wake( self->selfpipe );
  }
}

static void
w5100_write_socket_cr( nic_w5100_t *self, nic_w5100_socket_t *socket, libspectrum_byte b )
{
  nic_w5100_debug( "w5100: writing 0x%02x to S%d_CR\n", b, socket->id );

  switch( b ) {
    case W5100_SOCKET_COMMAND_OPEN:
      w5100_socket_open( socket );
      break;
    case W5100_SOCKET_COMMAND_LISTEN:
      w5100_socket_listen( self, socket );
      break;
    case W5100_SOCKET_COMMAND_CONNECT:
      w5100_socket_connect( self, socket );
      break;
    case W5100_SOCKET_COMMAND_DISCON:
      w5100_socket_discon( self, socket );
      break;
    case W5100_SOCKET_COMMAND_CLOSE:
      w5100_socket_close( self, socket );
      break;
    case W5100_SOCKET_COMMAND_SEND:
      w5100_socket_send( self, socket );
      break;
    case W5100_SOCKET_COMMAND_RECV:
      w5100_socket_recv( self, socket );
      break;
    default:
      ui_error( UI_ERROR_WARNING, "w5100: unknown command 0x%02x sent to socket %d\n", b, socket->id );
      break;
  }
}

static void
w5100_write_socket_port( nic_w5100_t *self, nic_w5100_socket_t *socket, int which, libspectrum_byte b )
{
  nic_w5100_debug( "w5100: writing 0x%02x to S%d_PORT%d\n", b, socket->id, which );
  socket->port[which] = b;
  if( ++socket->bind_count == 2 ) {
    if( socket->state == W5100_SOCKET_STATE_UDP && !socket->socket_bound ) {
      if( w5100_socket_bind_port( self, socket ) ) {
        socket->bind_count = 0;
        return;
      }
      compat_socket_selfpipe_wake( self->selfpipe );
    }
    socket->bind_count = 0;
  }
}

libspectrum_byte
nic_w5100_socket_read( nic_w5100_t *self, libspectrum_word reg )
{
  nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
  int socket_reg = reg & 0xff;
  int reg_offset;
  libspectrum_word fsr;
  libspectrum_byte b;

  w5100_socket_acquire_lock( socket );

  switch( socket_reg ) {
    case W5100_SOCKET_MR:
      b = socket->mode;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_MR\n", b, socket->id );
      break;
    case W5100_SOCKET_IR:
      b = socket->ir;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_IR\n", b, socket->id );
      break;
    case W5100_SOCKET_SR:
      b = socket->state;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_SR\n", b, socket->id );
      break;
    case W5100_SOCKET_PORT0: case W5100_SOCKET_PORT1:
      b = socket->port[socket_reg - W5100_SOCKET_PORT0];
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_PORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_PORT0 );
      break;
    case W5100_SOCKET_TX_FSR0: case W5100_SOCKET_TX_FSR1:
      reg_offset = socket_reg - W5100_SOCKET_TX_FSR0;
      fsr = 0x0800 - (socket->tx_wr - socket->tx_rr);
      b = ( fsr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_TX_FSR%d\n", b, socket->id, reg_offset );
      break;
    case W5100_SOCKET_TX_RR0: case W5100_SOCKET_TX_RR1:
      reg_offset = socket_reg - W5100_SOCKET_TX_RR0;
      b = ( socket->tx_rr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_TX_RR%d\n", b, socket->id, reg_offset );
      break;
    case W5100_SOCKET_TX_WR0: case W5100_SOCKET_TX_WR1:
      reg_offset = socket_reg - W5100_SOCKET_TX_WR0;
      b = ( socket->tx_wr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_TX_WR%d\n", b, socket->id, reg_offset );
      break;
    case W5100_SOCKET_RX_RSR0: case W5100_SOCKET_RX_RSR1:
      reg_offset = socket_reg - W5100_SOCKET_RX_RSR0;
      b = ( socket->rx_rsr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_RX_RSR%d\n", b, socket->id, reg_offset );
      break;
    case W5100_SOCKET_RX_RD0: case W5100_SOCKET_RX_RD1:
      reg_offset = socket_reg - W5100_SOCKET_RX_RD0;
      b = ( socket->rx_rd >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      nic_w5100_debug( "w5100: reading 0x%02x from S%d_RX_RD%d\n", b, socket->id, reg_offset );
      break;
    default:
      b = 0xff;
      ui_error( UI_ERROR_WARNING, "w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
      break;
  }

  w5100_socket_release_lock( socket );

  return b;
}

void
nic_w5100_socket_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
  int socket_reg = reg & 0xff;

  w5100_socket_acquire_lock( socket );

  switch( socket_reg ) {
    case W5100_SOCKET_MR:
      w5100_write_socket_mr( socket, b );
      break;
    case W5100_SOCKET_CR:
      w5100_write_socket_cr( self, socket, b );
      break;
    case W5100_SOCKET_IR:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_IR\n", b, socket->id );
      socket->ir &= ~b;
      break;
    case W5100_SOCKET_PORT0: case W5100_SOCKET_PORT1:
      w5100_write_socket_port( self, socket, socket_reg - W5100_SOCKET_PORT0, b );
      break;
    case W5100_SOCKET_DIPR0: case W5100_SOCKET_DIPR1: case W5100_SOCKET_DIPR2: case W5100_SOCKET_DIPR3:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_DIPR%d\n", b, socket->id, socket_reg - W5100_SOCKET_DIPR0 );
      socket->dip[socket_reg - W5100_SOCKET_DIPR0] = b;
      break;
    case W5100_SOCKET_DPORT0: case W5100_SOCKET_DPORT1:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_DPORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_DPORT0 );
      socket->dport[socket_reg - W5100_SOCKET_DPORT0] = b;
      break;
    case W5100_SOCKET_TX_WR0:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_TX_WR0\n", b, socket->id );
      socket->tx_wr = (socket->tx_wr & 0xff) | (b << 8);
      break;
    case W5100_SOCKET_TX_WR1:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_TX_WR1\n", b, socket->id );
      socket->tx_wr = (socket->tx_wr & 0xff00) | b;
      break;
    case W5100_SOCKET_RX_RD0:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_RX_RD0\n", b, socket->id );
      socket->rx_rd = (socket->rx_rd & 0xff) | (b << 8);
      break;
    case W5100_SOCKET_RX_RD1:
      nic_w5100_debug( "w5100: writing 0x%02x to S%d_RX_RD1\n", b, socket->id );
      socket->rx_rd = (socket->rx_rd & 0xff00) | b;
      break;
    default:
      ui_error( UI_ERROR_WARNING, "w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg );
      break;
  }

  if( socket_reg != W5100_SOCKET_PORT0 && socket_reg != W5100_SOCKET_PORT1 )
    socket->bind_count = 0;

  w5100_socket_release_lock( socket );
}

libspectrum_byte
nic_w5100_socket_read_rx_buffer( nic_w5100_t *self, libspectrum_word reg )
{
  nic_w5100_socket_t *socket = &self->socket[(reg - 0x6000) / 0x0800];
  int offset = reg & 0x7ff;
  libspectrum_byte b = socket->rx_buffer[offset];
  nic_w5100_debug( "w5100: reading 0x%02x from socket %d rx buffer offset 0x%03x\n", b, socket->id, offset );
  return b;
}

void
nic_w5100_socket_write_tx_buffer( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  nic_w5100_socket_t *socket = &self->socket[(reg - 0x4000) / 0x0800];
  int offset = reg & 0x7ff;
  nic_w5100_debug( "w5100: writing 0x%02x to socket %d tx buffer offset 0x%03x\n", b, socket->id, offset );
  socket->tx_buffer[offset] = b;
}

void
nic_w5100_socket_add_to_sets( nic_w5100_socket_t *socket, fd_set *readfds,
  fd_set *writefds, int *max_fd )
{
  w5100_socket_acquire_lock( socket );

  if( socket->fd != compat_socket_invalid ) {
    /* We can process a UDP read if we're in a UDP state and there are at least
       9 bytes free in our buffer (8 byte UDP header and 1 byte of actual
       data). */
    int udp_read = socket->state == W5100_SOCKET_STATE_UDP &&
      0x800 - socket->rx_rsr >= 9;
    /* We can process a TCP read if we're in the established state and have
       any room in our buffer (no header necessary for TCP). */
    int tcp_read = socket->state == W5100_SOCKET_STATE_ESTABLISHED &&
      0x800 - socket->rx_rsr >= 1;

    int tcp_listen = socket->state == W5100_SOCKET_STATE_LISTEN;

    socket->ok_for_io = 1;

    if( udp_read || tcp_read || tcp_listen ) {
      FD_SET( socket->fd, readfds );
      if( socket->fd > *max_fd )
        *max_fd = socket->fd;
      nic_w5100_debug( "w5100: checking for read on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, *max_fd );
    }

    if( socket->write_pending ) {
      FD_SET( socket->fd, writefds );
      if( socket->fd > *max_fd )
        *max_fd = socket->fd;
      nic_w5100_debug( "w5100: write pending on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, *max_fd );
    }
  }

  w5100_socket_release_lock( socket );
}

static void
w5100_socket_process_accept( nic_w5100_socket_t *socket )
{
  struct sockaddr_in sa;
  socklen_t sa_length = sizeof(sa);
  compat_socket_t new_fd;

  memset( &sa, 0, sizeof(sa) );

  new_fd = accept( socket->fd, (struct sockaddr*)&sa, &sa_length );
  if( new_fd == compat_socket_invalid ) {
    nic_w5100_debug( "w5100: error from accept on socket %d; errno %d: %s\n",
                     socket->id, compat_socket_get_error(),
                     compat_socket_get_strerror() );
    return;
  }

  nic_w5100_debug( "w5100: accepted connection from %s:%d on socket %d\n", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), socket->id );

  if( compat_socket_close( socket->fd ) == -1 )
    nic_w5100_debug( "w5100: error attempting to close fd %d for socket %d\n", socket->fd, socket->id );

  socket->fd = new_fd;
  socket->state = W5100_SOCKET_STATE_ESTABLISHED;
}

static void
w5100_socket_process_read( nic_w5100_socket_t *socket )
{
  libspectrum_byte buffer[0x800];
  int bytes_free = 0x800 - socket->rx_rsr;
  ssize_t bytes_read;
  struct sockaddr_in sa;

  int udp = socket->state == W5100_SOCKET_STATE_UDP;
  const char *description = udp ? "UDP" : "TCP";

  nic_w5100_debug( "w5100: reading from socket %d\n", socket->id );

  if( udp ) {
    socklen_t sa_length = sizeof(sa);
    bytes_read = recvfrom( socket->fd, (char*)buffer + 8, bytes_free - 8, 0,
      (struct sockaddr*)&sa, &sa_length );
  }
  else
    bytes_read = recv( socket->fd, (char*)buffer, bytes_free, 0 );

  nic_w5100_debug( "w5100: read 0x%03x bytes from %s socket %d\n", (int)bytes_read, description, socket->id );

  if( bytes_read > 0 || (udp && bytes_read == 0) ) {
    int offset = (socket->old_rx_rd + socket->rx_rsr) & 0x7ff;
    libspectrum_byte *dest = &socket->rx_buffer[offset];

    if( udp ) {
      /* Add the W5100's UDP header */
      memcpy( buffer, &sa.sin_addr.s_addr, 4 );
      memcpy( buffer + 4, &sa.sin_port, 2 );
      buffer[6] = (bytes_read >> 8) & 0xff;
      buffer[7] = bytes_read & 0xff;
      bytes_read += 8;
    }

    socket->rx_rsr += bytes_read;
    socket->ir |= 1 << 2;

    if( offset + bytes_read <= 0x800 ) {
      memcpy( dest, buffer, bytes_read );
    }
    else {
      int first_chunk = 0x800 - offset;
      memcpy( dest, buffer, first_chunk );
      memcpy( socket->rx_buffer, buffer + first_chunk, bytes_read - first_chunk );
    }
  }
  else if( bytes_read == 0 ) {  /* TCP */
    socket->state = W5100_SOCKET_STATE_CLOSE_WAIT;
    nic_w5100_debug( "w5100: EOF on %s socket %d; errno %d: %s\n",
                     description, socket->id, compat_socket_get_error(),
                     compat_socket_get_strerror() );
  }
  else {
    nic_w5100_debug( "w5100: error %d reading from %s socket %d: %s\n",
                     compat_socket_get_error(), description, socket->id,
                     compat_socket_get_strerror() );
  }
}

static void
w5100_socket_process_udp_write( nic_w5100_socket_t *socket )
{
  ssize_t bytes_sent;
  int offset = socket->tx_rr & 0x7ff;
  libspectrum_word length = socket->datagram_lengths[0];
  libspectrum_byte *data = &socket->tx_buffer[ offset ];
  struct sockaddr_in sa;
  libspectrum_byte buffer[0x800];

  nic_w5100_debug( "w5100: writing to UDP socket %d\n", socket->id );

  /* If the data wraps round the write buffer, we need to coalesce it into
     one chunk for the call to sendto() */
  if( offset + length > 0x800 ) {
    int first_chunk = 0x800 - offset;
    memcpy( buffer, data, first_chunk );
    memcpy( buffer + first_chunk, socket->tx_buffer, length - first_chunk );
    data = buffer;
  }

  memset( &sa, 0, sizeof(sa) );
  sa.sin_family = AF_INET;
  memcpy( &sa.sin_port, socket->dport, 2 );
  memcpy( &sa.sin_addr.s_addr, socket->dip, 4 );

  bytes_sent = sendto( socket->fd, (const char*)data, length, 0, (struct sockaddr*)&sa, sizeof(sa) );
  nic_w5100_debug( "w5100: sent 0x%03x bytes of 0x%03x to UDP socket %d\n",
                   (int)bytes_sent, length, socket->id );

  if( bytes_sent == length ) {
    if( --socket->datagram_count )
      memmove( socket->datagram_lengths, &socket->datagram_lengths[1],
        0x1f * sizeof(int) );

    socket->tx_rr += bytes_sent;
    if( socket->datagram_count == 0 ) {
      socket->write_pending = 0;
      socket->ir |= 1 << 4;
    }
  }
  else if( bytes_sent != -1 )
    nic_w5100_debug( "w5100: didn't manage to send full datagram to UDP socket %d?\n", socket->id );
  else
    nic_w5100_debug( "w5100: error %d writing to UDP socket %d: %s\n",
                     compat_socket_get_error(), socket->id,
                     compat_socket_get_strerror() );
}

static void
w5100_socket_process_tcp_write( nic_w5100_socket_t *socket )
{
  ssize_t bytes_sent;
  int offset = socket->tx_rr & 0x7ff;
  libspectrum_word length = socket->tx_wr - socket->tx_rr;
  libspectrum_byte *data = &socket->tx_buffer[ offset ];

  nic_w5100_debug( "w5100: writing to TCP socket %d\n", socket->id );

  /* If the data wraps round the write buffer, write it in two chunks */
  if( offset + length > 0x800 )
    length = 0x800 - offset;

  bytes_sent = send( socket->fd, (const char*)data, length, 0 );
  nic_w5100_debug( "w5100: sent 0x%03x bytes of 0x%03x to TCP socket %d\n",
                   (int)bytes_sent, length, socket->id );

  if( bytes_sent != -1 ) {
    socket->tx_rr += bytes_sent;
    if( socket->tx_rr == socket->tx_wr ) {
      socket->write_pending = 0;
      socket->ir |= 1 << 4;
    }
  }
  else
    nic_w5100_debug( "w5100: error %d writing to TCP socket %d: %s\n",
                     compat_socket_get_error(), socket->id,
                     compat_socket_get_strerror() );
}

void
nic_w5100_socket_process_io( nic_w5100_socket_t *socket, fd_set readfds,
  fd_set writefds )
{
  w5100_socket_acquire_lock( socket );

  /* Process only if we're an open socket, and we haven't been closed and
     re-opened since the select() started */
  if( socket->fd != compat_socket_invalid && socket->ok_for_io ) {
    if( FD_ISSET( socket->fd, &readfds ) ) {
      if( socket->state == W5100_SOCKET_STATE_LISTEN )
        w5100_socket_process_accept( socket );
      else
        w5100_socket_process_read( socket );
    }

    if( FD_ISSET( socket->fd, &writefds ) ) {
      if( socket->state == W5100_SOCKET_STATE_UDP ) {
        w5100_socket_process_udp_write( socket );
      }
      else if( socket->state == W5100_SOCKET_STATE_ESTABLISHED ) {
        w5100_socket_process_tcp_write( socket );
      }
    }
  }

  w5100_socket_release_lock( socket );
}
