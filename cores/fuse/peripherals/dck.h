/* dck.h: dock snapshot (Warajevo .DCK) handling routines
   Copyright (c) 2003 Darren Salt, Fredrick Meunier

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

   Darren: linux@youmustbejoking.demon.co.uk
   Fred: fredm@spamcop.net

*/

#ifndef FUSE_DCK_H
#define FUSE_DCK_H

/* Dock cart inserted? */
extern int dck_active;

int dck_insert( const char *filename );
void dck_eject( void );
int dck_reset( void );

#endif
