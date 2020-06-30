/* bitmap.h: Bitmap routines
   Copyright (c) 2007-2015 Stuart Brady

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

   Philip: philip-fuse@shadowmagic.org.uk

   Stuart: stuart.brady@gmail.com

*/

#ifndef FUSE_BITMAP_H
#define FUSE_BITMAP_H

static inline void
bitmap_set( libspectrum_byte *b, const size_t n )
{
  b[ n / 8 ] |= ( 1 << ( n % 8 ) );
}

static inline void
bitmap_reset( libspectrum_byte *b, const size_t n )
{
  b[ n / 8 ] &= ~( 1 << ( n % 8 ) );
}

static inline int
bitmap_test( const libspectrum_byte *b, const size_t n )
{
  return b[ n / 8 ] & ( 1 << ( n % 8 ) );
}

#endif			/* #ifndef FUSE_BITMAP_H */
