/* w5100.c: Wiznet W5100 emulation - main code
   
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

#include "fuse.h"
#include "ui/ui.h"
#include "w5100.h"
#include "w5100_internals.h"

enum w5100_registers {
  W5100_MR = 0x000,

  W5100_GWR0,
  W5100_GWR1,
  W5100_GWR2,
  W5100_GWR3,
  
  W5100_SUBR0,
  W5100_SUBR1,
  W5100_SUBR2,
  W5100_SUBR3,

  W5100_SHAR0,
  W5100_SHAR1,
  W5100_SHAR2,
  W5100_SHAR3,
  W5100_SHAR4,
  W5100_SHAR5,

  W5100_SIPR0,
  W5100_SIPR1,
  W5100_SIPR2,
  W5100_SIPR3,

  W5100_IMR = 0x016,

  W5100_RMSR = 0x01a,
  W5100_TMSR,
};

void
nic_w5100_reset( nic_w5100_t *self )
{
  size_t i;

  nic_w5100_debug( "w5100: reset\n" );

  memset( self->gw, 0, sizeof( self->gw ) );
  memset( self->sub, 0, sizeof( self->sub ) );
  memset( self->sha, 0, sizeof( self->sha ) );
  memset( self->sip, 0, sizeof( self->sip ) );

  for( i = 0; i < 4; i++ )
    nic_w5100_socket_reset( &self->socket[i] );
}

static void*
w5100_io_thread( void *arg )
{
  nic_w5100_t *self = arg;
  int i;

  while( !self->stop_io_thread ) {
    fd_set readfds, writefds;
    int active;
    compat_socket_t selfpipe_socket =
      compat_socket_selfpipe_get_read_fd( self->selfpipe );
    int max_fd = selfpipe_socket;

    FD_ZERO( &readfds );
    FD_ZERO( &writefds );

    FD_SET( selfpipe_socket, &readfds );

    for( i = 0; i < 4; i++ )
      nic_w5100_socket_add_to_sets( &self->socket[i], &readfds, &writefds,
        &max_fd );

    /* Note that if a socket is closed between when we added it to the sets
       above and when we call select() below, it will cause the select to fail
       with EBADF. We catch this and just run around the loop again - the
       offending socket will not be added to the sets again as it's now been
       closed */

    nic_w5100_debug( "w5100: io thread select\n" );

    active = select( max_fd + 1, &readfds, &writefds, NULL, NULL );

    nic_w5100_debug( "w5100: io thread wake; %d active\n", active );

    if( active != -1 ) {
      if( FD_ISSET( selfpipe_socket, &readfds ) ) {
        nic_w5100_debug( "w5100: discarding selfpipe data\n" );
        compat_socket_selfpipe_discard_data( self->selfpipe );
      }

      for( i = 0; i < 4; i++ )
        nic_w5100_socket_process_io( &self->socket[i], readfds, writefds );
    }
    else if( compat_socket_get_error() == compat_socket_EBADF ) {
      /* Do nothing - just loop again */
    }
    else {
      nic_w5100_debug( "w5100: select returned unexpected errno %d: %s\n",
                       compat_socket_get_error(),
                       compat_socket_get_strerror() );
    }
  }

  return NULL;
}

nic_w5100_t*
nic_w5100_alloc( void )
{
  int error;
  int i;
  nic_w5100_t *self;
  
  compat_socket_networking_init();

  self = libspectrum_new( nic_w5100_t, 1 );

  self->selfpipe = compat_socket_selfpipe_alloc();

  for( i = 0; i < 4; i++ )
    nic_w5100_socket_init( &self->socket[i], i );

  nic_w5100_reset( self );

  self->stop_io_thread = 0;

  error = pthread_create( &self->thread, NULL, w5100_io_thread, self );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "w5100: error %d creating thread", error );
    fuse_abort();
  }

  return self;
}

void
nic_w5100_free( nic_w5100_t *self )
{
  int i;

  if( self ) {
    self->stop_io_thread = 1;
    compat_socket_selfpipe_wake( self->selfpipe );

    pthread_join( self->thread, NULL );

    for( i = 0; i < 4; i++ )
      nic_w5100_socket_end( &self->socket[i] );

    compat_socket_selfpipe_free( self->selfpipe );

    compat_socket_networking_end();

    libspectrum_free( self );
  }
}

