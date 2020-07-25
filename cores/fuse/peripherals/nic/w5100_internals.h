/* w5100.c: Wiznet W5100 emulation - internal routines
   
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

#ifndef FUSE_W5100_INTERNALS_H
#define FUSE_W5100_INTERNALS_H

#include <signal.h>

#ifndef WIN32
#include <sys/select.h>
#endif

typedef enum w5100_socket_mode {
  W5100_SOCKET_MODE_CLOSED = 0x00,
  W5100_SOCKET_MODE_TCP,
  W5100_SOCKET_MODE_UDP,
  W5100_SOCKET_MODE_IPRAW,
  W5100_SOCKET_MODE_MACRAW,
  W5100_SOCKET_MODE_PPPOE,
} w5100_socket_mode;

typedef enum w5100_socket_state {
  W5100_SOCKET_STATE_CLOSED = 0x00,

  W5100_SOCKET_STATE_INIT = 0x13,
  W5100_SOCKET_STATE_LISTEN,
  W5100_SOCKET_STATE_ESTABLISHED = 0x17,
  W5100_SOCKET_STATE_CLOSE_WAIT = 0x1c,

  W5100_SOCKET_STATE_UDP = 0x22,
} w5100_socket_state;

enum w5100_socket_registers {
  W5100_SOCKET_MR = 0x00,
  W5100_SOCKET_CR,
  W5100_SOCKET_IR,
  W5100_SOCKET_SR,
  
  W5100_SOCKET_PORT0,
  W5100_SOCKET_PORT1,

  W5100_SOCKET_DIPR0 = 0x0c,
  W5100_SOCKET_DIPR1,
  W5100_SOCKET_DIPR2,
  W5100_SOCKET_DIPR3,

  W5100_SOCKET_DPORT0,
  W5100_SOCKET_DPORT1,

  W5100_SOCKET_TX_FSR0 = 0x20,
  W5100_SOCKET_TX_FSR1,
  W5100_SOCKET_TX_RR0,
  W5100_SOCKET_TX_RR1,
  W5100_SOCKET_TX_WR0,
  W5100_SOCKET_TX_WR1,

  W5100_SOCKET_RX_RSR0,
  W5100_SOCKET_RX_RSR1,
  W5100_SOCKET_RX_RD0,
  W5100_SOCKET_RX_RD1,
};

typedef struct nic_w5100_socket_t {

  int id; /* For debug use only */

  /* W5100 properties */

  w5100_socket_mode mode;
  libspectrum_byte flags;

  w5100_socket_state state;

  libspectrum_byte ir;      /* Interrupt register */

  libspectrum_byte port[2]; /* Source port */

  libspectrum_byte dip[4];  /* Destination IP address */
  libspectrum_byte dport[2];/* Destination port */

  libspectrum_word tx_rr;   /* Transmit read pointer */
  libspectrum_word tx_wr;   /* Transmit write pointer */

  libspectrum_word rx_rsr;  /* Received size */
  libspectrum_word rx_rd;   /* Received read pointer */

  libspectrum_word old_rx_rd; /* Used in RECV command processing */

  libspectrum_byte tx_buffer[0x800];  /* Transmit buffer */
  libspectrum_byte rx_buffer[0x800];  /* Received buffer */

  /* Host properties */

  compat_socket_t fd;       /* Socket file descriptor */
  int bind_count;           /* Number of writes to the Sn_PORTx registers we've received */
  int socket_bound;         /* True once we've bound the socket to a port */
  int write_pending;        /* True if we're waiting to write data on this socket */

  int last_send;            /* The value of Sn_TX_WR when the SEND command was last sent */
  int datagram_lengths[0x20]; /* The lengths of datagrams to be sent */
  int datagram_count;

  /* Flag used to indicate that a socket has been closed since we started
     waiting for it in a select() call and therefore the socket should no
     longer be used */
  int ok_for_io;

  pthread_mutex_t lock;     /* Mutex for this socket */

} nic_w5100_socket_t;

struct nic_w5100_t {
  libspectrum_byte gw[4];   /* Gateway IP address */
  libspectrum_byte sub[4];  /* Our subnet mask */
  libspectrum_byte sha[6];  /* MAC address */
  libspectrum_byte sip[4];  /* Our IP address */

  nic_w5100_socket_t socket[4];

  pthread_t thread;         /* Thread for doing I/O */
  sig_atomic_t stop_io_thread; /* Flag to stop I/O thread */
  compat_socket_selfpipe_t *selfpipe; /* Device for waking I/O thread */
};

void nic_w5100_socket_init( nic_w5100_socket_t *socket, int which );
void nic_w5100_socket_end( nic_w5100_socket_t *socket );

void nic_w5100_socket_reset( nic_w5100_socket_t *socket );

libspectrum_byte nic_w5100_socket_read( nic_w5100_t *self, libspectrum_word reg );
void nic_w5100_socket_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b );

libspectrum_byte nic_w5100_socket_read_rx_buffer( nic_w5100_t *self, libspectrum_word reg );
void nic_w5100_socket_write_tx_buffer( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b );

void nic_w5100_socket_add_to_sets( nic_w5100_socket_t *socket, fd_set *readfds,
  fd_set *writefds, int *max_fd );
void nic_w5100_socket_process_io( nic_w5100_socket_t *socket, fd_set readfds,
  fd_set writefds );

/* Debug routines */

/* Define this to spew debugging info to stdout */
#define W5100_DEBUG 0

void nic_w5100_debug( const char *format, ... )
     GCC_PRINTF( 1, 2 );
void nic_w5100_vdebug( const char *format, va_list ap )
     GCC_PRINTF( 1, 0 );
void nic_w5100_error( int severity, const char *format, ... )
     GCC_PRINTF( 2, 3 );

#endif                          /* #ifndef FUSE_W5100_H */
