/* fuse.h: Variables exported from the main file
   Copyright (c) 2000-2015 Philip Kendall

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

#ifndef FUSE_FUSE_H
#define FUSE_FUSE_H

#include <limits.h>

#include <libspectrum.h>

#include "compat.h"

extern const char *fuse_progname;	/* argv[0] */

extern int fuse_exiting;		/* Shall we exit now? */

extern int fuse_emulation_paused;	/* Is Spectrum emulation paused? */
int fuse_emulation_pause(void);		/* Stop and start emulation */
int fuse_emulation_unpause(void);

#ifdef UI_WIN32
int fuse_main(int argc, char **argv);
#endif

void fuse_abort( void ) GCC_NORETURN;	/* Emergency shutdown */

extern libspectrum_creator *fuse_creator; /* Creator information for file
					     formats which support this */

#endif			/* #ifndef FUSE_FUSE_H */
