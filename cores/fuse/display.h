/* display.h: Routines for printing the Spectrum's screen
   Copyright (c) 1999-2006 Philip Kendall

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

#ifndef FUSE_DISPLAY_H
#define FUSE_DISPLAY_H

#include <stddef.h>

#include <libspectrum.h>

/* The width and height of the Speccy's screen */
#define DISPLAY_WIDTH_COLS  32
#define DISPLAY_HEIGHT_ROWS 24

/* The width and height of the Speccy's screen */
/* Each main screen column can produce 16 pixels in hires mode */
#define DISPLAY_WIDTH         ( DISPLAY_WIDTH_COLS * 16 )
/* Each main screen row can produce only 8 pixels in any mode */
#define DISPLAY_HEIGHT        ( DISPLAY_HEIGHT_ROWS * 8 )

/* The width and height of the (emulated) border */
#define DISPLAY_BORDER_WIDTH_COLS  4
#define DISPLAY_BORDER_HEIGHT_COLS 3

/* The width and height of the (emulated) border */
/* Each main screen column can produce 16 pixels in hires mode */
#define DISPLAY_BORDER_WIDTH  ( DISPLAY_BORDER_WIDTH_COLS * 16 )
/* Aspect corrected border width */
#define DISPLAY_BORDER_ASPECT_WIDTH  ( DISPLAY_BORDER_WIDTH_COLS * 8 )
/* Each main screen row can produce only 8 pixels in any mode */
#define DISPLAY_BORDER_HEIGHT ( DISPLAY_BORDER_HEIGHT_COLS * 8 )

/* The width and height of the window we'll be displaying */
#define DISPLAY_SCREEN_WIDTH  ( DISPLAY_WIDTH  + 2 * DISPLAY_BORDER_WIDTH  )
#define DISPLAY_SCREEN_HEIGHT ( DISPLAY_HEIGHT + 2 * DISPLAY_BORDER_HEIGHT )

/* And the width in columns */
#define DISPLAY_SCREEN_WIDTH_COLS ( DISPLAY_WIDTH_COLS + 2 * DISPLAY_BORDER_WIDTH_COLS )

/* The aspect ratio corrected display width */
#define DISPLAY_ASPECT_WIDTH  ( DISPLAY_SCREEN_WIDTH / 2 )

extern int display_ui_initialised;

extern libspectrum_byte display_lores_border;
extern libspectrum_byte display_hires_border;

extern libspectrum_dword
display_last_screen[ DISPLAY_SCREEN_WIDTH_COLS * DISPLAY_SCREEN_HEIGHT ];

/* Offsets as to where the data and the attributes for each pixel
   line start */
extern libspectrum_word display_line_start[ DISPLAY_HEIGHT ];
extern libspectrum_word display_attr_start[ DISPLAY_HEIGHT ];

typedef struct display_startup_context {
  int *argc;
  char ***argv;
} display_startup_context;

int display_init( int *argc, char ***argv );
void display_register_startup( display_startup_context *context );
void display_line(void);

typedef void (*display_dirty_fn)( libspectrum_word offset );
/* Function to use to mark as 'dirty' the pixels which have been changed by a
   write to 'offset' within the RAM page containing the screen */
extern display_dirty_fn display_dirty;
void display_dirty_timex( libspectrum_word offset );
void display_dirty_pentagon_16_col( libspectrum_word offset );
void display_dirty_sinclair( libspectrum_word offset );

typedef void (*display_write_if_dirty_fn)( int x, int y );
/* Function to write a dirty 8x1 chunk of pixels to the display */
extern display_write_if_dirty_fn display_write_if_dirty;
void display_write_if_dirty_timex( int x, int y );
void display_write_if_dirty_pentagon_16_col( int x, int y );
void display_write_if_dirty_sinclair( int x, int y );

typedef void (*display_dirty_flashing_fn)(void);
/* Function to dirty the pixels which are changed by virtue of having a flash
   attribute */
extern display_dirty_flashing_fn display_dirty_flashing;
void display_dirty_flashing_timex(void);
void display_dirty_flashing_pentagon_16_col(void);
void display_dirty_flashing_sinclair(void);

void display_parse_attr( libspectrum_byte attr, libspectrum_byte *ink,
			 libspectrum_byte *paper );

void display_set_lores_border(int colour);
void display_set_hires_border(int colour);
int display_dirty_border(void);

int display_frame(void);
void display_refresh_main_screen(void);
void display_refresh_all(void);

#define display_get_offset( x, y ) display_line_start[(y)]+(x)

#define display_get_addr( x, y ) \
  scld_last_dec.name.altdfile ? display_get_offset( (x), (y) )+ALTDFILE_OFFSET : \
  display_get_offset( (x), (y) )
int display_getpixel( int x, int y );

void display_update_critical( int x, int y );

#endif			/* #ifndef FUSE_DISPLAY_H */
