/* microdrive.c: Routines for handling microdrive images
   Copyright (c) 2004-2015 Philip Kendall

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

#include "internals.h"

/* The type itself */

struct libspectrum_microdrive {

  libspectrum_byte data[ LIBSPECTRUM_MICRODRIVE_CARTRIDGE_LENGTH ];
  int write_protect;
  libspectrum_byte cartridge_len;

};

typedef struct libspectrum_microdrive_block {

  libspectrum_byte hdflag;		/* bit0 = 1-head, ( 0 - data ) */
  libspectrum_byte hdbnum;		/* block num 1 -- 254 */
  libspectrum_word hdblen;		/* not used */
  libspectrum_byte hdbnam[11];		/* cartridge label + \0 */
  libspectrum_byte hdchks;		/* header checksum */
 
  libspectrum_byte recflg;		/* bit0 = 0-data, bit1, bit2 */
  libspectrum_byte recnum;		/* data block num  */
  libspectrum_word reclen;		/* block length 0 -- 512 */
  libspectrum_byte recnam[11];		/* record (file) name + \0 */
  libspectrum_byte rechks;		/* descriptor checksum */

  libspectrum_byte data[512];		/* data bytes */
  libspectrum_byte datchk;		/* data checksum */

} libspectrum_microdrive_block;

static const size_t MDR_LENGTH = LIBSPECTRUM_MICRODRIVE_CARTRIDGE_LENGTH + 1;

/* Constructor/destructor */

/* Allocate a microdrive image */
libspectrum_microdrive*
libspectrum_microdrive_alloc( void )
{
  return libspectrum_new( libspectrum_microdrive, 1 );
}

/* Free a microdrive image */
libspectrum_error
libspectrum_microdrive_free( libspectrum_microdrive *microdrive )
{
  libspectrum_free( microdrive );

  return LIBSPECTRUM_ERROR_NONE;
}

/* Accessors */

libspectrum_byte
libspectrum_microdrive_data( const libspectrum_microdrive *microdrive,
			     size_t which )
{
  return microdrive->data[ which ];
}

void
libspectrum_microdrive_set_data( libspectrum_microdrive *microdrive,
				 size_t which, libspectrum_byte data )
{
  microdrive->data[ which ] = data;
}

int
libspectrum_microdrive_write_protect( const libspectrum_microdrive *microdrive )
{
  return microdrive->write_protect;
}

void
libspectrum_microdrive_set_write_protect( libspectrum_microdrive *microdrive,
					  int write_protect )
{
  microdrive->write_protect = write_protect;
}

libspectrum_byte
libspectrum_microdrive_cartridge_len( const libspectrum_microdrive *microdrive )
{
  return microdrive->cartridge_len;
}

void
libspectrum_microdrive_set_cartridge_len( libspectrum_microdrive *microdrive,
			     libspectrum_byte len )
{
  microdrive->cartridge_len = len;
}

void
libspectrum_microdrive_get_block( const libspectrum_microdrive *microdrive,
				  libspectrum_byte which,
				  libspectrum_microdrive_block *block )
{
  const libspectrum_byte *d;
  
  d = microdrive->data + which * LIBSPECTRUM_MICRODRIVE_BLOCK_LEN;

  /* The block header */
  block->hdflag = *(d++);
  block->hdbnum = *(d++);
  block->hdblen = *d + ( *( d + 1 ) << 8 ); d += 2;
  memcpy( block->hdbnam, d, 10 ); d += 10; block->hdbnam[10] = '\0';
  block->hdchks = *(d++);

  /* The data header */
  block->recflg = *(d++);
  block->recnum = *(d++);
  block->reclen = *d + ( *( d + 1 ) << 8 ); d += 2;
  memcpy( block->recnam, d, 10 ); d += 10; block->recnam[10] = '\0';
  block->rechks = *(d++);

  /* The data itself */
  memcpy( block->data, d, 512 ); d += 512;
  block->datchk = *(d++);

}

