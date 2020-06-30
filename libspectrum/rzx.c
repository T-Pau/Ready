/* rzx.c: routines for dealing with .rzx files
   Copyright (c) 2002-2015 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strcasecmp() on QNX6 */
#endif				/* #ifdef HAVE_STRINGS_H */

#ifdef HAVE_GCRYPT_H
#include <gcrypt.h>
#endif				/* #ifdef HAVE_GCRYPT_H */

#include "internals.h"

/* The strings used for each snapshot type */
typedef struct snapshot_string_t {
  libspectrum_id_t format;
  const char *string;
} snapshot_string_t;

static snapshot_string_t snapshot_strings[] = {
  { LIBSPECTRUM_ID_SNAPSHOT_SNA, "SNA" },
  { LIBSPECTRUM_ID_SNAPSHOT_SZX, "SZX" },
  { LIBSPECTRUM_ID_SNAPSHOT_Z80, "Z80" },
  { 0, NULL },	/* End marker */
};

typedef struct libspectrum_rzx_frame_t {

  size_t instructions;

  size_t count;
  libspectrum_byte* in_bytes;

  int repeat_last;			/* Set if we should use the last
					   frame's IN bytes */

} libspectrum_rzx_frame_t;

typedef struct input_block_t {

  libspectrum_rzx_frame_t *frames;
  size_t count;
  size_t allocated;

  size_t tstates;

  /* Used for recording to note the last non-repeated frame. We can't
     really use a direct pointer to the frame here as that will move
     around when we do a renew on the array, so just dereference it
     every time */
  size_t non_repeat;

} input_block_t;

typedef struct snapshot_block_t {

  libspectrum_snap *snap;
  int automatic;

} snapshot_block_t;

typedef struct signature_block_t {

  size_t length;	/* Length of the signed data from rzx->signed_start */

#ifdef HAVE_GCRYPT_H
  gcry_mpi_t r, s;
#endif			/* #ifdef HAVE_GCRYPT_H */

} signature_block_t;

typedef struct rzx_block_t {

  libspectrum_rzx_block_id type;

  union {

    input_block_t input;
    snapshot_block_t snap;
    libspectrum_dword keyid;
    signature_block_t signature;

  } types;

} rzx_block_t;

struct libspectrum_rzx {

  GSList *blocks;

  /* Playback variables */
  GSList *current_block;
  input_block_t *current_input;
  size_t current_frame;

  libspectrum_rzx_frame_t *data_frame;
  size_t in_count;

  /* Signature parameters */
  const libspectrum_byte *signed_start;
  size_t signed_length;

};

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_creator( const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_snapshot( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		   const libspectrum_byte *end );
static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_frames( input_block_t *block, const libspectrum_byte **ptr,
		 const libspectrum_byte *end );
static libspectrum_error
rzx_read_sign_start( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		     const libspectrum_byte *end );
static libspectrum_error
rzx_read_sign_end( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		   const libspectrum_byte *end );

static void
rzx_write_header( libspectrum_buffer *buffer, int sign );
static void
rzx_write_creator( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                   libspectrum_creator *creator );
static libspectrum_error
rzx_write_snapshot( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                    libspectrum_snap *snap, libspectrum_id_t snap_format,
                    libspectrum_creator *creator, int compress );
static void
rzx_write_input( input_block_t *block, libspectrum_buffer *buffer,
                 libspectrum_buffer *block_data, int compress );
static libspectrum_error
rzx_write_signed_start( libspectrum_buffer *buffer,
                        libspectrum_buffer *block_data,
                        libspectrum_rzx_dsa_key *key,
			libspectrum_creator *creator );
static libspectrum_error
rzx_write_signed_end( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                      libspectrum_rzx_dsa_key *key );

/* The signature used to identify .rzx files */
static const char * const rzx_signature = "RZX!";

/* The IN count used to signify 'repeat last frame' */
const libspectrum_word libspectrum_rzx_repeat_frame = 0xffff;

/*
 * Generic block handling routines
 */

static void
block_alloc( rzx_block_t **block, libspectrum_rzx_block_id type )
{
  *block = libspectrum_new( rzx_block_t, 1 );
  (*block)->type = type;
}

static libspectrum_error
block_free( rzx_block_t *block )
{
  size_t i;
  input_block_t *input;
#ifdef HAVE_GCRYPT_H
  signature_block_t *signature;
#endif				/* #ifdef HAVE_GCRYPT_H */

  switch( block->type ) {

  case LIBSPECTRUM_RZX_INPUT_BLOCK:
    input = &( block->types.input );
    for( i = 0; i < input->count; i++ )
      if( !input->frames[i].repeat_last ) libspectrum_free( input->frames[i].in_bytes );
    libspectrum_free( input->frames );
    libspectrum_free( block );
    return LIBSPECTRUM_ERROR_NONE;

  case LIBSPECTRUM_RZX_SNAPSHOT_BLOCK:
    libspectrum_snap_free( block->types.snap.snap );
    libspectrum_free( block );
    return LIBSPECTRUM_ERROR_NONE;

  case LIBSPECTRUM_RZX_SIGN_START_BLOCK:
    libspectrum_free( block );
    return LIBSPECTRUM_ERROR_NONE;

  case LIBSPECTRUM_RZX_SIGN_END_BLOCK:
#ifdef HAVE_GCRYPT_H
    signature = &( block->types.signature );
    gcry_mpi_release( signature->r );
    gcry_mpi_release( signature->s );
#endif				/* #ifdef HAVE_GCRYPT_H */

    libspectrum_free( block );
    return LIBSPECTRUM_ERROR_NONE;

  case LIBSPECTRUM_RZX_CREATOR_BLOCK:
    break;

  }

  libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			   "unknown RZX block type %d at %s:%d", block->type,
			   __FILE__, __LINE__ );
  return LIBSPECTRUM_ERROR_LOGIC;
}

