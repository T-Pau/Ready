/* widget.c: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001-2005 Matan Ziv-Av, Philip Kendall, Russell Marks
   Copyright (c) 2015 Stuart Brady
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

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "fuse.h"
#include "display.h"
#include "machine.h"
#include "ui/uidisplay.h"
#include "keyboard.h"
#include "menu.h"
#include "periph.h"
#include "peripherals/joystick.h"
#include "pokefinder/pokefinder.h"
#include "screenshot.h"
#include "timer/timer.h"
#include "ui/widget/options_internals.h"
#include "ui/widget/widget_internals.h"
#include "utils.h"

#ifdef WIN32
#include <windows.h>
#endif

/* Bitmap font storage */
typedef struct {
  libspectrum_byte bitmap[15], left, width, defined;
} widget_font_character;

static widget_font_character *widget_font[1] = {0};

static const widget_font_character default_invalid = {
  { 0x7E, 0xDF, 0x9F, 0xB5, 0xA5, 0x8F, 0xDF, 0x7E }, 0, 8, 1
}; /* "(?)" (inv) */

static const widget_font_character default_unknown = {
  { 0x7C, 0xDE, 0xBE, 0xAA, 0xDE, 0x7C }, 1, 6, 1
}; /* "(?)" */

static const widget_font_character default_keyword = {
  { 0x7C, 0x82, 0xEE, 0xD6, 0xBA, 0x7C }, 1, 6, 1
}; /* "(K)" */

/* The current widget keyhandler */
widget_keyhandler_fn widget_keyhandler;

/* The data used for recursive widgets */
typedef struct widget_recurse_t {

  widget_type type;		/* Which type of widget are we? */
  void *data;			/* What data were we passed? */

  int finished;			/* Have we finished this widget yet? */

} widget_recurse_t;

static widget_recurse_t widget_return[10]; /* The stack to recurse on */

/* The settings used whilst playing with an options dialog box */
settings_info widget_options_settings;

static int widget_read_font( const char *filename )
{
  utils_file file;
  int error;
  int i;

  error = utils_read_auxiliary_file( filename, &file, UTILS_AUXILIARY_WIDGET );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find font file '%s'", filename );
    return 1;
  }
  if( error ) return error;

  i = 0;
  while( i < file.length ) {
    int code, page, left, width;

    if( i + 3 > file.length ) {
      ui_error( UI_ERROR_ERROR, "font contains invalid character" );
      utils_close_file( &file );
      return 1;
    }

    code = file.buffer[i];
    page = file.buffer[i+1];
    if( page == 0 && ( code == 0xA3 || ( code < 0x7F && code != 0x60 ) ) ) {
      left = file.buffer[i+2] & 7;
    } else {
      left = -1;
    }
    width = file.buffer[i+2] >> 4 & 15;

    /* weed out invalid character codes and misdefined characters */
    if( page != 0 /* we don't currently have more than page 0 */
	|| i + 3 + width > file.length || (left >= 0 && left + width > 8) )
    {
      ui_error( UI_ERROR_ERROR, "font contains invalid character" );
      utils_close_file( &file );
      return 1;
    }

    if( !widget_font[page] )
    {
      widget_font[page] = calloc( 256, sizeof( widget_font_character ) );
      if( !widget_font[page] )
      {
        ui_error( UI_ERROR_ERROR, "out of memory" );
        utils_close_file( &file );
        return 1;
      }
    }

    widget_font[page][code].defined = 1;
    widget_font[page][code].left = left < 0 ? 0 : left;
    widget_font[page][code].width = width ? width : 3;
    memcpy( &widget_font[page][code].bitmap, &file.buffer[i+3], width );

    i += 3 + width;
  }

  utils_close_file( &file );

  return 0;
}

static const widget_font_character *
widget_char( int pp )
{
  if( pp < 0 || pp >= 256 ) return &default_invalid;
  if( !widget_font[pp >> 8] || !widget_font[pp >> 8][pp & 255].defined )
    return &default_unknown;
  return &widget_font[ pp >> 8 ][ pp & 255 ];
}

