/* psg.h: recording AY chip output to .psg files
   Copyright (c) 2003-2016 Matthew Westcott, Philip Kendall

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

#ifndef FUSE_PSG_H
#define FUSE_PSG_H

#include <libspectrum.h>

/* Are we currently recording a .psg file? */
extern int psg_recording;

void psg_register_startup( void );

int psg_start_recording( const char *filename );
int psg_stop_recording( void );

int psg_frame( void );

int psg_write_register( libspectrum_byte reg, libspectrum_byte value );

#endif			/* #ifndef FUSE_PSG_H */
