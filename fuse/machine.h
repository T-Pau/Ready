/* machine.h: Routines for handling the various machine types
   Copyright (c) 1999-2011 Philip Kendall

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

#ifndef FUSE_MACHINE_H
#define FUSE_MACHINE_H

#include <stdlib.h>

#include <libspectrum.h>

#include "display.h"
#include "peripherals/ay.h"
#include "peripherals/covox.h"
#include "peripherals/specdrum.h"
#include "spectrum.h"

typedef libspectrum_byte (*spectrum_unattached_port_fn)( void );

/* How long do things take to happen; fields are pulled from libspectrum
   via the libspectrum_timings_* functions */
typedef struct machine_timings {

  /* Processor speed */
  libspectrum_dword processor_speed;

  /* Line timings in tstates */
  libspectrum_word left_border, horizontal_screen, right_border;
  libspectrum_word tstates_per_line;

  /* Interrupt length */
  libspectrum_word interrupt_length;

  /* Frame timing */
  libspectrum_dword tstates_per_frame;

} machine_timings;

typedef struct fuse_machine_info {

  libspectrum_machine machine; /* which machine type is this? */
  const char *id;	/* Used to select from command line */
  int capabilities;	/* Capabilities of this machine */

  int (*reset)(void);	/* Reset function */

  int timex;      /* Timex machine (keyboard emulation/loading sounds etc.) */

  machine_timings timings; /* How long do things take to happen? */
  /* Redraw line y this many tstates after interrupt */
  libspectrum_dword line_times[DISPLAY_SCREEN_HEIGHT+1];

  spectrum_raminfo ram; /* How do we access memory, and what's currently
			   paged in */

  spectrum_unattached_port_fn unattached_port; /* What to return if we read
						  from a port which isn't
						  attached to anything */

  ayinfo ay;		/* The AY-3-8912 chip */

  specdrum_info specdrum; /* SpecDrum settings */

  covox_info covox; /* Covox settings */

  int (*shutdown)( void );

  int (*memory_map)( void );

} fuse_machine_info;

extern fuse_machine_info **machine_types;	/* All available machines */
extern int machine_count;		/* of which there are this many */

extern fuse_machine_info *machine_current;	/* The currently selected machine */

void machine_register_startup( void );

int machine_select( libspectrum_machine type );
int machine_select_id( const char *id );
const char* machine_get_id( libspectrum_machine type );

int machine_load_rom_bank_from_buffer( memory_page* bank_map, int page_num,
  unsigned char *buffer, size_t length, int custom );
int machine_load_rom_bank( memory_page* bank_map, int page_num,
  const char *filename, const char *fallback, size_t expected_length );
int machine_load_rom( int page_num, const char *filename, const char *fallback,
  size_t expected_length );

int machine_reset( int hard_reset );

#endif			/* #ifndef FUSE_MACHINE_H */
