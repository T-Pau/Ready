/* spectrum.h: Spectrum 48K specific routines
   Copyright (c) 1999-2016 Philip Kendall, Darren Salt

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

#ifndef FUSE_SPECTRUM_H
#define FUSE_SPECTRUM_H

#include <stdlib.h>

#include <libspectrum.h>

#include "memory_pages.h"

/* How many tstates have elapsed since the last interrupt? (or more
   precisely, since the ULA last pulled the /INT line to the Z80 low) */
extern libspectrum_dword tstates;

/* Things relating to memory */

extern libspectrum_byte RAM[ SPECTRUM_RAM_PAGES ][0x4000];

typedef int
  (*spectrum_port_from_ula_function)( libspectrum_word port );
typedef libspectrum_byte
  (*spectrum_contention_delay_function)( libspectrum_dword time );

typedef struct spectrum_raminfo {

  /* Is this port result supplied by the ULA? */
  spectrum_port_from_ula_function port_from_ula;

  /* What's the actual delay at the current tstate with MREQ active */
  spectrum_contention_delay_function contend_delay;

  /* And without MREQ */
  spectrum_contention_delay_function contend_delay_no_mreq;

  int locked;			/* Is the memory configuration locked? */
  int current_page, current_rom; /* Current paged memory */

  libspectrum_byte last_byte;	/* The last byte sent to the 128K port */
  libspectrum_byte last_byte2;	/* The last byte sent to +3 port */

  int special;			/* Is a +3 special config in use? */

  int romcs;			/* Is the /ROMCS line low? */

  int valid_pages;		/* Available RAM */

} spectrum_raminfo;

libspectrum_byte spectrum_contend_delay_none( libspectrum_dword time );
libspectrum_byte spectrum_contend_delay_65432100( libspectrum_dword time );
libspectrum_byte spectrum_contend_delay_76543210( libspectrum_dword time );

libspectrum_byte spectrum_unattached_port( void );
libspectrum_byte spectrum_unattached_port_none( void );

/* Miscellaneous stuff */

extern int spectrum_frame_event;

void spectrum_register_startup( void );
int spectrum_frame( void );

#endif			/* #ifndef FUSE_SPECTRUM_H */