static int
printchar( int x, int y, int col, int ch )
{
  int mx, my;
  const widget_font_character *bitmap = widget_char( ch );

  for( mx = 0; mx < bitmap->width; mx++ ) {
    int b = bitmap->bitmap[mx];
    for( my = 0; my < 8; my++ )
      if( b & 128 >> my ) widget_putpixel( x + mx, y + my, col );
  }

  return x + bitmap->width + 1;
}

int
widget_printstring( int x, int y, int col, const char *s )
{
  int c;
  int shadow = 0;
  if( !s ) return x;

  while( x < 256 + DISPLAY_BORDER_ASPECT_WIDTH
	 && ( c = *(libspectrum_byte *)s++ ) != 0 ) {
    if( col == WIDGET_COLOUR_DISABLED && c < 26 ) continue;
    if( col != WIDGET_COLOUR_DISABLED ) {
      if( c && c < 17 ) { col = c - 1; continue; }
      if( c < 26 ) { shadow = c - 17; continue; }
    }

    if( shadow && col ) {
      printchar( x - 1, y,     shadow - 1, c );
      printchar( x + 1, y,     shadow - 1, c );
      printchar( x,     y - 1, shadow - 1, c );
      printchar( x,     y + 1, shadow - 1, c );
      printchar( x + 1, y + 1, shadow - 1, c );
      x = printchar( x, y, (col & 7) ^ 8, c );
    } else
      x = printchar( x, y, col, c );
  }
  return x;
}

int
widget_printstring_fixed( int x, int y, int col, const char *s )
{
  int c;

  if( !s ) return x;

  while( x < 256 + DISPLAY_BORDER_ASPECT_WIDTH
	 && ( c = *(libspectrum_byte *)s++ ) != 0 ) {
    widget_printchar_fixed(x, y, col, c);
    ++x;
  }
  return x;
}

void
widget_printchar_fixed( int x, int y, int col, int c )
{
  int mx, my;
  int inverse = 0;
  const widget_font_character *bitmap;

  x *= 8; y *= 8;

  if( c < 128 )
    bitmap = widget_char( c );
  else if( c < 144 ) {
    if( c & 1 ) widget_rectangle( x + 4, y    , 4, 4, col );
    if( c & 2 ) widget_rectangle( x    , y    , 4, 4, col );
    if( c & 4 ) widget_rectangle( x + 4, y + 4, 4, 4, col );
    if( c & 8 ) widget_rectangle( x    , y + 4, 4, 4, col );
    return;
  } else if( c < 165 ) {
    inverse = 1;
    bitmap = widget_char( c - 144 + 'A' );
  } else {
    bitmap = &default_keyword;
  }

  x += bitmap->left;
  for( mx = 0; mx < bitmap->width; mx++ ) {
    int b = bitmap->bitmap[mx];
    if( inverse ) b = ~b;
    for( my = 0; my < 8; my++ )
      if( b & 128 >> my ) widget_putpixel( x + mx, y + my, col );
  }
}

void widget_print_title( int y, int col, const char *s )
{
  char buffer[128];
  snprintf( buffer, sizeof( buffer ), "\x0A%s", s );
  widget_printstring( 128 - widget_stringwidth( buffer ) / 2, y, col, buffer );
}

void widget_printstring_right( int x, int y, int col, const char *s )
{
  widget_printstring( x - widget_stringwidth( s ), y, col, s );
}

size_t widget_stringwidth( const char *s )
{
  return widget_substringwidth( s, UINT_MAX );
}

size_t widget_substringwidth( const char *s, size_t count )
{
  size_t width = 0;
  int c;
  if( !s )
    return 0;
  while( count-- && (c = *(libspectrum_byte *)s++) != 0 ) {
    if( c < 18 )
      continue;
    width += widget_char( c )->width + 1;
  }
  return width - 1;
}

