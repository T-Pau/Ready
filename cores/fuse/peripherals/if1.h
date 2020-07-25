/* if1.h: Interface 1 handling routines
   Copyright (c) 2004-2016 Gergely Szasz, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

   Gergely: szaszg@hu.inter.net

*/

#ifndef FUSE_IF1_H
#define FUSE_IF1_H

#include <libspectrum.h>

/* IF1 */
extern int if1_active;
extern int if1_available;

void if1_register_startup( void );

void if1_page( void );
void if1_unpage( void );
void if1_memory_map( void );

int if1_mdr_insert( int drive, const char *filename );
int if1_mdr_write( int drive, const char *filename );
int if1_mdr_eject( int drive );
int if1_mdr_save( int drive, int saveas );
void if1_mdr_writeprotect( int drive, int wrprot );
void if1_plug( const char *filename, int what );
void if1_unplug( int what );

void if1_update_menu( void );

int if1_unittest( void );

#endif				/* #ifndef FUSE_IF1_H */