libspectrum_byte
nic_w5100_read( nic_w5100_t *self, libspectrum_word reg )
{
  libspectrum_byte b;

  if( reg < 0x030 ) {
    switch( reg ) {
      case W5100_MR:
        /* We don't support any flags, so we always return zero here */
        b = 0x00;
        nic_w5100_debug( "w5100: reading 0x%02x from MR\n", b );
        break;
      case W5100_GWR0: case W5100_GWR1: case W5100_GWR2: case W5100_GWR3:
        b = self->gw[reg - W5100_GWR0];
        nic_w5100_debug( "w5100: reading 0x%02x from GWR%d\n", b, reg - W5100_GWR0 );
        break;
      case W5100_SUBR0: case W5100_SUBR1: case W5100_SUBR2: case W5100_SUBR3:
        b = self->sub[reg - W5100_SUBR0];
        nic_w5100_debug( "w5100: reading 0x%02x from SUBR%d\n", b, reg - W5100_SUBR0 );
        break;
      case W5100_SHAR0: case W5100_SHAR1: case W5100_SHAR2:
      case W5100_SHAR3: case W5100_SHAR4: case W5100_SHAR5:
        b = self->sha[reg - W5100_SHAR0];
        nic_w5100_debug( "w5100: reading 0x%02x from SHAR%d\n", b, reg - W5100_SHAR0 );
        break;
      case W5100_SIPR0: case W5100_SIPR1: case W5100_SIPR2: case W5100_SIPR3:
        b = self->sip[reg - W5100_SIPR0];
        nic_w5100_debug( "w5100: reading 0x%02x from SIPR%d\n", b, reg - W5100_SIPR0 );
        break;
      case W5100_IMR:
        /* We support only "allow all" */
        b = 0xef;
        nic_w5100_debug( "w5100: reading 0x%02x from IMR\n", b );
        break;
      case W5100_RMSR: case W5100_TMSR:
        /* We support only 2K per socket */
        b = 0x55;
        nic_w5100_debug( "w5100: reading 0x%02x from %s\n", b, reg == W5100_RMSR ? "RMSR" : "TMSR" );
        break;
      default:
        b = 0xff;
        /* This is a debug rather than a warning because it happens on snapshot save */
        nic_w5100_debug( "w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
        break;
    }
  }
  else if( reg >= 0x400 && reg < 0x800 ) {
    b = nic_w5100_socket_read( self, reg );
  }
  else if( reg >= 0x6000 && reg < 0x8000 ) {
    b = nic_w5100_socket_read_rx_buffer( self, reg );
  }
  else {
    b = 0xff;
    nic_w5100_error( UI_ERROR_WARNING,
      "w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
  }

  return b;
}

static void
w5100_write_mr( nic_w5100_t *self, libspectrum_byte b )
{
  nic_w5100_debug( "w5100: writing 0x%02x to MR\n", b );

  if( b & 0x80 )
    nic_w5100_reset( self );

  if( b & 0x7f )
    nic_w5100_error( UI_ERROR_WARNING,
                     "w5100: unsupported value 0x%02x written to MR\n", b );
}

static void
w5100_write_imr( nic_w5100_t *self, libspectrum_byte b )
{
  nic_w5100_debug( "w5100: writing 0x%02x to IMR\n", b );

  if( b != 0xef )
    nic_w5100_error( UI_ERROR_WARNING, 
                     "w5100: unsupported value 0x%02x written to IMR\n", b );
}
  

static void
w5100_write__msr( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  const char *regname = reg == W5100_RMSR ? "RMSR" : "TMSR";

  nic_w5100_debug( "w5100: writing 0x%02x to %s\n", b, regname );

  if( b != 0x55 )
    nic_w5100_error( UI_ERROR_WARNING, 
                     "w5100: unsupported value 0x%02x written to %s\n",
                     b, regname );
}

void
nic_w5100_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  if( reg < 0x030 ) {
    switch( reg ) {
      case W5100_MR:
        w5100_write_mr( self, b );
        break;
      case W5100_GWR0: case W5100_GWR1: case W5100_GWR2: case W5100_GWR3:
        nic_w5100_debug( "w5100: writing 0x%02x to GWR%d\n", b, reg - W5100_GWR0 );
        self->gw[reg - W5100_GWR0] = b;
        break;
      case W5100_SUBR0: case W5100_SUBR1: case W5100_SUBR2: case W5100_SUBR3:
        nic_w5100_debug( "w5100: writing 0x%02x to SUBR%d\n", b, reg - W5100_SUBR0 );
        self->sub[reg - W5100_SUBR0] = b;
        break;
      case W5100_SHAR0: case W5100_SHAR1: case W5100_SHAR2:
      case W5100_SHAR3: case W5100_SHAR4: case W5100_SHAR5:
        nic_w5100_debug( "w5100: writing 0x%02x to SHAR%d\n", b, reg - W5100_SHAR0 );
        self->sha[reg - W5100_SHAR0] = b;
        break;
      case W5100_SIPR0: case W5100_SIPR1: case W5100_SIPR2: case W5100_SIPR3:
        nic_w5100_debug( "w5100: writing 0x%02x to SIPR%d\n", b, reg - W5100_SIPR0 );
        self->sip[reg - W5100_SIPR0] = b;
        break;
      case W5100_IMR:
        w5100_write_imr( self, b );
        break;
      case W5100_RMSR: case W5100_TMSR:
        w5100_write__msr( self, reg, b );
        break;
      default:
        /* This is a debug rather than a warning because it happens on snapshot load */
        nic_w5100_debug( "w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg );
        break;
    }
  }
  else if( reg >= 0x400 && reg < 0x800 ) {
    nic_w5100_socket_write( self, reg, b );
  }
  else if( reg >= 0x4000 && reg < 0x6000 ) {
    nic_w5100_socket_write_tx_buffer( self, reg, b );
  }
  else
    nic_w5100_error( UI_ERROR_WARNING, 
                     "w5100: writing 0x%02x to unsupported register 0x%03x\n",
                     b, reg );
}

void
nic_w5100_from_snapshot( nic_w5100_t *self, libspectrum_byte *data )
{
  int i;

  for( i = 0; i < 0x30; i++ )
    nic_w5100_write( self, i, data[i] );
}

libspectrum_byte*
nic_w5100_to_snapshot( nic_w5100_t *self )
{
  libspectrum_byte *data = libspectrum_new( libspectrum_byte, 0x30 );
  int i;

  for( i = 0; i < 0x30; i++ )
    data[i] = nic_w5100_read( self, i );

  return data;
}

void
nic_w5100_debug( const char *format, ... )
{
  if( W5100_DEBUG ) {
    va_list ap;
    va_start( ap, format );
    vprintf( format, ap );
    va_end( ap );
  }
}

void
nic_w5100_vdebug( const char *format, va_list ap )
{
  if( W5100_DEBUG ) {
    vprintf( format, ap );
  }
}

void
nic_w5100_error( int severity, const char *format, ... )
{
  va_list ap;

  va_start( ap, format );
  nic_w5100_vdebug( format, ap );
  va_end( ap );
}