size_t widget_charwidth( int c )
{
  return widget_char( c )->width;
}

void widget_rectangle( int x, int y, int w, int h, int col )
{
    int mx, my;
    
    for( my = 0; my < h; my++ )
      for( mx = 0; mx < w; mx++ )
        widget_putpixel( x + mx, y + my, col );
}

void
widget_draw_line_horiz(int x, int y, int length, int colour )
{
  int i;

  for (i=0; i<length; i++) {
    uidisplay_putpixel( x+i, y, colour );
  }
}

void
widget_draw_line_vert(int x, int y, int length, int colour )
{
  int i;

  for (i=0; i<length; i++) {
    uidisplay_putpixel( x, y+i, colour );
  }
}

void
widget_draw_rectangle_outline(int x, int y, int w, int h, int colour )
{
  widget_draw_line_horiz( x, y, w, colour );
  widget_draw_line_horiz( x, y+h-1, w, colour );
  widget_draw_line_vert( x, y, h, colour );
  widget_draw_line_vert( x+w-1, y, h, colour );
}   

void
widget_draw_rectangle_solid( int x, int y, int w, int h, int colour )
{
  int v, p;

  if( y < 0 ) {
    h = y + h;
    y = 0;
  }

  if( x < 0 ) {
    w = x + w;
    x = 0;
  }

  /* clip rectangle to screen edges */
  if( x + w > DISPLAY_SCREEN_WIDTH - 1 )
    w = DISPLAY_SCREEN_WIDTH - x;

  if( y + h > DISPLAY_SCREEN_HEIGHT - 1 )
    h = DISPLAY_SCREEN_HEIGHT - y;

  for (v=0; v<h; v++) {
    for (p=0; p<w; p++) {
        uidisplay_putpixel( x+p, y+v, colour );
    }
  }
}

void
widget_draw_rectangle_outline_rounded( int x, int y, int w, int h, int colour )
{
  widget_draw_line_horiz( x+1, y, w-2, colour );
  widget_draw_line_horiz( x+1, y+h-1, w-2, colour );
  widget_draw_line_vert( x, y+1, h-2, colour );
  widget_draw_line_vert( x+w-1, y+1, h-2, colour );

  uidisplay_putpixel( x+1, y+h-2, colour );
  uidisplay_putpixel( x+1, y+1, colour );
  uidisplay_putpixel( x+w-2, y+1, colour );
  uidisplay_putpixel( x+w-2, y+h-2, colour );
}

void
widget_draw_submenu_arrow(int x, int y, int colour)
{
  widget_draw_line_vert(x + 2, y, 6, colour);
  widget_draw_line_vert(x + 3, y + 1, 4, colour);
  widget_draw_line_vert(x + 4, y + 2, 2, colour);
}

void widget_print_checkbox( int x, int y, int colour, int value )
{
    static const int CHECK_COLOR=4;
    int z;

    y += 2;
    x += 6;
    widget_rectangle( x, y - 1, 3, 3, colour );
    widget_rectangle( x - 5, y, 5, 5, 0 );
    widget_rectangle( x - 4, y + 1, 3, 3, colour );
    if( value ) {	/* checked */
      for( z = -1; z < 3; z++ ) {
        widget_putpixel( x - z, y + z, CHECK_COLOR );
        widget_putpixel( x - z + 1, y + z, CHECK_COLOR );
      }
      widget_putpixel( x - z + 1, y + z, CHECK_COLOR );
      widget_putpixel( x - z, y + z - 1, CHECK_COLOR );
      widget_putpixel( x - z, y + z - 2, CHECK_COLOR );
      widget_putpixel( x - z - 1, y + z - 2, CHECK_COLOR );
    }
}

