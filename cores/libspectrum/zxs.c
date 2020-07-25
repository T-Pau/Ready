/* zxs.c: Routines for .zxs snapshots
   Copyright (c) 1998,2003,2015 Philip Kendall

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

#include <string.h>

#ifdef HAVE_ZLIB_H
#define ZLIB_CONST
#include <zlib.h>
#endif				/* #ifdef HAVE_ZLIB_H */

#include "internals.h"

static libspectrum_error
read_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	    const libspectrum_byte *end );

typedef libspectrum_error (*read_chunk_fn)( libspectrum_snap *snap,
					    int *compression,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length,
					    int parameter );

static libspectrum_error
inflate_block( libspectrum_byte **uncompressed, size_t *uncompressed_length,
	       const libspectrum_byte **compressed, size_t compressed_length )
{

#ifdef HAVE_ZLIB_H

  libspectrum_dword header_length, expected_crc32, actual_crc32;
  libspectrum_byte *zlib_buffer;
  unsigned long actual_length;
  int error;

  /* First, look at the compression header */
  header_length = libspectrum_read_dword( compressed );
  if( header_length != 12 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_inflate_block: unknown header length %lu",
			     (unsigned long)header_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  compressed_length -= 12;

  expected_crc32 = libspectrum_read_dword( compressed );
  *uncompressed_length = libspectrum_read_dword( compressed );

  /* Some space to put the zlib header for decompression */
  zlib_buffer = libspectrum_new( libspectrum_byte, ( compressed_length + 6 ) );

  /* zlib's header */
  zlib_buffer[0] = 0x78; zlib_buffer[1] = 0xda;

  memcpy( &zlib_buffer[2], *compressed, compressed_length );
  *compressed += compressed_length;

  *uncompressed = libspectrum_new( libspectrum_byte, *uncompressed_length );

  actual_length = *uncompressed_length;
  error = uncompress( *uncompressed, &actual_length, zlib_buffer,
		      compressed_length + 6 );

  /* At this point, we expect to get a Z_DATA_ERROR, as we don't have
     the Adler-32 checksum of the data. There is a 1 in 65521 chance
     that the random bytes will match, so we might (very rarely) get
     Z_OK */
  if( error != Z_DATA_ERROR && error != Z_OK ) {
    libspectrum_free( *uncompressed ); libspectrum_free( zlib_buffer );
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "zxs_inflate_block: unexpected zlib error" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( *uncompressed_length != actual_length ) {
    libspectrum_free( *uncompressed ); libspectrum_free( zlib_buffer );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "zxs_inflate_block: block expanded to 0x%04lx, not the expected 0x%04lx bytes",
      actual_length, (unsigned long)*uncompressed_length
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_free( zlib_buffer );

  actual_crc32 = crc32( 0, Z_NULL, 0 );
  actual_crc32 = crc32( actual_crc32, *uncompressed, *uncompressed_length );

  if( actual_crc32 != expected_crc32 ) {
    libspectrum_free( *uncompressed );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "zxs_inflate_block: crc 0x%08x does not match expected 0x%08x",
      actual_crc32, expected_crc32
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  return LIBSPECTRUM_ERROR_NONE;

#else				/* #ifdef HAVE_ZLIB_H */

  /* No zlib, so can't inflate the block */
  return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

}

static libspectrum_error
read_riff_chunk( libspectrum_snap *snap, int *compression GCC_UNUSED,
		 const libspectrum_byte **buffer, const libspectrum_byte *end,
		 size_t data_length GCC_UNUSED, int parameter GCC_UNUSED)
{
  char id[5];
  libspectrum_error error;

  if( end - *buffer < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "zxs_read_riff_chunk: not enough data for form type"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;

  if( strcmp( id, "SNAP" ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_riff_chunk: unknown form type '%s'",
			     id );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  while( *buffer < end ) {
    error = read_chunk( snap, buffer, end );
    if( error ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_fmtz_chunk( libspectrum_snap *snap, int *compression,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED,
		 size_t data_length, int parameter GCC_UNUSED )
{
  libspectrum_word model;

  if( data_length != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_fmtz_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *buffer += 2;			/* Skip version number */
  
  model = libspectrum_read_word( buffer );

  switch( model ) {

  case 0x0010:  case 0x0020:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );
    break;

  case 0x0030:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_128 );
    break;
    
  case 0x0040:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2 );
    break;
    
  case 0x0050:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS2A );
    break;
    
  case 0x0060:
    libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_PLUS3 );
    break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "zxs_read_fmtz_chunk: unknown machine type 0x%04x", model
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *buffer += 2;			/* Skip hardware flags */

  *compression = libspectrum_read_word( buffer );

  switch( *compression ) {

  case 0x0008:			/* Deflation */
    *compression = 1; break;

  case 0xffff:			/* Not compressed */
    *compression = 0; break;

  default:
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "zxs_read_fmtz_chunk: unknown compression type 0x%04x", *compression
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_rz80_chunk( libspectrum_snap *snap, int *compression GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED,
		 size_t data_length, int parameter GCC_UNUSED )
{
  if( data_length != 33 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_rZ80_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_a   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_f   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_bc  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_a_  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_f_  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_bc_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl_ ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_ix  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_iy  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_pc  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_sp  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_i   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_r   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff1( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff2( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_im  ( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_tstates( snap, libspectrum_read_dword( buffer ) );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_r048_chunk( libspectrum_snap *snap, int *compression GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED,
		 size_t data_length, int parameter GCC_UNUSED )
{
  if( data_length != 9 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "zxs_read_r048_chunk: unknown length %lu", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_ula( snap, **buffer ); (*buffer)++;

  *buffer += 8;			/* Skip key data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_r128_chunk( libspectrum_snap *snap, int *compression GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED,
		 size_t data_length, int parameter GCC_UNUSED )
{
  size_t i;

  if( data_length != 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_r128_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_128_memoryport( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_ay_registerport( snap, **buffer ); (*buffer)++;

  for( i = 0; i < 16; i++ ) {
    libspectrum_snap_set_ay_registers( snap, i, **buffer ); (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_rplus3_chunk( libspectrum_snap *snap, int *compression GCC_UNUSED,
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end GCC_UNUSED, size_t data_length,
		   int parameter GCC_UNUSED )
{
  if( data_length != 1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_rplus3_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_plus3_memoryport( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ram_chunk( libspectrum_snap *snap, int *compression,
		const libspectrum_byte **buffer,
		const libspectrum_byte *end GCC_UNUSED,
		size_t data_length, int parameter )
{
  int page = parameter;
  libspectrum_byte *buffer2; size_t uncompressed_length;
  libspectrum_error error;

  if( *compression ) {

    error = inflate_block( &buffer2, &uncompressed_length,
			   buffer, data_length );
    if( error ) return error;

    if( uncompressed_length != 0x4000 ) {
      libspectrum_free( buffer2 );
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
	"zxs_read_ram_chunk: page %d does not expand to 0x4000 bytes", page
      );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

  } else {			/* Uncompressed data */

    if( data_length != 0x4000 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
	"zxs_read_ram_chunk: page %d has unknown length %lu", page,
	(unsigned long)data_length
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    buffer2 = libspectrum_new( libspectrum_byte, 0x4000 );
    memcpy( buffer2, buffer, 0x4000 ); *buffer += 0x4000;
  }

  libspectrum_snap_set_pages( snap, page, buffer2 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_chunk( libspectrum_snap *snap GCC_UNUSED, int *compression GCC_UNUSED,
	    const libspectrum_byte **buffer,
	    const libspectrum_byte *end GCC_UNUSED,
	    size_t data_length, int parameter GCC_UNUSED )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_chunk_t {

  const char *id;
  read_chunk_fn function;
  int parameter;

};

static struct read_chunk_t read_chunks[] = {

  { "RIFF", read_riff_chunk,   0 },
  { "fmtz", read_fmtz_chunk,   0 },
  { "rZ80", read_rz80_chunk,   0 },
  { "r048", read_r048_chunk,   0 },
  { "r128", read_r128_chunk,   0 },
  { "r+3 ", read_rplus3_chunk, 0 },

  { "ram0", read_ram_chunk,    0 },
  { "ram1", read_ram_chunk,    1 },
  { "ram2", read_ram_chunk,    2 },
  { "ram3", read_ram_chunk,    3 },
  { "ram4", read_ram_chunk,    4 },
  { "ram5", read_ram_chunk,    5 },
  { "ram6", read_ram_chunk,    6 },
  { "ram7", read_ram_chunk,    7 },

  { "LIST", skip_chunk,        0 },

};

static libspectrum_error
read_chunk_header( char *id, libspectrum_dword *data_length, 
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end )
{
  if( end - *buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "zxs_read_chunk_header: not enough data for chunk header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;
  *data_length = libspectrum_read_dword( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_chunk( libspectrum_snap *snap, const libspectrum_byte **buffer,
	    const libspectrum_byte *end )
{
  char id[5];
  libspectrum_dword data_length;
  libspectrum_error error;
  size_t i; int done;
  int compression;

  error = read_chunk_header( id, &data_length, buffer, end );
  if( error ) return error;

  if( *buffer + data_length > end ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "zxs_read_chunk: chunk length goes beyond end of file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  done = 0;

  for( i = 0; !done && i < ARRAY_SIZE( read_chunks ); i++ ) {

    if( !strcmp( id, read_chunks[i].id ) ) {
      error = read_chunks[i].function( snap, &compression, buffer, end,
				       data_length, read_chunks[i].parameter );
      if( error ) return error;
      done = 1;
    }

  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "zxs_read_chunk: unknown chunk id '%s'", id );
    *buffer += data_length;
  }

  if( data_length % 2 ) (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_zxs_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_error error;

  /* Set machine type in case it's not set later */
  libspectrum_snap_set_machine( snap, LIBSPECTRUM_MACHINE_48 );

  error = read_chunk( snap, &buffer, buffer + length );
  if( error ) {

    /* Tidy up any RAM pages we may have allocated */
    size_t i;

    for( i = 0; i < 8; i++ ) {
      libspectrum_byte *page = libspectrum_snap_pages( snap, i );
      if( page ) {
	libspectrum_free( page );
	libspectrum_snap_set_pages( snap, i, NULL );
      }
    }

    return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}
