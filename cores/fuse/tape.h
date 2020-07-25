/* tape.h: tape handling routines
   Copyright (c) 1999-2016 Philip Kendall
   Copyright (c) 2015 Sergio Baldoví

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

#ifndef FUSE_TAPE_H
#define FUSE_TAPE_H

#include <libspectrum.h>

void tape_register_startup( void );

int tape_open( const char *filename, int autoload );

int
tape_read_buffer( unsigned char *buffer, size_t length, libspectrum_id_t type,
		  const char *filename, int autoload );

int tape_close( void );
int tape_rewind( void );
int tape_select_block( size_t n );
int tape_select_block_no_update( size_t n );
int tape_get_current_block( void );
int tape_write( const char *filename );

int tape_can_autoload( void );

int tape_load_trap( void );
int tape_save_trap( void );

int tape_do_play( int autoplay );
int tape_toggle_play( int autoplay );

void tape_next_edge( libspectrum_dword last_tstates, int from_acceleration );

int tape_stop( void );
int tape_is_playing( void );
int tape_present( void );

void tape_record_start( void );
int tape_record_stop( void );

/* Call a user-supplied function for every block in the current tape */
int
tape_foreach( void (*function)( libspectrum_tape_block *block,
				void *user_data),
	      void *user_data );

/* Fill 'buffer' with up a maximum of 'length' characters of
   information about 'block' */
int tape_block_details( char *buffer, size_t length,
			libspectrum_tape_block *block );

extern int tape_microphone;
extern int tape_modified;
extern int tape_playing;
extern int tape_recording;

extern int tape_edge_event;

#endif