/* Arrows for any scrollable widget */
void
widget_up_arrow( int x, int y, int colour )
{
  int i, j;
  x = x * 8 + 1;
  y = y * 8 + 7;
  for( j = 6; j; --j ) {
    for( i = (j + 1) / 2; i < 4; ++i ) {
      widget_putpixel( x +     i, y - j, colour );
      widget_putpixel( x + 7 - i, y - j, colour );
    }
  }
}

void
widget_down_arrow( int x, int y, int colour )
{
  int i, j;
  x = x * 8 + 1;
  y = y * 8;
  for( j = 6; j; --j ) {
    for( i = (j + 1) / 2; i < 4; ++i ) {
      widget_putpixel( x +     i, y + j, colour );
      widget_putpixel( x + 7 - i, y + j, colour );
    }
  }
}

/* Force screen rasters y to (y+h) inclusive to be redrawn */
void
widget_display_rasters( int y, int h )
{
  int scale = machine_current->timex ? 2 : 1;

  uidisplay_area( 0, scale * ( DISPLAY_BORDER_HEIGHT + y ),
		  scale * DISPLAY_ASPECT_WIDTH, scale * h );
  uidisplay_frame_end();
}

/* Global initialisation/end routines */

int widget_init( void )
{
  int error;

  error = widget_read_font( "fuse.font" );
  if( error ) return error;

  widget_filenames = NULL;
  widget_numfiles = 0;

  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING_ROLLBACK, 0 );
  ui_menu_activate( UI_MENU_ITEM_TAPE_RECORDING, 0 );
#ifdef HAVE_LIB_XML2
  ui_menu_activate( UI_MENU_ITEM_FILE_SVG_CAPTURE, 0 );
#endif
  return 0;
}

int widget_end( void )
{
  size_t i;

  if( widget_filenames ) {
    for( i=0; i<widget_numfiles; i++) {
      free( widget_filenames[i]->name );
      free( widget_filenames[i] );
    }
    free( widget_filenames );
  }

  /* we don't currently have more than page 0 */
  free( widget_font[0] );

  return 0;
}

/* General widget routine */

int widget_do( widget_type which, void *data )
{
  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 1;

  if( which == WIDGET_TYPE_QUERY && !settings_current.confirm_actions ) {
    widget_query.confirm = 1;
    return 0;
  }

  if( ui_widget_level == -1 ) uidisplay_frame_save();

  /* We're now one widget level deeper */
  ui_widget_level++;

  /* Store what type of widget we are and what data we were given */
  widget_return[ui_widget_level].type = which;
  widget_return[ui_widget_level].data = data;

  uidisplay_frame_restore();

  /* Draw this widget */
  widget_data[ which ].draw( data );

  /* Set up the keyhandler for this widget */
  widget_keyhandler = widget_data[which].keyhandler;

  /* Process this widget until it returns */
  widget_return[ui_widget_level].finished = 0;
  while( ! widget_return[ui_widget_level].finished ) {
    
    /* Go to sleep for a bit */
    timer_sleep( 10 );

    /* Process any events */
    ui_event();
  }

  /* Do any post-widget processing if it exists */
  if( widget_data[which].finish ) {
    widget_data[which].finish( widget_return[ui_widget_level].finished );
  }

  uidisplay_frame_restore();

  /* Now return to the previous widget level */
  ui_widget_level--;
    
  if( ui_widget_level >= 0 ) {

    /* If we're going back to another widget, set up its keyhandler and
       draw it again, unless it's already finished */
    if( ! widget_return[ui_widget_level].finished ) {
      widget_keyhandler =
	widget_data[ widget_return[ui_widget_level].type ].keyhandler;
      widget_data[ widget_return[ui_widget_level].type ].draw(
        widget_return[ui_widget_level].data
      );
    }

  } else {

    /* Refresh the Spectrum's display, including the border */
    display_refresh_all();

  }

  return 0;
}

/* End the currently running widget */
int
widget_end_widget( widget_finish_state state )
{
  widget_return[ ui_widget_level ].finished = state;
  return 0;
}

