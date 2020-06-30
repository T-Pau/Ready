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

#ifndef FUSE_MEMPOOL_H
#define FUSE_MEMPOOL_H

extern const int MEMPOOL_UNTRACKED;

void mempool_register_startup( void );
int mempool_register_pool( void );
void* mempool_malloc( int pool, size_t size );
void* mempool_malloc_n( int pool, size_t nmemb, size_t size );
char* mempool_strdup( int pool, const char *string );
void mempool_free( int pool );

#define mempool_new( pool, type, count ) \
  ( ( type * ) mempool_malloc_n( (pool), (count), sizeof( type ) ) )

/* Unit test helper routines */

int mempool_get_pools( void );
int mempool_get_pool_size( int pool );

#endif				/* #ifndef FUSE_MEMPOOL_H */
