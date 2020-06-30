/* divmmc.h: DivMMC interface routines
   Copyright (c) 2005-2016 Matthew Westcott, Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#ifndef FUSE_DIVMMC_H
#define FUSE_DIVMMC_H

#include <libspectrum.h>

/* Whether DivMMC is currently paged in */
extern int divmmc_active;

/* Notify DivMMC hardware of an opcode fetch to one of the designated
   entry / exit points. Depending on configuration, it may or may not
   result in the DivMMC memory being paged in */
void divmmc_set_automap( int state );

/* Call this after some state change other than an opcode fetch which could
   trigger DivMMC paging (such as updating the write-protect flag), to
   re-evaluate whether paging will actually happen */
void divmmc_refresh_page_state( void );

void divmmc_register_startup( void );
int divmmc_insert( const char *filename );
void divmmc_commit( void );
int divmmc_eject( void );

int divmmc_unittest( void );

#endif			/* #ifndef FUSE_DIVMMC_H */