static void
block_free_wrapper( gpointer data, gpointer user_data )
{
  block_free( data );
}

static gint
find_block( gconstpointer a, gconstpointer b )
{
  const rzx_block_t *block = a;
  libspectrum_byte id = GPOINTER_TO_INT( b );

  return block->type - id;
}

/*
 * Main routines
 */

libspectrum_rzx*
libspectrum_rzx_alloc( void )
{
  libspectrum_rzx *rzx = libspectrum_new( libspectrum_rzx, 1 );
  rzx->blocks = NULL;
  rzx->current_block = NULL;
  rzx->current_input = NULL;
  rzx->signed_start = NULL;
  return rzx;
}

void
libspectrum_rzx_start_input( libspectrum_rzx *rzx, libspectrum_dword tstates )
{
  rzx_block_t *block;

  block_alloc( &block, LIBSPECTRUM_RZX_INPUT_BLOCK );

  rzx->current_input = &( block->types.input );

  rzx->current_input->tstates = tstates;
  rzx->current_input->frames = NULL;
  rzx->current_input->allocated = 0;
  rzx->current_input->count = 0;
  rzx->current_input->non_repeat = 0;

  rzx->blocks = g_slist_append( rzx->blocks, block );
}

libspectrum_error
libspectrum_rzx_stop_input( libspectrum_rzx *rzx )
{
  rzx->current_input = NULL;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_add_snap( libspectrum_rzx *rzx, libspectrum_snap *snap, int automatic )
{
  rzx_block_t *block;

  libspectrum_rzx_stop_input( rzx );

  block_alloc( &block, LIBSPECTRUM_RZX_SNAPSHOT_BLOCK );

  block->types.snap.snap = snap;
  block->types.snap.automatic = automatic;

  rzx->blocks = g_slist_append( rzx->blocks, block );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_rollback( libspectrum_rzx *rzx, libspectrum_snap **snap )
{
  GSList *previous, *list;
  rzx_block_t *block;

  /* Find the last snapshot block in the file */
  previous = NULL; list = rzx->blocks;

  while( 1 ) {
    list =
      g_slist_find_custom( list,
			   GINT_TO_POINTER( LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ),
			   find_block );
    if( !list ) break;

    previous = list;
    list = list->next;
  }
    

  if( !previous ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "no snapshot block found in recording" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( rzx->current_input ) {
    libspectrum_rzx_stop_input( rzx );
  }

  /* Delete all blocks after the snapshot */
  g_slist_foreach( previous->next, block_free_wrapper, NULL );
  previous->next = NULL;

  block = previous->data;
  *snap = block->types.snap.snap;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_rollback_to( libspectrum_rzx *rzx, libspectrum_snap **snap,
			     size_t which )
{
  GSList *previous = NULL, *list;
  rzx_block_t *block;
  size_t i;

  /* Find the nth snapshot block in the file */
  for( i = 0, list = rzx->blocks; i <= which; i++, list = list->next ) {
    list =
      g_slist_find_custom( list,
			   GINT_TO_POINTER( LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ),
			   find_block );
    if( !list ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "snapshot block %lu not found in recording",
			       (unsigned long)which );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    previous = list;
  }

  if( rzx->current_input ) {
    libspectrum_rzx_stop_input( rzx );
  }

  /* Delete all blocks after the snapshot */
  g_slist_foreach( previous->next, block_free_wrapper, NULL );
  previous->next = NULL;

  block = previous->data;
  *snap = block->types.snap.snap;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
input_block_resize( input_block_t *input, size_t new_count )
{
  libspectrum_rzx_frame_t *ptr;
  size_t new_allocated;

  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( new_count > input->allocated ) {

    new_allocated = input->allocated >= 25 ? 2 * input->allocated : 50;
    if( new_allocated < new_count ) new_allocated = new_count;

    ptr = libspectrum_renew( libspectrum_rzx_frame_t, input->frames,
                             new_allocated );
    if( !ptr ) return LIBSPECTRUM_ERROR_MEMORY;

    input->frames = ptr;
    input->allocated = new_allocated;    
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_store_frame( libspectrum_rzx *rzx, size_t instructions,
			     size_t count, libspectrum_byte *in_bytes )
{
  input_block_t *input;
  libspectrum_rzx_frame_t *frame;
  libspectrum_error error;

  input = rzx->current_input;

  /* Check we've got an IRB to record to */
  if( !input ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_INVALID,
      "libspectrum_rzx_store_frame called with no active input block"
    );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  /* Get more space if we need it */
  if( input->allocated == input->count ) {
    error = input_block_resize( input, input->count + 1 );
    if( error ) return error;
  }

  frame = &input->frames[ input->count ];

  frame->instructions = instructions;

  /* Check for repeated frames */
  if( input->count != 0 && count != 0 &&
      count == input->frames[ input->non_repeat ].count &&
      !memcmp( in_bytes, input->frames[ input->non_repeat ].in_bytes,
	       count )
    ) {
	
    frame->repeat_last = 1;
    frame->count = 0;
    frame->in_bytes = NULL;

  } else {

    frame->repeat_last = 0;
    frame->count = count;

    /* Note this as the last non-repeated frame */
    input->non_repeat = input->count;

    if( count ) {

      frame->in_bytes = libspectrum_new( libspectrum_byte, count );

      memcpy( frame->in_bytes, in_bytes,
	      count * sizeof( *( frame->in_bytes ) ) );

    } else {

      frame->in_bytes = NULL;

    }
  }

  /* Move along to the next frame */
  input->count++;

  return 0;
}

libspectrum_error
libspectrum_rzx_start_playback( libspectrum_rzx *rzx, int which,
				libspectrum_snap **snap )
{
  GSList *list, *previous;
  rzx_block_t *block;
  int i;

  *snap = NULL;

  for( i = which, previous = NULL, list = rzx->blocks;
       list;
       previous = list, list = list->next ) {
    
    block = list->data;

    /* Skip any blocks which aren't input recording blocks */
    if( block->type != LIBSPECTRUM_RZX_INPUT_BLOCK ) continue;

    /* Skip input recording blocks until we find the one we want */
    if( i-- ) continue;

    rzx->current_block = list;
    rzx->current_input = &( block->types.input );

    rzx->current_frame = 0; rzx->in_count = 0;
    rzx->data_frame = rzx->current_input->frames;

    /* If the previous frame was a snap, return that as well */
    if( previous ) {

      block = previous->data;

      if( block->type == LIBSPECTRUM_RZX_SNAPSHOT_BLOCK )
	*snap = block->types.snap.snap;
    }

    return LIBSPECTRUM_ERROR_NONE;

  }

  libspectrum_print_error(
    LIBSPECTRUM_ERROR_INVALID,
    "libspectrum_rzx_start_playback: input recording block %d does not exist",
    which
  );
  return LIBSPECTRUM_ERROR_INVALID;
}

libspectrum_error
libspectrum_rzx_playback_frame( libspectrum_rzx *rzx, int *finished,
				libspectrum_snap **snap )
{
  *snap = NULL;
  *finished = 0;

  /* Check we read the correct number of INs during this frame */
  if( rzx->in_count != rzx->data_frame->count ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_rzx_playback_frame: wrong number of INs in frame %lu: expected %lu, got %lu",
      (unsigned long)rzx->current_frame,
      (unsigned long)rzx->data_frame->count, (unsigned long)rzx->in_count
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Increment the frame count and see if we've finished with this file */
  if( ++rzx->current_frame >= rzx->current_input->count ) {

    GSList *it = rzx->current_block->next;
    rzx->current_block = NULL;

    for( ; it; it = it->next ) {

      rzx_block_t *block = it->data;

      if( block->type == LIBSPECTRUM_RZX_INPUT_BLOCK ) {
	rzx->current_block = it;
	break;
      } else if( block->type == LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ) {
	*snap = block->types.snap.snap;
      }

    }

    if( rzx->current_block ) {
    
      rzx_block_t *block = rzx->current_block->data;
      rzx->current_input = &( block->types.input );

      rzx->current_frame = 0; rzx->in_count = 0;
      rzx->data_frame = rzx->current_input->frames;

    } else {
      *finished = 1;
    }

    return LIBSPECTRUM_ERROR_NONE;
  }

  /* Move the data frame pointer along, unless we're supposed to be
     repeating the last frame */
  if( !rzx->current_input->frames[ rzx->current_frame ].repeat_last )
    rzx->data_frame = &rzx->current_input->frames[ rzx->current_frame ];

  /* And start with the first byte of the new frame */
  rzx->in_count = 0;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_playback( libspectrum_rzx *rzx, libspectrum_byte *byte )
{
  /* Check we're not trying to read off the end of the array */
  if( rzx->in_count >= rzx->data_frame->count ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_rzx_playback: more INs during frame %lu than stored in RZX file (%lu)",
      (unsigned long)rzx->current_frame, (unsigned long)rzx->data_frame->count
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  *byte = rzx->data_frame->in_bytes[ rzx->in_count++ ];
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_free( libspectrum_rzx *rzx )
{
  g_slist_foreach( rzx->blocks, block_free_wrapper, NULL );
  g_slist_free( rzx->blocks );
  libspectrum_free( rzx );
  return LIBSPECTRUM_ERROR_NONE;
}

size_t
libspectrum_rzx_tstates( libspectrum_rzx *rzx )
{
  return rzx->current_input->tstates;
}

size_t
libspectrum_rzx_instructions( libspectrum_rzx *rzx )
{
  return rzx->current_input->frames[ rzx->current_frame ].instructions;
}

libspectrum_dword
libspectrum_rzx_get_keyid( libspectrum_rzx *rzx )
{
  GSList *list;
  rzx_block_t *block;

  list =
    g_slist_find_custom( rzx->blocks,
			 GINT_TO_POINTER( LIBSPECTRUM_RZX_SIGN_START_BLOCK ),
			 find_block );
  if( !list ) return 0;

  block = list->data;
  return block->types.keyid;
}

libspectrum_error
libspectrum_rzx_get_signature( libspectrum_rzx *rzx,
			       libspectrum_signature *signature )
{
  GSList *list;
  rzx_block_t *block;
  signature_block_t *sigblock;

  list =
    g_slist_find_custom( rzx->blocks,
			 GINT_TO_POINTER( LIBSPECTRUM_RZX_SIGN_END_BLOCK ),
			 find_block );
  if( !list ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "no end of signed data block found" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block = list->data;
  sigblock = &( block->types.signature );

  signature->start = rzx->signed_start;
  signature->length = sigblock->length;

#ifdef HAVE_GCRYPT_H
  signature->r = gcry_mpi_copy( sigblock->r );
  signature->s = gcry_mpi_copy( sigblock->s );
#endif				/* #ifdef HAVE_GCRYPT_H */

  return LIBSPECTRUM_ERROR_NONE;
}
  

libspectrum_error
libspectrum_rzx_read( libspectrum_rzx *rzx, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *ptr, *end;
  libspectrum_byte *new_buffer;
  libspectrum_id_t raw_type;
  libspectrum_class_t class;

  /* Find out if this file needs decompression */
  new_buffer = NULL;

  error = libspectrum_identify_file_raw( &raw_type, NULL, buffer, length );
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

  ptr = buffer; end = buffer + length;

  error = rzx_read_header( &ptr, end );
  if( error != LIBSPECTRUM_ERROR_NONE ) { libspectrum_free( new_buffer ); return error; }

  rzx->signed_start = ptr;

  while( ptr < end ) {

    libspectrum_byte id;

    id = *ptr++;

    switch( id ) {

    case LIBSPECTRUM_RZX_CREATOR_BLOCK:
      error = rzx_read_creator( &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_free( new_buffer );
	return error;
      }
      break;
      
    case LIBSPECTRUM_RZX_SNAPSHOT_BLOCK:
      error = rzx_read_snapshot( rzx, &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_free( new_buffer );
	return error;
      }
      break;

    case LIBSPECTRUM_RZX_INPUT_BLOCK:
      error = rzx_read_input( rzx, &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_free( new_buffer );
	return error;
      }
      break;

    case LIBSPECTRUM_RZX_SIGN_START_BLOCK:
      error = rzx_read_sign_start( rzx, &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_free( new_buffer );
	return error;
      }
      break;

    case LIBSPECTRUM_RZX_SIGN_END_BLOCK:
      error = rzx_read_sign_end( rzx, &ptr, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	libspectrum_free( new_buffer );
	return error;
      }
      break;

    default:
      libspectrum_print_error(
	LIBSPECTRUM_ERROR_UNKNOWN,
        "libspectrum_rzx_read: unknown RZX block ID 0x%02x", id
      );
      libspectrum_free( new_buffer );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
  }

  libspectrum_free( new_buffer );
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  /* Check the header exists */
  if( end - (*ptr) < (ptrdiff_t)strlen( rzx_signature ) + 6 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_header: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check the RZX signature exists */
  if( memcmp( *ptr, rzx_signature, strlen( rzx_signature ) ) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_SIGNATURE,
			     "rzx_read_header: RZX signature not found" );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  /* Skip over the signature and the version numbers */
  (*ptr) += strlen( rzx_signature ) + 2;

  /* And skip the flags as well - separate call as we'll need the return
     value if we ever fix the stuff below */
  libspectrum_read_dword( ptr );

  /* FIXME: how to handle signatures */

  /* This is where the signed data starts (if it's signed at all) */
/*   if( signature && ( flags & 0x01 ) ) signature->start = *ptr; */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_creator( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t length;

  /* Check we've got enough data for the block */
  if( end - (*ptr) < 28 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_creator: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length */
  length = libspectrum_read_dword( ptr );

  /* Check there's still enough data (the -5 is because we've already read
     the block ID and the length) */
  if( end - (*ptr) < (ptrdiff_t)length - 5 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_creator: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  (*ptr) += length - 5;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_snapshot( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		   const libspectrum_byte *end )
{
  rzx_block_t *block;
  libspectrum_snap *snap;
  size_t blocklength, snaplength;
  libspectrum_error error = LIBSPECTRUM_ERROR_NONE;
  libspectrum_dword flags;
  const libspectrum_byte *snap_ptr;
  int done;
  snapshot_string_t *type;

  /* For deflated snapshot data: */
  int compressed;
  libspectrum_byte *gzsnap = NULL; size_t uncompressed_length = 0;

  if( end - (*ptr) < 16 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_snapshot: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  blocklength = libspectrum_read_dword( ptr );

  if( end - (*ptr) < (ptrdiff_t)blocklength - 5 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_snapshot: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* See if we want a compressed snap */
  flags = libspectrum_read_dword( ptr );

  /* We don't handle 'links' to external snapshots. I really think these
     are just more trouble than they're worth */
  if( flags & 0x01 ) {
    /* TODO log error if this isn't the first block as that can't be easily
       covered by prompting the user for a snapshot file (but should be
       unlikely to encounter in reality?) */
    (*ptr) += blocklength - 9;
    return LIBSPECTRUM_ERROR_NONE;
  }

  /* Do we have a compressed snapshot? */
  compressed = flags & 0x02;

  /* How long is the (uncompressed) snap? */
  (*ptr) += 4;
  snaplength = libspectrum_read_dword( ptr );
  (*ptr) -= 8;

  /* If compressed, uncompress the data */
  if( compressed ) {

#ifdef HAVE_ZLIB_H

    error = libspectrum_zlib_inflate( (*ptr) + 8, blocklength - 17,
				      &gzsnap, &uncompressed_length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    if( uncompressed_length != snaplength ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "rzx_read_snapshot: compressed snapshot has wrong length"
      );
      libspectrum_free( gzsnap );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }   
    snap_ptr = gzsnap;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "rzx_read_snapshot: zlib needed for decompression\n"
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    /* If not compressed, check things are consistent */
    if( blocklength != snaplength + 17 ) {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_CORRUPT,
        "rzx_read_snapshot: inconsistent snapshot lengths"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    snap_ptr = (*ptr) + 8;
    uncompressed_length = snaplength;

  }

  block_alloc( &block, LIBSPECTRUM_RZX_SNAPSHOT_BLOCK );
  block->types.snap.snap = libspectrum_snap_alloc();
  block->types.snap.automatic = 0;

  snap = block->types.snap.snap;

  for( done = 0, type = snapshot_strings; type->format; type++ ) {
    if( !strncasecmp( (char*)*ptr, type->string, 4 ) ) {
      error = libspectrum_snap_read( snap, snap_ptr, uncompressed_length,
				     type->format, NULL );
      done = 1;
    }
  }

  if( !done ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:rzx_read_snapshot: unrecognised snapshot format", __FILE__
    );
    if( compressed ) libspectrum_free( gzsnap );
    block_free( block );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    if( compressed ) libspectrum_free( gzsnap );
    block_free( block );
    return error;
  }

  /* Free the decompressed data (if we created it) */
  if( compressed ) libspectrum_free( gzsnap );

  /* Skip over the data */
  (*ptr) += blocklength - 9;

  rzx->blocks = g_slist_append( rzx->blocks, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t blocklength;
  libspectrum_dword flags; int compressed;
  libspectrum_error error;
  rzx_block_t *rzx_block;
  input_block_t *block;

  /* Check we've got enough data for the block */
  if( end - (*ptr) < 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_input: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block_alloc( &rzx_block, LIBSPECTRUM_RZX_INPUT_BLOCK );

  block = &( rzx_block->types.input );

  /* Get the length and number of frames */
  blocklength = libspectrum_read_dword( ptr );
  block->count = libspectrum_read_dword( ptr );

  /* Frame size is undefined, so just skip it */
  (*ptr)++;

  /* Allocate memory for the frames */
  block->frames = libspectrum_new( libspectrum_rzx_frame_t, block->count );
  block->allocated = block->count;

  /* Fetch the T-state counter and the flags */
  block->tstates = libspectrum_read_dword( ptr );

  flags = libspectrum_read_dword( ptr );
  compressed = flags & 0x02;

  if( compressed ) {

#ifdef HAVE_ZLIB_H

    libspectrum_byte *data; const libspectrum_byte *data_ptr;
    size_t data_length = 0;

    /* Discount the block intro */
    blocklength -= 18;

    /* Check that we've got enough compressed data */
    if( end - (*ptr) < (ptrdiff_t)blocklength ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_input: not enough data in buffer" );
      libspectrum_free( rzx_block );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    error = libspectrum_zlib_inflate( *ptr, blocklength, &data, &data_length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      block_free( rzx_block );
      return error;
    }

    *ptr += blocklength;

    data_ptr = data;

    error = rzx_read_frames( block, &data_ptr, data + data_length );
    if( error ) { libspectrum_free( rzx_block ); libspectrum_free( data ); return error; }

    libspectrum_free( data );

#else				/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "rzx_read_input: zlib needed for decompression" );
    libspectrum_free( rzx_block );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

  } else {			/* Data not compressed */

    error = rzx_read_frames( block, ptr, end );
    if( error ) { libspectrum_free( rzx_block ); return error; }
  }

  rzx->blocks = g_slist_append( rzx->blocks, rzx_block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_frames( input_block_t *block, const libspectrum_byte **ptr,
		 const libspectrum_byte *end )
{
  size_t i, j;

  /* And read in the frames */
  for( i=0; i < block->count; i++ ) {

    /* Check the two length bytes exist */
    if( end - (*ptr) < 4 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_frames: not enough data in buffer" );
      for( j=0; j<i; j++ ) {
	if( !block->frames[i].repeat_last ) libspectrum_free( block->frames[j].in_bytes );
      }
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    block->frames[i].instructions = libspectrum_read_word( ptr );
    block->frames[i].count        = libspectrum_read_word( ptr );

    if( block->frames[i].count == libspectrum_rzx_repeat_frame ) {
      block->frames[i].repeat_last = 1;
      continue;
    }

    block->frames[i].repeat_last = 0;

    if( end - (*ptr) < (ptrdiff_t)block->frames[i].count ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "rzx_read_frames: not enough data in buffer" );
      for( j=0; j<i; j++ ) {
	if( !block->frames[i].repeat_last ) libspectrum_free( block->frames[j].in_bytes );
      }
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    if( block->frames[i].count ) {

      block->frames[i].in_bytes =
	libspectrum_new( libspectrum_byte, block->frames[i].count );
      memcpy( block->frames[i].in_bytes, *ptr, block->frames[i].count );

    } else {
      block->frames[i].in_bytes = NULL;
    }

    (*ptr) += block->frames[i].count;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_sign_start( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		     const libspectrum_byte *end )
{
  rzx_block_t *block;
  libspectrum_dword length;

  /* Check we've got enough data for the length */

  if( end - (*ptr) < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "rzx_read_sign_start: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  length = libspectrum_read_dword( ptr );

  /* Check the length is at least the expected 13 bytes */
  if( length < 13 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "rzx_read_sign_start: block length %lu less than the minimum 13 bytes",
      (unsigned long)length
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check there's still enough data (the -5 is because we've already read
     the length and the block ID) */
  if( end - (*ptr) < (ptrdiff_t)length - 5 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "rzx_read_sign_start: not enough data in buffer"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block_alloc( &block, LIBSPECTRUM_RZX_SIGN_START_BLOCK );

  block->types.keyid = libspectrum_read_dword( ptr );

  /* Skip the week code */
  *ptr += 4;

  /* Skip anything we don't know about */
  *ptr += length - 13;

  rzx->blocks = g_slist_append( rzx->blocks, block );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_sign_end( libspectrum_rzx *rzx, const libspectrum_byte **ptr,
		   const libspectrum_byte *end )
{
  rzx_block_t *block;
  signature_block_t *signature;
  size_t length;

  /* Check we've got enough data for the length */
  if( end - (*ptr) < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_sign_end: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Get the length; the -5 is because we've read the block ID and the length
     bytes */
  length = libspectrum_read_dword( ptr ) - 5;

  /* Check there's still enough data */
  if( end - (*ptr) < (ptrdiff_t)length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "rzx_read_sign_end: not enough data in buffer" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  block_alloc( &block, LIBSPECTRUM_RZX_SIGN_END_BLOCK );

  signature = &( block->types.signature );

  /* - 5 as we don't sign the block ID and length of this block */
  signature->length = ( *ptr - rzx->signed_start ) - 5;

#ifdef HAVE_GCRYPT_H
  { 
    gcry_error_t error; size_t mpi_length;

    error = gcry_mpi_scan( &signature->r, GCRYMPI_FMT_PGP, *ptr, length,
			   &mpi_length );
    if( error ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "error reading 'r': %s",
			       gcry_strerror( error ) );
      libspectrum_free( block );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    (*ptr) += mpi_length; length -= mpi_length;

    error = gcry_mpi_scan( &signature->s, GCRYMPI_FMT_PGP, *ptr, length,
			   &mpi_length );
    if( error ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			       "error reading 's': %s",
			       gcry_strerror( error ) );
      gcry_mpi_release( signature->r );
      libspectrum_free( block );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    (*ptr) += mpi_length; length -= mpi_length;

  }
#endif				/* #ifdef HAVE_GCRYPT_H */

  (*ptr) += length;

  rzx->blocks = g_slist_append( rzx->blocks, block );

  return LIBSPECTRUM_ERROR_NONE;
}
  

libspectrum_error
libspectrum_rzx_write( libspectrum_byte **buffer, size_t *length,
		       libspectrum_rzx *rzx, libspectrum_id_t snap_format,
		       libspectrum_creator *creator, int compress,
		       libspectrum_rzx_dsa_key *key )
{
  libspectrum_error error;
  GSList *list;
  libspectrum_byte *ptr = *buffer;
  libspectrum_buffer *new_buffer = libspectrum_buffer_alloc();
  libspectrum_buffer *block_data = libspectrum_buffer_alloc();

  if( creator ) rzx_write_creator( new_buffer, block_data, creator );

  if( key ) {
    error = rzx_write_signed_start( new_buffer, block_data, key, creator );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_buffer_free( new_buffer );
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  for( list = rzx->blocks; list; list = list->next ) {

    rzx_block_t *block = list->data;

    switch( block->type ) {

    case LIBSPECTRUM_RZX_SNAPSHOT_BLOCK:
      error = rzx_write_snapshot( new_buffer, block_data,
                                  block->types.snap.snap, snap_format, creator,
                                  compress );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
        libspectrum_buffer_free( new_buffer );
        libspectrum_buffer_free( block_data );
        return error;
      }
      break;

    case LIBSPECTRUM_RZX_INPUT_BLOCK:
      /* z80 snapshots can't safely store an intermediate state */
      snap_format = LIBSPECTRUM_ID_SNAPSHOT_SZX;

      rzx_write_input( &( block->types.input ), new_buffer, block_data,
                       compress );
      break;

    case LIBSPECTRUM_RZX_CREATOR_BLOCK:
    case LIBSPECTRUM_RZX_SIGN_START_BLOCK:
    case LIBSPECTRUM_RZX_SIGN_END_BLOCK:
      break;

    }
  }

  if( key ) {
    error = rzx_write_signed_end( new_buffer, block_data, key );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_buffer_free( new_buffer );
      libspectrum_buffer_free( block_data );
      return error;
    }
  }
  
  rzx_write_header( block_data, key ? 1 : 0 );
  libspectrum_buffer_append( buffer, length, &ptr, block_data );
  libspectrum_buffer_free( block_data );

  libspectrum_buffer_append( buffer, length, &ptr, new_buffer );
  libspectrum_buffer_free( new_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
rzx_write_header( libspectrum_buffer *buffer, int sign )
{
  libspectrum_buffer_write( buffer, rzx_signature, strlen( rzx_signature ) );

  libspectrum_buffer_write_byte( buffer, 0 );	/* Major version number */

  /* Flags */
#ifdef HAVE_GCRYPT_H
  /* Minor version number: 12 if we're not signing, 13 if we are */
  libspectrum_buffer_write_byte( buffer, sign ? 13 : 12 );	/* Minor version number */
  libspectrum_buffer_write_dword( buffer, sign ? 0x01 : 0x00 );
#else				/* #ifdef HAVE_GCRYPT_H */
  libspectrum_buffer_write_byte( buffer, 12 );	/* Minor version number */
  libspectrum_buffer_write_dword( buffer, 0 );
#endif				/* #ifdef HAVE_GCRYPT_H */
}

static void
rzx_write_block_header( libspectrum_buffer *dest, libspectrum_buffer *src,
                        libspectrum_byte type )
{
  size_t block_length = libspectrum_buffer_get_data_size( src ) +
                      sizeof( libspectrum_dword ) + sizeof( libspectrum_byte );

  libspectrum_buffer_write_byte( dest, type );
  libspectrum_buffer_write_dword( dest, block_length );

  libspectrum_buffer_write_buffer( dest, src );

  libspectrum_buffer_clear( src );
}

static void
rzx_write_creator( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                   libspectrum_creator *creator )
{
  size_t custom_length = libspectrum_creator_custom_length( creator );

  libspectrum_buffer_write( block_data, libspectrum_creator_program( creator ),
                            20 );

  libspectrum_buffer_write_word( block_data, libspectrum_creator_major( creator ) );
  libspectrum_buffer_write_word( block_data, libspectrum_creator_minor( creator ) );

  if( custom_length ) {
    libspectrum_buffer_write( block_data,
                              libspectrum_creator_custom( creator ),
                              custom_length );
  }

  rzx_write_block_header( buffer, block_data, LIBSPECTRUM_RZX_CREATOR_BLOCK );
}

static libspectrum_error
rzx_compress( libspectrum_buffer *dest, libspectrum_buffer *src, int *compress )
{
#ifdef HAVE_ZLIB_H
  libspectrum_byte *out_data = libspectrum_buffer_get_data( src );
  size_t out_length = libspectrum_buffer_get_data_size( src );
  libspectrum_byte *compressed_data = NULL;
  libspectrum_error error;
#endif

  if( !(*compress) ) {
    libspectrum_buffer_write_buffer( dest, src );
    return LIBSPECTRUM_ERROR_NONE;
  }

#ifdef HAVE_ZLIB_H
  error = libspectrum_zlib_compress( out_data, out_length, &compressed_data,
                                     &out_length );
  if( error != LIBSPECTRUM_ERROR_NONE || 
      out_length >= libspectrum_buffer_get_data_size( src ) ) {
    *compress = 0;

    /* We couldn't compress the data, so just copy the uncompressed data
       across instead */
    libspectrum_buffer_write_buffer( dest, src );
    libspectrum_free( compressed_data );

    return LIBSPECTRUM_ERROR_NONE;
  }

  libspectrum_buffer_write( dest, compressed_data, out_length );
  libspectrum_free( compressed_data );

  return LIBSPECTRUM_ERROR_NONE;

#else				/* #ifdef HAVE_ZLIB_H */

  libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                           "rzx_compress: compression needs zlib" );
  return LIBSPECTRUM_ERROR_UNKNOWN;

#endif				/* #ifdef HAVE_ZLIB_H */

}

static libspectrum_error
rzx_write_snapshot( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                    libspectrum_snap *snap, libspectrum_id_t snap_format,
                    libspectrum_creator *creator, int compress )
{
  libspectrum_error error = LIBSPECTRUM_ERROR_NONE;
  int flags, done;
  snapshot_string_t *type;
  libspectrum_buffer *snap_data = libspectrum_buffer_alloc();
  size_t uncompressed_data_size;

  if( snap_format == LIBSPECTRUM_ID_UNKNOWN ) {
    /* If not given a snap format, try using .z80. If that would result
       in major information loss, use .szx instead */
    snap_format = LIBSPECTRUM_ID_SNAPSHOT_Z80;
    error = libspectrum_snap_write_buffer( block_data, &flags, snap,
                                           snap_format, creator, 0 );
    if( error ) { goto cleanup; }

    if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
      libspectrum_buffer_clear( block_data );
      snap_format = LIBSPECTRUM_ID_SNAPSHOT_SZX;
      error = libspectrum_snap_write_buffer( block_data, &flags, snap,
                                             snap_format, creator, 0 );
      if( error ) { goto cleanup; }
    }
  } else {
    error = libspectrum_snap_write_buffer( block_data, &flags, snap,
                                           snap_format, creator, 0 );
    if( error ) { goto cleanup; }
  }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_WARNING,
			     "%s:rzx_write_snapshot: embedded snapshot has lost a significant amount of information",
			     __FILE__ );
  }

  uncompressed_data_size = libspectrum_buffer_get_data_size( block_data );
  rzx_compress( snap_data, block_data, &compress );
  libspectrum_buffer_clear( block_data );

  libspectrum_buffer_write_dword( block_data, compress ? 2 : 0 );

  for( type = snapshot_strings, done = 0; type->format; type++ ) {
    if( type->format == snap_format ) {
      libspectrum_buffer_write( block_data, type->string, 4 );
      done = 1;
      break;
    }
  }

  if( !done ) {
    error =  LIBSPECTRUM_ERROR_UNKNOWN;
    libspectrum_print_error(
      error,
      "%s:rzx_write_snapshot: unexpected snap type %d", __FILE__, snap_format
    );
    goto cleanup;
  }

  /* Uncompressed snapshot length */
  libspectrum_buffer_write_dword( block_data, uncompressed_data_size );

  libspectrum_buffer_write_buffer( block_data, snap_data );

  rzx_write_block_header( buffer, block_data, LIBSPECTRUM_RZX_SNAPSHOT_BLOCK );

cleanup:
  libspectrum_buffer_free( snap_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
rzx_write_input( input_block_t *block, libspectrum_buffer *buffer,
                 libspectrum_buffer *block_data, int compress )
{
  size_t i;
  libspectrum_buffer *frame_data = libspectrum_buffer_alloc();

  /* Write the frames */
  for( i = 0; i < block->count; i++ ) {

    libspectrum_rzx_frame_t *frame = &block->frames[i];

    libspectrum_buffer_write_word( block_data, frame->instructions );

    if( frame->repeat_last ) {
      libspectrum_buffer_write_word( block_data, libspectrum_rzx_repeat_frame );
    } else {
      libspectrum_buffer_write_word( block_data, frame->count );
      libspectrum_buffer_write( block_data, frame->in_bytes, frame->count );
    }

  }

  rzx_compress( frame_data, block_data, &compress );
  libspectrum_buffer_clear( block_data );

  /* How many frames? */
  libspectrum_buffer_write_dword( block_data, block->count );

  /* Each frame has an undefined length, so write a zero */
  libspectrum_buffer_write_byte( block_data, 0 );

  /* T-state counter */
  libspectrum_buffer_write_dword( block_data, block->tstates );

  /* Flags */
  libspectrum_buffer_write_dword( block_data, compress ? 0x02 : 0 );

  libspectrum_buffer_write_buffer( block_data, frame_data );

  rzx_write_block_header( buffer, block_data, LIBSPECTRUM_RZX_INPUT_BLOCK );

  libspectrum_buffer_clear( block_data );
  libspectrum_buffer_free( frame_data );
}

static libspectrum_error
rzx_write_signed_start( libspectrum_buffer *buffer,
                        libspectrum_buffer *block_data,
                        libspectrum_rzx_dsa_key *key,
			libspectrum_creator *creator )
{
#ifdef HAVE_GCRYPT_H
  /* Key ID */
  if( !key || !key->y || strlen( key->y ) < 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_INVALID,
			     "rzx_write_signed_start: invalid key" );
    return LIBSPECTRUM_ERROR_INVALID;
  }

  libspectrum_buffer_write_dword(
    block_data, strtoul( &key->y[ strlen( key->y ) - 8 ], NULL, 16 )
  );

  /* Week code */
  if( creator ) {
    libspectrum_buffer_write_dword( block_data,
			     libspectrum_creator_competition_code( creator ) );
  } else {
    libspectrum_buffer_write_dword( block_data, 0 );
  }

  rzx_write_block_header( buffer, block_data,
                          LIBSPECTRUM_RZX_SIGN_START_BLOCK );

#endif				/* #ifdef HAVE_GCRYPT_H */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_signed_end( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                      libspectrum_rzx_dsa_key *key )
{
#ifdef HAVE_GCRYPT_H
  libspectrum_error error;
  libspectrum_byte *signature; size_t sig_length;

  /* Get the actual signature */
  error = libspectrum_sign_data( &signature, &sig_length,
                                 libspectrum_buffer_get_data( buffer ),
                                 libspectrum_buffer_get_data_size( buffer ),
                                 key );
  if( error ) return error;

  /* Write the signature */
  libspectrum_buffer_write( block_data, signature, sig_length );

  libspectrum_free( signature );

  rzx_write_block_header( buffer, block_data,
                          LIBSPECTRUM_RZX_SIGN_END_BLOCK );

#endif				/* #ifdef HAVE_GCRYPT_H */

  return LIBSPECTRUM_ERROR_NONE;
}

void
libspectrum_rzx_insert_snap( libspectrum_rzx *rzx, libspectrum_snap *snap,
			     int where )
{
  rzx_block_t *block;

  block_alloc( &block, LIBSPECTRUM_RZX_SNAPSHOT_BLOCK );
  block->types.snap.snap = snap;
  block->types.snap.automatic = 0;

  rzx->blocks = g_slist_insert( rzx->blocks, block, where );
}

/*
 * Iterator functions
 */

libspectrum_rzx_iterator
libspectrum_rzx_iterator_begin( libspectrum_rzx *rzx )
{
  return rzx->blocks;
}

libspectrum_rzx_iterator
libspectrum_rzx_iterator_next( libspectrum_rzx_iterator it )
{
  return it->next;
}

libspectrum_rzx_iterator
libspectrum_rzx_iterator_last( libspectrum_rzx *rzx )
{
  /* This function iterates over the whole list */
  return g_slist_last( rzx->blocks );
}

libspectrum_rzx_block_id
libspectrum_rzx_iterator_get_type( libspectrum_rzx_iterator it )
{
  rzx_block_t *block = it->data;

  return block->type;
}

size_t
libspectrum_rzx_iterator_get_frames( libspectrum_rzx_iterator it )
{
  rzx_block_t *block = it->data;

  if( block->type != LIBSPECTRUM_RZX_INPUT_BLOCK ) return -1;

  return block->types.input.count;
}

void
libspectrum_rzx_iterator_delete( libspectrum_rzx *rzx,
				 libspectrum_rzx_iterator it )
{
  block_free( it->data );

  rzx->blocks = g_slist_delete_link( rzx->blocks, it );
}

libspectrum_snap*
libspectrum_rzx_iterator_get_snap( libspectrum_rzx_iterator it )
{
  rzx_block_t *block = it->data;

  if( block->type != LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ) return NULL;

  return block->types.snap.snap;
}

int
libspectrum_rzx_iterator_snap_is_automatic( libspectrum_rzx_iterator it )
{
  rzx_block_t *block = it->data;

  if( block->type != LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ) return 0;

  return block->types.snap.automatic;
}

static libspectrum_error
input_block_merge( input_block_t *input, input_block_t *next_input )
{
  libspectrum_error error;

  /* Get more space if we need it */
  if( input->allocated < input->count + next_input->count ) {
    error = input_block_resize( input, input->count + next_input->count );
    if( error ) return error;
  }

  /* Note in_bytes are not duplicated */
  memcpy( &( input->frames[input->count] ), next_input->frames,
          next_input->count * sizeof( libspectrum_rzx_frame_t ) );

  input->non_repeat = input->count + next_input->non_repeat;
  input->count += next_input->count;
  next_input->count = 0; /* don't free reused in_bytes */

  return 0;
}

libspectrum_error
libspectrum_rzx_finalise( libspectrum_rzx *rzx )
{
  GSList *list, *item, *next_item;
  rzx_block_t *block, *next_block;
  libspectrum_error error;
  int first_snap = 1;
  int finalised = 0;

  /* Delete interspersed snapshots */
  list = rzx->blocks;

  while( list ) {
    item = list;
    block = list->data;
    list = list->next;

    if( block->type == LIBSPECTRUM_RZX_SNAPSHOT_BLOCK ) {
      if( first_snap ) {
        first_snap = 0;
      } else {
        block_free( block );
        rzx->blocks = g_slist_delete_link( rzx->blocks, item );
        finalised = 1;
      }
    }
  }

  /* Merge adjacent input blocks */
  list = rzx->blocks;

  while( list ) {
    block = list->data;

    if( block->type == LIBSPECTRUM_RZX_INPUT_BLOCK ) {

      next_item = list->next;
      if( !next_item ) break;

      next_block = next_item->data;

      if( next_block->type == LIBSPECTRUM_RZX_INPUT_BLOCK ) {
        error = input_block_merge( &( block->types.input ),
                                   &( next_block->types.input ) );
        if( error ) return error;

        block_free( next_block );
        rzx->blocks = g_slist_delete_link( rzx->blocks, next_item );
        finalised = 1;
      } else {
        list = list->next;
      }

    } else {
      list = list->next;
    }
  }

  return finalised? LIBSPECTRUM_ERROR_NONE : LIBSPECTRUM_ERROR_INVALID;
}
