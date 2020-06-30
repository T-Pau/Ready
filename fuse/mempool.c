/* mempool.c: pooled system memory
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

#include <string.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "mempool.h"

static GArray *memory_pools;

const int MEMPOOL_UNTRACKED = -1;

static int
mempool_init( void *context )
{
  memory_pools = g_array_new( FALSE, FALSE, sizeof( GArray* ) );

  return 0;
}

int
mempool_register_pool( void )
{
  GArray *pool = g_array_new( FALSE, FALSE, sizeof( void* ) );

  g_array_append_val( memory_pools, pool );

  return memory_pools->len - 1;
}

void*
mempool_malloc( int pool, size_t size )
{
  void *ptr;

  if( pool == MEMPOOL_UNTRACKED ) return libspectrum_malloc( size );

  if( pool < 0 || pool >= memory_pools->len ) return NULL;

  ptr = libspectrum_malloc( size );
  if( !ptr ) return NULL;

  g_array_append_val( g_array_index( memory_pools, GArray*, pool ), ptr );

  return ptr;
}

void *
mempool_malloc_n( int pool, size_t nmemb, size_t size )
{
  void *ptr;

  if( pool == MEMPOOL_UNTRACKED ) return libspectrum_malloc_n( nmemb, size );

  if( pool < 0 || pool >= memory_pools->len ) return NULL;

  ptr = libspectrum_malloc_n( nmemb, size );
  if( !ptr ) return NULL;

  g_array_append_val( g_array_index( memory_pools, GArray*, pool ), ptr );

  return ptr;
}

char*
mempool_strdup( int pool, const char *string )
{
  size_t length = strlen( string ) + 1;

  char *ptr = mempool_malloc( pool, length );
  if( !ptr ) return NULL;

  memcpy( ptr, string, length );

  return ptr;
}

void
mempool_free( int pool )
{
  size_t i;

  GArray *p = g_array_index( memory_pools, GArray*, pool );

  for( i = 0; i < p->len; i++ )
    libspectrum_free( g_array_index( p, void*, i ) );

  g_array_set_size( p, 0 );
}

/* Tidy-up function called at end of emulation */
static void
mempool_end( void )
{
  int i;
  GArray *pool;

  if( !memory_pools ) return;

  for( i = 0; i < memory_pools->len; i++ ) {
    pool = g_array_index( memory_pools, GArray *, i );

    g_array_free( pool, TRUE );
  }

  g_array_free( memory_pools, TRUE );
  memory_pools = NULL;
}

void
mempool_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_MEMPOOL, dependencies,
                            ARRAY_SIZE( dependencies ), mempool_init, NULL,
                            mempool_end );
}

/* Unit test helper routines */

int
mempool_get_pools( void )
{
  return memory_pools->len;
}

int
mempool_get_pool_size( int pool )
{
  return g_array_index( memory_pools, GArray*, pool )->len;
}
