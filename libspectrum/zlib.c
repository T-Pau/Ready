/* zlib.c: routines for zlib (de)compression of data
   Copyright (c) 2002 Darren Salt, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

   Darren: E-mail: linux@youmustbejoking.demon.co.uk

   Philip: E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* #ifdef HAVE_UNISTD_H */

#define ZLIB_CONST
#include <zlib.h>

#include "internals.h"

static libspectrum_error
skip_gzip_header( const libspectrum_byte **gzptr, size_t *gzlength );
static libspectrum_error
skip_null_terminated_string( const libspectrum_byte **ptr, size_t *length,
			     const char *name );
static libspectrum_error
zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
	      libspectrum_byte **outptr, size_t *outlength, int gzip_hack );

libspectrum_error 
libspectrum_zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength )
/* Inflates a block of data.
 * Input:	gzptr		-> source (deflated) data
 *		*gzlength	== source data length
 * Output:	*outptr		-> inflated data (malloced in this fn)
 *		*outlength	== length of the inflated data
 * Returns:	error flag (libspectrum_error)
 */
{
  return zlib_inflate( gzptr, gzlength, outptr, outlength, 0 );
}

libspectrum_error
libspectrum_gzip_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength )
{
  int error;

  error = skip_gzip_header( &gzptr, &gzlength ); if( error ) return error;

  return zlib_inflate( gzptr, gzlength, outptr, outlength, 1 );
}

libspectrum_error
libspectrum_zip_inflate( const libspectrum_byte *zipptr, size_t ziplength,
                         libspectrum_byte **outptr, size_t *outlength )
{
  return zlib_inflate( zipptr, ziplength, outptr, outlength, 1 );
}

static libspectrum_error
zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
	      libspectrum_byte **outptr, size_t *outlength, int gzip_hack )
{
  z_stream stream;
  int error;

  /* Use default memory management */
  stream.zalloc = Z_NULL; stream.zfree = Z_NULL; stream.opaque = Z_NULL;

  stream.next_in = gzptr; stream.avail_in = gzlength;

  if( gzip_hack ) { 

    /*
     * HACK ALERT (comment from zlib 1.1.14:gzio.c:143)
     *
     * windowBits is passed < 0 to tell that there is no zlib header.
     * Note that in this case inflate *requires* an extra "dummy" byte
     * after the compressed stream in order to complete decompression
     * and return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes
     * are present after the compressed stream.
     *
     */
    error = inflateInit2( &stream, -15 );

  } else {

    error = inflateInit( &stream );

  }

  switch( error ) {

  case Z_OK: break;

  case Z_MEM_ERROR: 
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory at %s:%d", __FILE__, __LINE__ );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "error from inflateInit2: %s", stream.msg );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  }

  if( *outlength ) {

    *outptr = libspectrum_new( libspectrum_byte, *outlength );
    stream.next_out = *outptr; stream.avail_out = *outlength;
    error = inflate( &stream, Z_FINISH );

  } else {

    *outptr = stream.next_out = NULL;
    *outlength = stream.avail_out = 0;

    do {

      libspectrum_byte *ptr;

      *outlength += 16384; stream.avail_out += 16384;
      ptr = libspectrum_renew( libspectrum_byte, *outptr, *outlength );
      stream.next_out = ptr + ( stream.next_out - *outptr );
      *outptr = ptr;

      error = inflate( &stream, 0 );

    } while( error == Z_OK );

  }

  *outlength = stream.next_out - *outptr;
  *outptr = libspectrum_renew( libspectrum_byte, *outptr, *outlength );

  switch( error ) {

  case Z_STREAM_END: break;

  case Z_NEED_DICT:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "gzip inflation needs dictionary" );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  case Z_DATA_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT, "corrupt gzip data" );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_CORRUPT;

  case Z_MEM_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "out of memory at %s:%d", __FILE__, __LINE__ );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_MEMORY;

  case Z_BUF_ERROR:
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough space in gzip output buffer" );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_CORRUPT;

  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "gzip error from inflate: %s",
			     stream.msg );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  error = inflateEnd( &stream );
  if( error != Z_OK ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "gzip error from inflateEnd: %s", stream.msg );
    libspectrum_free( *outptr );
    inflateEnd( &stream );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_gzip_header( const libspectrum_byte **gzptr, size_t *gzlength )
{
  libspectrum_byte flags;
  libspectrum_error error;

  if( *gzlength < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough data for gzip header" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( (*gzptr)[0] != 0x1f || (*gzptr)[1] != 0x8b ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "gzip header missing" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( (*gzptr)[2] != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "unknown gzip compression method %d",
			     (*gzptr)[2] );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = (*gzptr)[3];

  (*gzptr) += 10; (*gzlength) -= 10;

  if( flags & 0x04 ) {		/* extra header present */

    size_t length;

    if( *gzlength < 2 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
	"not enough data for gzip extra header length"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    length = (*gzptr)[0] + (*gzptr)[1] * 0x100;
    (*gzptr) += 2; (*gzlength) -= 2;

    if( *gzlength < length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "not enough data for gzip extra header" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

  }

  if( flags & 0x08 ) {		/* original file name present */
    error = skip_null_terminated_string( gzptr, gzlength, "original name" );
    if( error ) return error;
  }

  if( flags & 0x10 ) {		/* comment present */
    error = skip_null_terminated_string( gzptr, gzlength, "comment" );
    if( error ) return error;
  }

  if( flags & 0x02 ) {		/* header CRC present */

    if( *gzlength < 2 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "not enough data for gzip header CRC" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Could check the header CRC if we really wanted to */
    (*gzptr) += 2; (*gzptr) -= 2;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_null_terminated_string( const libspectrum_byte **ptr, size_t *length,
			     const char *name )
{
  while( **ptr && *length ) { (*ptr)++; (*length)--; }

  if( !( *length ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "not enough data for gzip %s", name );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Skip the null as well */
  (*ptr)++; (*length)--;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_zlib_compress( const libspectrum_byte *data, size_t length,
			   libspectrum_byte **gzptr, size_t *gzlength )
/* Deflates a block of data.
 * Input:	data		-> source data
 *		length		== source data length
 * Output:	*gzptr		-> deflated data (malloced in this fn),
 *		*gzlength	== length of the deflated data
 * Returns:	error flag (libspectrum_error)
 */
{
  uLongf gzl = (uLongf)( length * 1.001 ) + 12;
  int gzret;

  *gzptr = libspectrum_new( libspectrum_byte, gzl );
  gzret = compress2( *gzptr, &gzl, data, length, Z_BEST_COMPRESSION );

  switch (gzret) {

  case Z_OK:			/* initialised OK */
    *gzlength = gzl;
    return LIBSPECTRUM_ERROR_NONE;

  case Z_MEM_ERROR:		/* out of memory */
    libspectrum_free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			     "libspectrum_zlib_compress: out of memory" );
    return LIBSPECTRUM_ERROR_MEMORY;

  case Z_VERSION_ERROR:		/* unrecognised version */
    libspectrum_free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "libspectrum_zlib_compress: unknown version" );
    return LIBSPECTRUM_ERROR_UNKNOWN;

  case Z_BUF_ERROR:		/* Not enough space in output buffer.
				   Shouldn't happen */
    libspectrum_free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_zlib_compress: out of space?" ); 
    return LIBSPECTRUM_ERROR_LOGIC;

  default:			/* some other error */
    libspectrum_free( *gzptr ); *gzptr = 0;
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "libspectrum_zlib_compress: unexpected error?" ); 
    return LIBSPECTRUM_ERROR_LOGIC;
  }
}
