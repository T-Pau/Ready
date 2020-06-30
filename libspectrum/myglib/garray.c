/* garray.c: Minimal replacement for GArray
   Copyright (c) 2008-2016 Philip Kendall

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

#ifndef HAVE_LIB_GLIB		/* Use this iff we're not using the 
				   `proper' glib */
#include <string.h>

#include "internals.h"

GArray*
g_array_new( gboolean zero_terminated, gboolean clear, guint element_size )
{
  GArray *array;

  /* Don't support these options */
  if( zero_terminated || clear ) {
    fprintf( stderr, "%s: zero_terminated and clear options not supported\n",
	     __func__ );
    abort();
  }

  array = libspectrum_malloc( sizeof( *array ) );

  array->element_size = element_size;
  array->data = NULL;
  array->len = array->allocated = 0;

  return array;
}

static void
expand_array( GArray *array, guint len )
{
  size_t new_size = array->len + len;
  gchar *new_data;

  if( new_size < 2 * array->allocated ) new_size = 2 * array->allocated;
  if( new_size < 8 ) new_size = 8;

  new_data = libspectrum_realloc( array->data, new_size * array->element_size );

  array->data = new_data;
  array->allocated = new_size;
}

GArray*
g_array_sized_new( gboolean zero_terminated, gboolean clear,
                   guint element_size, guint reserved_size )
{
  GArray *array = g_array_new( zero_terminated, clear, element_size );

  expand_array( array, reserved_size );

  return array;
}

GArray*
g_array_append_vals( GArray *array, gconstpointer data, guint len )
{
  if( array->len + len > array->allocated )
    expand_array( array, len );

  memcpy( array->data + array->len * array->element_size,
	  data,
	  len * array->element_size );

  array->len += len;

  return array;
}

GArray*
g_array_set_size( GArray *array, guint length )
{
  if( length > array->allocated )
    expand_array( array, length - array->len );

  array->len = length;

  return array;
}

GArray*
g_array_remove_index_fast( GArray *array, guint index )
{
  if( index < array->len - 1 )
    memcpy( array->data + index * array->element_size,
            array->data + (array->len - 1) * array->element_size,
            array->element_size );

  array->len--;

  return array;
}

gchar*
g_array_free( GArray *array, gboolean free_segment )
{
  gchar* segment;

  if( !array ) return NULL;

  if( free_segment ) {
    libspectrum_free( array->data );
    segment = NULL;
  }
  else
    segment = array->data;

  libspectrum_free( array );

  return segment;
}

#endif				/* #ifndef HAVE_LIB_GLIB */
