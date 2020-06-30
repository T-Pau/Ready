/* buffer.c: Self-inflating buffer. Trivial class for convenient storing of
   arbitrary data.
   Copyright (c) 2016-2017 Fredrick Meunier

   Based on buffer.h from pzxtools.
   Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)

   The buffer.h source code is released under the MIT license, see included
   buffer_license.txt.

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <string.h>

#include "internals.h"

struct libspectrum_buffer {
  libspectrum_byte* buffer;
  size_t buffer_size;
  size_t bytes_used;
};

void
libspectrum_buffer_reallocate( libspectrum_buffer *buffer,
                               const size_t new_size )
{
  buffer->buffer = libspectrum_realloc( buffer->buffer, new_size );
  buffer->buffer_size = new_size;
}

libspectrum_buffer*
libspectrum_buffer_alloc( void )
{
  libspectrum_buffer *buffer = libspectrum_new( libspectrum_buffer, 1 );

  buffer->buffer = NULL;
  buffer->buffer_size = 0;
  buffer->bytes_used = 0;

  libspectrum_buffer_reallocate( buffer, 65536 );

  return buffer;
}

void
libspectrum_buffer_free( libspectrum_buffer *buffer )
{
  libspectrum_free( buffer->buffer );
  buffer->buffer = NULL;
  buffer->buffer_size = 0;
  buffer->bytes_used = 0;
  libspectrum_free( buffer );
}

int
libspectrum_buffer_is_empty( const libspectrum_buffer *buffer )
{
  return buffer->bytes_used == 0;
}

int
libspectrum_buffer_is_not_empty( const libspectrum_buffer *buffer )
{
  return buffer->bytes_used > 0;
}

void
libspectrum_buffer_write_byte( libspectrum_buffer *buffer,
                               const libspectrum_byte data )
{
  libspectrum_buffer_write( buffer, &data, sizeof( libspectrum_byte ) ) ;
}

void
libspectrum_buffer_write_word( libspectrum_buffer *buffer,
                               const libspectrum_word data )
{
  libspectrum_word lsb_data;
  libspectrum_byte* lsb_data_ptr = (libspectrum_byte*)&lsb_data;
  libspectrum_write_word( &lsb_data_ptr, data );
  libspectrum_buffer_write( buffer, &lsb_data, sizeof( libspectrum_word ) ) ;
}

void
libspectrum_buffer_write_dword( libspectrum_buffer *buffer,
                                const libspectrum_dword data )
{
  libspectrum_dword lsb_data;
  libspectrum_byte* lsb_data_ptr = (libspectrum_byte*)&lsb_data;
  libspectrum_write_dword( &lsb_data_ptr, data );
  libspectrum_buffer_write( buffer, &lsb_data, sizeof( libspectrum_dword ) ) ;
}

void
libspectrum_buffer_write_buffer( libspectrum_buffer *dest,
                                 const libspectrum_buffer *src )
{
  if( src ) libspectrum_buffer_write( dest, src->buffer, src->bytes_used );
}

static void
reallocate_to_new_size( libspectrum_buffer *buffer, const size_t size )
{
  while ( size > buffer->buffer_size - buffer->bytes_used ) {
    libspectrum_buffer_reallocate( buffer, 2 * buffer->buffer_size );
  }
}

void
libspectrum_buffer_write( libspectrum_buffer *buffer, const void* data,
                          const size_t size )
{
  reallocate_to_new_size( buffer, size );

  memcpy( buffer->buffer + buffer->bytes_used, data, size );

  buffer->bytes_used += size;
}

void
libspectrum_buffer_set( libspectrum_buffer *buffer, libspectrum_byte value,
                        const size_t size )
{
  reallocate_to_new_size( buffer, size );

  memset( buffer->buffer + buffer->bytes_used, value, size );

  buffer->bytes_used += size;
}

size_t
libspectrum_buffer_get_data_size( const libspectrum_buffer *buffer )
{
  return buffer ? buffer->bytes_used : 0;
}

libspectrum_byte*
libspectrum_buffer_get_data( const libspectrum_buffer *buffer )
{
  return buffer ? buffer->buffer : NULL;
}

void
libspectrum_buffer_clear( libspectrum_buffer *buffer )
{
  if( buffer ) buffer->bytes_used = 0;
}

void
libspectrum_buffer_append( libspectrum_byte **buffer, size_t *length,
                           libspectrum_byte **ptr,
                           const libspectrum_buffer *src )
{
  if( !src->bytes_used ) return;
  libspectrum_make_room( buffer, src->bytes_used, ptr, length );
  memcpy( *ptr, src->buffer, src->bytes_used );
  *ptr += src->bytes_used;
}
