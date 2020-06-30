/* zxmmc.h: ZXMMC interface routines
   Copyright (c) 2017 Philip Kendall, Sergio Baldoví

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

#ifndef FUSE_ZXMMC_H
#define FUSE_ZXMMC_H

#include <libspectrum.h>

void zxmmc_register_startup( void );
int zxmmc_insert( const char *filename );
void zxmmc_commit( void );
int zxmmc_eject( void );

#endif			/* #ifndef FUSE_ZXMMC_H */