/* End all currently running widgets */
int widget_end_all( widget_finish_state state )
{
  int i;

  for( i=0; i<=ui_widget_level; i++ )
    widget_return[i].finished = state;

  return 0;
}

void
widget_finish( void )
{
  widget_end_all( WIDGET_FINISHED_OK );
}

int widget_dialog( int x, int y, int width, int height )
{
  widget_rectangle( 8*x, 8*y, 8*width, 8*height, WIDGET_COLOUR_BACKGROUND );
  return 0;
}

static void
widget_draw_speccy_rainbow_bar(int x, int y)
{
  int i = 0;
  int cur_x = x - 8;

  for (i = 0; i < 8; i++) {
    widget_draw_line_horiz(cur_x, y + i, 8, 10);       /* bright red */
    widget_draw_line_horiz(cur_x + 8, y + i, 8, 14);   /* bright yellow */
    widget_draw_line_horiz(cur_x + 16, y + i, 8, 12);  /* bright green */
    widget_draw_line_horiz(cur_x + 24, y + i, 8, 13);  /* bright cyan */
    cur_x--;
  }
}

const int menu_vert_external_margin = 8;

int
widget_calculate_menu_width(widget_menu_entry *menu)
{
  widget_menu_entry *ptr;
  int max_width=0;

  if (!menu) {
    return 64;
  }

  max_width = widget_stringwidth( menu->text )+5*8;

  for( ptr = &menu[1]; ptr->text; ptr++ ) {
    int total_width = widget_stringwidth(ptr->text)+8;

    if( ptr->submenu ) {
      total_width += 3*8;
    }

    /* If this has extra details, leave room for the extra text */
    if( ptr->detail ) total_width += widget_stringwidth( ptr->detail() )+2*8;

    if (total_width > max_width)
      max_width = total_width;
  }

  return (max_width+menu_vert_external_margin*2)/8;
}


int widget_dialog_with_border( int x, int y, int width, int height )
{
  int ink = WIDGET_COLOUR_FOREGROUND;
  int paper = WIDGET_COLOUR_BACKGROUND;

  x += DISPLAY_BORDER_WIDTH_COLS;
  y += DISPLAY_BORDER_HEIGHT_COLS;

  widget_draw_rectangle_solid(8*x + 1, 8*y + 1, 8*width - 2,
                              8*height - 2, paper);
  widget_draw_rectangle_solid(8*x + 1, 8*y + 1, 8*width - 2, 7*1, ink);
  widget_draw_rectangle_outline_rounded(8*x, 8*y, 8*width, 8*height, ink);

  widget_draw_speccy_rainbow_bar(8*x + 8*width - 32, 8*y);

  return 0;
}

void
widget_putpixel( int x, int y, int colour )
{
  uidisplay_putpixel( x + DISPLAY_BORDER_ASPECT_WIDTH, y + DISPLAY_BORDER_HEIGHT,
                      colour );
}

/* The widgets actually available. Make sure the order here matches the
   order defined in enum widget_type (widget.h) */

