/* uidisplay.h: Low-level display routines
   Copyright (c) 2000-2003 Philip Kendall

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

#ifndef FUSE_UIDISPLAY_H
#define FUSE_UIDISPLAY_H

#include <libspectrum.h>

/* User interface specific functions */

int uidisplay_init( int width, int height );

void uidisplay_area( int x, int y, int w, int h );
void uidisplay_frame_end( void );
int uidisplay_hotswap_gfx_mode( void );

#ifdef USE_WIDGET
/* Routines for backing up and restoring the frame buffer as the widget UI does
   it's work */
void uidisplay_frame_save( void );
void uidisplay_frame_restore( void );
#endif                          /* #ifdef USE_WIDGET */

int uidisplay_end(void);

/* General functions */

void uidisplay_spectrum_screen( const libspectrum_byte *screen, int border );

void uidisplay_putpixel( int x, int y, int colour );
void uidisplay_plot8( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                      libspectrum_byte paper );
void uidisplay_plot16( int x, int y, libspectrum_word data, libspectrum_byte ink,
                       libspectrum_byte paper);

#endif			/* #ifndef FUSE_UIDISPLAY_H */
