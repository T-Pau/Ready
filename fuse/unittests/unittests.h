/* unittests.h: Unit testing framework for Fuse
   Copyright (c) 2008-2015 Philip Kendall

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

#ifndef FUSE_UNITTESTS_H
#define FUSE_UNITTESTS_H

int unittests_run( void );

int unittests_assert_2k_page( libspectrum_word base, int source, int page );
int unittests_assert_4k_page( libspectrum_word base, int source, int page );
int unittests_assert_8k_page( libspectrum_word base, int source, int page );
int unittests_assert_16k_page( libspectrum_word base, int source, int page );
int unittests_assert_16k_ram_page( libspectrum_word base, int page );

int unittests_paging_test_48( int ram8000 );

#endif				/* #ifndef FUSE_UNITTESTS_H */
