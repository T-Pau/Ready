/* dck.c: Routines for handling Warajevo DCK files
   Copyright (c) 2003-2015 Darren Salt, Fredrick Meunier

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

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "internals.h"

static const int DCK_PAGE_SIZE = 0x2000;

/* Initialise a libspectrum_dck_block structure */
static void
libspectrum_dck_block_alloc( libspectrum_dck_block **dck )
{
  size_t i;

  *dck = libspectrum_new( libspectrum_dck_block, 1 );

  (*dck)->bank = LIBSPECTRUM_DCK_BANK_DOCK;
  for( i = 0; i < 8; i++ ) {
    (*dck)->access[i] = LIBSPECTRUM_DCK_PAGE_NULL;
    (*dck)->pages[i] = NULL;
  }
}

/* Free all memory used by a libspectrum_dck_block structure */
static libspectrum_error
libspectrum_dck_block_free( libspectrum_dck_block *dck, int keep_pages )
{
  size_t i;

  if( !keep_pages )
    for( i = 0; i < 8; i++ )
      if( dck->pages[i] )
        libspectrum_free( dck->pages[i] );

  libspectrum_free( dck );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Initialise a libspectrum_dck structure (constructor!) */
libspectrum_dck*
libspectrum_dck_alloc( void )
{
  libspectrum_dck *dck = libspectrum_new( libspectrum_dck, 1 );
  size_t i;
  for( i=0; i<256; i++ ) dck->dck[i] = NULL;
  return dck;
}

/* Free all memory used by a libspectrum_dck structure (destructor...) */
libspectrum_error
libspectrum_dck_free( libspectrum_dck *dck, int keep_pages )
{
  size_t i;

  for( i=0; i<256; i++ )
    if( dck->dck[i] ) {
      libspectrum_dck_block_free( dck->dck[i], keep_pages );
      dck->dck[i] = NULL;
    }

  return LIBSPECTRUM_ERROR_NONE;
}

/* Read in the DCK file */
libspectrum_error
libspectrum_dck_read( libspectrum_dck *dck, const libspectrum_byte *buffer,
		      const size_t length )
{
  return libspectrum_dck_read2( dck, buffer, length, NULL );
}

libspectrum_error
libspectrum_dck_read2( libspectrum_dck *dck, const libspectrum_byte *buffer,
		       const size_t clength, const char *filename )
{
  int i;
  int num_dck_block = 0;
  libspectrum_error error;
  const libspectrum_byte *end;
  libspectrum_id_t raw_type;
  libspectrum_class_t class;
  libspectrum_byte *new_buffer;
  size_t length;

  /* Ugly but necessary to get around the fact that 'clength' is a const
     parameter, but we now modify it */
  length = clength;

  /* For storing the uncompressed data in if the input file was
     compressed */
  new_buffer = NULL;

  error = libspectrum_identify_file_raw( &raw_type, filename, buffer, length );
  if( error ) return error;

  error = libspectrum_identify_class( &class, raw_type );
  if( error ) return error;

  if( class == LIBSPECTRUM_CLASS_COMPRESSED ) {

    size_t new_length;

    error = libspectrum_uncompress_file( &new_buffer, &new_length, NULL,
                                         raw_type, buffer, length, NULL );
    if( error ) return error;
    buffer = new_buffer; length = new_length;
  }

  end = buffer + length;

  for( i=0; i<256; i++ ) dck->dck[i]=NULL;

  while( buffer < end ) {
    int pages = 0;

    if( buffer + 9 > end ) {
      libspectrum_print_error( 
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_dck_read: not enough data in buffer"
      );
      error = LIBSPECTRUM_ERROR_CORRUPT;
      goto end;
    }

    switch( buffer[0] ) {
    case LIBSPECTRUM_DCK_BANK_DOCK:
    case LIBSPECTRUM_DCK_BANK_EXROM:
    case LIBSPECTRUM_DCK_BANK_HOME:
      break;
    default:
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "libspectrum_dck_read: unknown bank ID %d",
			       buffer[0] );
      error = LIBSPECTRUM_ERROR_UNKNOWN;
      goto end;
    }

    for( i = 1; i < 9; i++ )
      switch( buffer[i] ) {
      case LIBSPECTRUM_DCK_PAGE_NULL:
      case LIBSPECTRUM_DCK_PAGE_RAM_EMPTY:
        break;
      case LIBSPECTRUM_DCK_PAGE_ROM:
      case LIBSPECTRUM_DCK_PAGE_RAM:
        pages++;
        break;
      default:
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "libspectrum_dck_read: unknown page type %d",
				 buffer[i] );
        error = LIBSPECTRUM_ERROR_UNKNOWN;
	goto end;
      }

    if( buffer + 9 + DCK_PAGE_SIZE * pages > end ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "libspectrum_dck_read: not enough data in buffer"
      );
      error = LIBSPECTRUM_ERROR_CORRUPT;
      goto end;
    }

    /* Allocate new dck_block */
    libspectrum_dck_block_alloc( &dck->dck[num_dck_block] );

    /* Copy the bank ID */
    dck->dck[num_dck_block]->bank = *buffer++;
    /* Copy the page types */
    for( i = 0; i < 8; i++ ) dck->dck[num_dck_block]->access[i] = *buffer++;

    /* Allocate the pages */
    for( i = 0; i < 8; i++ ) {
      switch( dck->dck[num_dck_block]->access[i] ) {
      case LIBSPECTRUM_DCK_PAGE_NULL:
        break;
      case LIBSPECTRUM_DCK_PAGE_RAM_EMPTY:
        dck->dck[num_dck_block]->pages[i] =
                        libspectrum_new0( libspectrum_byte, DCK_PAGE_SIZE );
        if( !dck->dck[num_dck_block]->pages[i] ) {
          libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
                                   "libspectrum_dck_read: out of memory" );
          error = LIBSPECTRUM_ERROR_MEMORY;
	  goto end;
        }
        break;
      case LIBSPECTRUM_DCK_PAGE_ROM:
      case LIBSPECTRUM_DCK_PAGE_RAM:
        dck->dck[num_dck_block]->pages[i] =
	  libspectrum_new( libspectrum_byte, DCK_PAGE_SIZE );
        memcpy( dck->dck[num_dck_block]->pages[i], buffer, DCK_PAGE_SIZE );
        buffer += DCK_PAGE_SIZE;
        break;
      }
    }

    num_dck_block++;
    if( num_dck_block == 256 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_MEMORY,
			       "libspectrum_dck_read: more than 256 banks" );
      error = LIBSPECTRUM_ERROR_MEMORY;
      goto end;
    }
  }

  /* Finished successfully */
  error = LIBSPECTRUM_ERROR_NONE;

  end:

  libspectrum_free( new_buffer );
  return error;
}
