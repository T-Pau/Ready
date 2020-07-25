/* widget_internals.h: Functions internal to the widget code
   Copyright (c) 2001-2005 Matan Ziv-Av, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

#ifndef FUSE_WIDGET_INTERNALS_H
#define FUSE_WIDGET_INTERNALS_H

#include <sys/types.h>
#include <stdlib.h>

#include <libspectrum.h>

#include "settings.h"
#include "widget.h"

/* The default colours used in the widget */
#define WIDGET_COLOUR_DISABLED   7	/* White */
#define WIDGET_COLOUR_BACKGROUND 15	/* Bright White */
#define WIDGET_COLOUR_FOREGROUND 0	/* Black */
#define WIDGET_COLOUR_HIGHLIGHT  13	/* Cyan */
#define WIDGET_COLOUR_TITLE      WIDGET_COLOUR_BACKGROUND

/* The ways of finishing a widget */
typedef enum widget_finish_state {
  WIDGET_FINISHED_OK = 1,
  WIDGET_FINISHED_CANCEL,
} widget_finish_state;

/* A function to draw a widget */
typedef int (*widget_draw_fn)( void *data );

/* The information we need to store for each widget */
typedef struct widget_t {
  widget_draw_fn draw;			/* Draw this widget */
  int (*finish)( widget_finish_state finished ); /* Post-widget processing */
  widget_keyhandler_fn keyhandler;	/* Keyhandler */
} widget_t;

int widget_end_widget( widget_finish_state state );
int widget_end_all( widget_finish_state state );

int widget_timer_init( void );
int widget_timer_end( void );

void widget_putpixel( int x, int y, int colour );
void widget_rectangle( int x, int y, int w, int h, int col );
void widget_draw_line_horiz( int x, int y, int length, int colour );
void widget_draw_line_vert( int x, int y, int length, int colour );
void widget_draw_rectangle_outline( int x, int y, int w, int h, int colour );
void widget_draw_rectangle_solid( int x, int y, int w, int h, int colour );
void widget_draw_rectangle_outline_rounded( int x, int y, int w, int h, int colour );
int widget_printstring( int x, int y, int col, const char *s );
int widget_printstring_fixed( int x, int y, int col, const char *s );
void widget_printchar_fixed( int x, int y, int col, int c );
void widget_print_title( int y, int col, const char *s );
void widget_printstring_right( int x, int y, int col, const char *s );
void widget_display_rasters( int y, int h );
#define widget_display_lines(y,h) widget_display_rasters((y)*8,(h)*8)

size_t widget_stringwidth( const char *s );
size_t widget_substringwidth( const char *s, size_t count );
size_t widget_charwidth( int c );

void widget_up_arrow( int x, int y, int colour );
void widget_down_arrow( int x, int y, int colour );
void widget_draw_submenu_arrow(int x, int y, int colour);
void widget_print_checkbox( int x, int y, int colour, int value );

extern widget_finish_state widget_finished;

int widget_dialog( int x, int y, int width, int height );
int widget_dialog_with_border( int x, int y, int width, int height );

int split_message( const char *message, char ***lines, size_t *count,
		   const size_t line_length );

/* File selector */

typedef struct widget_dirent {
  int mode;
  char *name;
} widget_dirent;

typedef struct widget_filesel_data {
  int exit_all_widgets;
  const char *title;
} widget_filesel_data;

extern struct widget_dirent **widget_filenames;
extern size_t widget_numfiles;

int widget_filesel_load_draw( void* data );
int widget_filesel_save_draw( void* data );
int widget_filesel_finish( widget_finish_state finished );
void widget_filesel_keyhandler( input_key key );

/* Tape menu */

int widget_tape_draw( void* data );
void widget_tape_keyhandler( input_key key );

/* File menu */

int widget_file_draw( void* data );
void widget_file_keyhandler( input_key key );

/* Options menu */
int widget_menu_filter( void *data );

/* Machine menu */

int widget_machine_draw( void* data );
void widget_machine_keyhandler( input_key key );

/* Keyboard picture */

typedef struct widget_picture_data {
  const char *filename;
  libspectrum_byte *screen;
  int border;
} widget_picture_data;

int widget_picture_draw( void* data );
void widget_picture_keyhandler( input_key key );

/* Help menu */

int widget_help_draw( void* data );
void widget_help_keyhandler( input_key key );

/* General menu code */

int widget_menu_draw( void* data );
void widget_menu_keyhandler( input_key key );

/* More callbacks */
scaler_type widget_select_scaler( int (*selector)( scaler_type ) );

/* The generalised selector widget */

typedef struct widget_select_t {

  const char *title;	/* Dialog title */
  const char * const *options;	/* The available options */
  size_t count;		/* The number of options */
  size_t current;	/* Which option starts active? */

  int result;		/* What was selected? ( -1 if dialog cancelled ) */
  int finish_all;	/* close all widget or not */

} widget_select_t;

int widget_select_draw( void *data );
void widget_select_keyhandler( input_key key );
int widget_select_finish( widget_finish_state finished );

/* The tape browser widget */

int widget_browse_draw( void* data );
void widget_browse_keyhandler( input_key key );
int widget_browse_finish( widget_finish_state finished );

/* The text entry widget */

typedef enum widget_text_input_allow {
  WIDGET_INPUT_ASCII,
  WIDGET_INPUT_DIGIT,
  WIDGET_INPUT_ALPHA,
  WIDGET_INPUT_ALNUM
} widget_text_input_allow;

typedef struct widget_text_t {
  const char *title;
  widget_text_input_allow allow; 
  unsigned int max_length;
  char text[40];
} widget_text_t;

int widget_text_draw( void* data );
void widget_text_keyhandler( input_key key );
int widget_text_finish( widget_finish_state finished );

extern char *widget_text_text;	/* The returned text */

/* The options widgets */
int widget_options_finish( widget_finish_state finished );

/* The error widget */

int widget_error_draw( void *data );
void widget_error_keyhandler( input_key key );

/* The debugger widget */

int widget_debugger_draw( void *data );
void widget_debugger_keyhandler( input_key key );

/* The poke file widget */

int widget_pokemem_draw( void *data );
void widget_pokemem_keyhandler( input_key key );
int widget_pokemem_finish( widget_finish_state finished );

/* The poke finder widget */

int widget_pokefinder_draw( void *data );
void widget_pokefinder_keyhandler( input_key key );

/* The memory browser widget */

int widget_memory_draw( void *data );
void widget_memory_keyhandler( input_key key );

/* The about fuse widget */

int widget_about_draw( void *data );
void widget_about_keyhandler( input_key key );

/* The ROM selector widget */

typedef struct widget_roms_info {

  int initialised;

  const char *title;
  size_t start, count;
  int is_peripheral;

} widget_roms_info;

int widget_roms_draw( void *data );
void widget_roms_keyhandler( input_key key );
int widget_roms_finish( widget_finish_state finished );

/* The query widgets */

typedef union {
  int confirm;
  ui_confirm_save_t save;
} widget_query_t;

extern widget_query_t widget_query;

int widget_query_draw( void *data );
void widget_query_keyhandler( input_key key );
int widget_query_save_draw( void *data );
void widget_query_save_keyhandler( input_key key );
int widget_query_finish( widget_finish_state finished );

/* The widgets actually available */

extern widget_t widget_data[];

#endif				/* #ifndef FUSE_WIDGET_INTERNALS_H */