void
libspectrum_microdrive_set_block( libspectrum_microdrive *microdrive,
				  libspectrum_byte which,
				  libspectrum_microdrive_block *block )
{
  libspectrum_byte *d = &microdrive->data[ which * 
					   LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ];
  /* The block header */
  *(d++) = block->hdflag;
  *(d++) = block->hdbnum;
  *(d++) = block->hdblen & 0xff; *(d++) = block->hdblen >> 8;
  memcpy( d, block->hdbnam, 10 ); d += 10;
  *(d++) = block->hdchks;

  /* The data header */
  *(d++) = block->recflg;
  *(d++) = block->recnum;
  *(d++) = block->reclen & 0xff; *(d++) = block->reclen >> 8;
  memcpy( d, block->recnam, 10 ); d += 10;
  *(d++) = block->rechks;

  /* The data itself */
  memcpy( d, block->data, 512 ); d += 512;
  *(d++) = block->datchk;

}

int
libspectrum_microdrive_checksum( libspectrum_microdrive *microdrive,
				 libspectrum_byte what )
{
  libspectrum_byte *data;
  unsigned int checksum, carry;
  int i;

#define DO_CHECK \
    checksum += *data;		/* LD    A,E */ \
				/* ADD A, (HL) */ \
    if( checksum > 255 ) {	/* ... ADD A, (HL) --> CY*/ \
      carry = 1;		\
      checksum -= 256;		\
    } else {			\
      carry = 0;		\
    }				\
    data++;			/* INC HL */ \
    checksum += carry + 1;	/* ADC A, #01 */ \
    if( checksum == 256 ) {	/* JR Z,LSTCHK */ \
      checksum -= 256;		/* LD E,A */\
    } else {			\
      checksum--;		/* DEC A */ \
    }

/*

LOOP-C:  LD      A,E             ; fetch running sum
         ADD     A,(HL)          ; add to current location.
         INC     HL              ; point to next location.


         ADC     A,#01           ; avoid the value #FF.
         JR      Z,LSTCHK        ; forward to STCHK

         DEC     A               ; decrement.

LSTCHK   LD      E,A             ; update the 8-bit sum.

*/  
  data = &(microdrive->data[ what * LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ]);
  
  if( ( *( data + 15 ) & 2 ) && 
        ( *( data + 17 ) == 0 ) && ( *( data + 18 ) == 0 ) ) {
    return -1;		/* PRESET BAD BLOCK */
  }

  checksum = 0;			/* Block header */
  for( i = LIBSPECTRUM_MICRODRIVE_HEAD_LEN; i > 1; i-- ) {
    DO_CHECK;
  }
  if( *(data++) != checksum ) 
    return 1;

  checksum = 0;			/* Record header */
  for( i = LIBSPECTRUM_MICRODRIVE_HEAD_LEN; i > 1; i-- ) {
    DO_CHECK;
  }
  if( *(data++) != checksum ) 
    return 2;

  if( ( *( data - 13 ) == 0 ) && ( *( data - 12 ) == 0 ) ) {
    return 0;		/* Erased / empty block: data checksum irrelevant */
  }

  checksum = 0;			/* Data */
  for( i = LIBSPECTRUM_MICRODRIVE_DATA_LEN; i > 0; i-- ) {
    DO_CHECK;
  }
  if( *data != checksum )
    return 3;

  return 0;
}

/* .mdr format routines */

libspectrum_error
libspectrum_microdrive_mdr_read( libspectrum_microdrive *microdrive,
				 libspectrum_byte *buffer, size_t length )
{
  size_t data_length;

  if( length < LIBSPECTRUM_MICRODRIVE_BLOCK_LEN * 10 ||
     ( length % LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ) > 1 ||
       length > MDR_LENGTH ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_microdrive_mdr_read: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  data_length = length - ( length % LIBSPECTRUM_MICRODRIVE_BLOCK_LEN );

  memcpy( microdrive->data, buffer, data_length ); buffer += data_length;

  if( ( length % LIBSPECTRUM_MICRODRIVE_BLOCK_LEN ) == 1 )
    libspectrum_microdrive_set_write_protect( microdrive, *buffer );
  else
    libspectrum_microdrive_set_write_protect( microdrive, 0 );

  libspectrum_microdrive_set_cartridge_len( microdrive,
			      data_length / LIBSPECTRUM_MICRODRIVE_BLOCK_LEN );
 
  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_microdrive_mdr_write( const libspectrum_microdrive *microdrive,
				  libspectrum_byte **buffer, size_t *length )
{
  *length = microdrive->cartridge_len * LIBSPECTRUM_MICRODRIVE_BLOCK_LEN;
  *buffer = libspectrum_new( libspectrum_byte, *length + 1 );

  memcpy( *buffer, microdrive->data, *length );

  (*buffer)[ *length ] = microdrive->write_protect;

  (*length)++;
}
