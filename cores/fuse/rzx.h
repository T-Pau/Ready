/* rzx.h: .rzx files
   Copyright (c) 2002-2016 Philip Kendall

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

#ifndef FUSE_RZX_H
#define FUSE_RZX_H

#include <libspectrum.h>

/* The offset used to get the count of instructions from the R register */
extern int rzx_instructions_offset;

/* The number of bytes read via IN during the current frame */
extern size_t rzx_in_count;

/* And the values of those bytes */
extern libspectrum_byte *rzx_in_bytes;

/* How big is the above array? */
extern size_t rzx_in_allocated;

/* Are we currently recording a .rzx file? */
extern int rzx_recording;

/* Are we currently playing back a .rzx file? */
extern int rzx_playback;

/* Is the .rzx file being recorded in competition mode? */
extern int rzx_competition_mode;

/* The number of instructions in the current .rzx playback frame */
extern size_t rzx_instruction_count;

/* The actual RZX data */
extern libspectrum_rzx *rzx;

void rzx_register_startup( void );

int rzx_start_recording( const char *filename, int embed_snapshot );
int rzx_stop_recording( void );
int rzx_continue_recording( const char *filename );
int rzx_finalise_recording( const char *filename );

int rzx_start_playback( const char *filename, int check_snapshot );
int
rzx_start_playback_from_buffer( const unsigned char *buffer, size_t length );

int rzx_stop_playback( int add_interrupt );

int rzx_frame( void );

int rzx_store_byte( libspectrum_byte value );

int rzx_rollback( void );

int rzx_rollback_to( void );

#endif			/* #ifndef FUSE_RZX_H */
