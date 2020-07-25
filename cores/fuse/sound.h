/* sound.h: Sound support
   Copyright (c) 2000-2004 Russell Marks, Matan Ziv-Av, Philip Kendall
   Copyright (c) 2016 Fredrick Meunier

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

#ifndef FUSE_SOUND_H
#define FUSE_SOUND_H

#include <libspectrum.h>

void sound_register_startup( void );

void sound_init( const char *device );
void sound_pause( void );
void sound_unpause( void );
void sound_end( void );
void sound_ay_write( int reg, int val, libspectrum_dword now );
void sound_ay_reset( void );
void sound_specdrum_write( libspectrum_word port, libspectrum_byte val );
void sound_covox_write( libspectrum_word port, libspectrum_byte val );
void sound_frame( void );
void sound_beeper( libspectrum_dword at_tstates, int on );
libspectrum_dword sound_get_effective_processor_speed( void );

extern int sound_enabled;
extern int sound_framesiz;

/* Stereo separation types:
 *  * ACB is used in the Melodik interface.
 *  * ABC stereo is used in the Pentagon/Scorpion.
 *  * BAC stereo does seem to exist but is quite rare:
 *      Z80Stealth emulates BAC stereo but that's about all.
 *  * CAB, BCA and CBA don't get many search results.
 */

#define SOUND_STEREO_AY_NONE	0
#define SOUND_STEREO_AY_ACB	1
#define SOUND_STEREO_AY_ABC	2

extern int sound_stereo_ay;

/* The low-level sound interface */

int sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr );
void sound_lowlevel_end( void );
void sound_lowlevel_frame( libspectrum_signed_word *data, int len );

#endif				/* #ifndef FUSE_SOUND_H */