widget_t widget_data[] = {

  { widget_filesel_load_draw, widget_filesel_finish, widget_filesel_keyhandler  },
  { widget_filesel_save_draw, widget_filesel_finish, widget_filesel_keyhandler  },
  { widget_general_draw,  widget_options_finish, widget_general_keyhandler  },
  { widget_picture_draw,  NULL,                  widget_picture_keyhandler  },
  { widget_about_draw,    NULL,                  widget_about_keyhandler    },
  { widget_menu_draw,	  NULL,			 widget_menu_keyhandler     },
  { widget_select_draw,   widget_select_finish,  widget_select_keyhandler   },
  { widget_media_draw,	  widget_options_finish, widget_media_keyhandler    },
  { widget_sound_draw,	  widget_options_finish, widget_sound_keyhandler    },
  { widget_error_draw,	  NULL,			 widget_error_keyhandler    },
  { widget_rzx_draw,      widget_options_finish, widget_rzx_keyhandler      },
  { widget_movie_draw,    widget_options_finish, widget_movie_keyhandler    },
  { widget_browse_draw,   widget_browse_finish,  widget_browse_keyhandler   },
  { widget_text_draw,	  widget_text_finish,	 widget_text_keyhandler     },
  { widget_debugger_draw, NULL,			 widget_debugger_keyhandler },
  { widget_pokefinder_draw, NULL,		 widget_pokefinder_keyhandler },
  { widget_pokemem_draw, widget_pokemem_finish,	widget_pokemem_keyhandler },
  { widget_memory_draw,   NULL,			 widget_memory_keyhandler   },
  { widget_roms_draw,     widget_roms_finish,	 widget_roms_keyhandler     },
  { widget_peripherals_general_draw, widget_options_finish, widget_peripherals_general_keyhandler },
  { widget_peripherals_disk_draw, widget_options_finish, widget_peripherals_disk_keyhandler },
  { widget_query_draw,    widget_query_finish,	 widget_query_keyhandler    },
  { widget_query_save_draw,widget_query_finish,	 widget_query_save_keyhandler },
  { widget_diskoptions_draw, widget_options_finish, widget_diskoptions_keyhandler  },
};

#ifndef UI_SDL
#ifndef UI_X
/* The statusbar handling functions */
/* TODO: make these do something useful */
int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  return 0;
}
#endif
#endif                          /* #ifndef UI_SDL */

/* Tape browser update function. The dialog box is created every time it
   is displayed, so no need to do anything here */
int
ui_tape_browser_update( ui_tape_browser_update_type change,
                        libspectrum_tape_block *block )
{
  return 0;
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  if( !settings_current.confirm_actions ) return UI_CONFIRM_SAVE_DONTSAVE;

  if( widget_do_query_save( message ) )
    return UI_CONFIRM_SAVE_CANCEL;
  return widget_query.confirm;
}

int
ui_query( const char *message )
{
  widget_do_query( message );
  return widget_query.save;
}

/* FIXME: make this do something useful */
int
ui_get_rollback_point( GSList *points )
{
  return -1;
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  widget_select_t info;
  int error;
  char title[80];

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  snprintf( title, sizeof( title ), "Configure %s joystick",
	    libspectrum_joystick_name( libspectrum_type ) );

  info.title = title;
  info.options = joystick_connection;
  info.count = JOYSTICK_CONN_COUNT;
  info.current = UI_CONFIRM_JOYSTICK_NONE;
  info.finish_all = 1;

  error = widget_do_select( &info );
  if( error ) return UI_CONFIRM_JOYSTICK_NONE;

  return (ui_confirm_joystick_t)info.result;
}

int
ui_widgets_reset( void )
{
  pokefinder_clear();
  return 0;
}

void
ui_popup_menu( int native_key )
{
  switch( native_key ) {
  case INPUT_KEY_F1:
    fuse_emulation_pause();
    widget_do_menu( widget_menu );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F2:
    fuse_emulation_pause();
    menu_file_savesnapshot( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F3:
    fuse_emulation_pause();
    menu_file_open( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F4:
    fuse_emulation_pause();
    menu_options_general( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F5:
    fuse_emulation_pause();
    menu_machine_reset( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F6:
    fuse_emulation_pause();
    menu_media_tape_write( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F7:
    fuse_emulation_pause();
    menu_media_tape_open( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F8:
    menu_media_tape_play( 0 );
    break;
  case INPUT_KEY_F9:
    fuse_emulation_pause();
    menu_machine_select( 0 );
    fuse_emulation_unpause();
    break;
  case INPUT_KEY_F10:
    fuse_emulation_pause();
    menu_file_exit( 0 );
    fuse_emulation_unpause();
    break;

  default: break;		/* Remove gcc warning */

  }
}

void
ui_widget_keyhandler( int native_key )
{
  widget_keyhandler( native_key );
}
