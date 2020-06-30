/* gtkdisplay.c: GTK+ routines for dealing with the Speccy screen
   Copyright (c) 2000-2005 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "display.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "settings.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* A copy of every pixel on the screen, replaceable by plotting directly into
   rgb_image below */
libspectrum_word
  gtkdisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t gtkdisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* An RGB image of the Spectrum screen; slightly bigger than the real
   screen to handle the smoothing filters which read around each pixel */
static guchar rgb_image[ 4 * 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) *
                                 ( DISPLAY_SCREEN_WIDTH  + 3 )   ];
static const gint rgb_pitch = ( DISPLAY_SCREEN_WIDTH + 3 ) * 4;

/* The scaled image */
static guchar scaled_image[ 3 * DISPLAY_SCREEN_HEIGHT *
                            6 * DISPLAY_SCREEN_WIDTH ];
static const ptrdiff_t scaled_pitch = 6 * DISPLAY_SCREEN_WIDTH;

/* The colour palette */
static const guchar rgb_colours[16][3] = {

  {   0,   0,   0 },
  {   0,   0, 192 },
  { 192,   0,   0 },
  { 192,   0, 192 },
  {   0, 192,   0 },
  {   0, 192, 192 },
  { 192, 192,   0 },
  { 192, 192, 192 },
  {   0,   0,   0 },
  {   0,   0, 255 },
  { 255,   0,   0 },
  { 255,   0, 255 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  { 255, 255,   0 },
  { 255, 255, 255 },

};

/* And the colours (and black and white 'colours') in 32-bit format */
libspectrum_dword gtkdisplay_colours[16];
static libspectrum_dword bw_colours[16];

/* Colour format for the back buffer in endianess-order */
typedef enum {
  FORMAT_x8r8g8b8,    /* Cairo  (GTK3) */
  FORMAT_x8b8g8r8     /* GdkRGB (GTK2) */
} colour_format_t;


#if GTK_CHECK_VERSION( 3, 0, 0 )

static int display_updated = 0;

static cairo_surface_t *surface = NULL;

#endif                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int gtkdisplay_current_size=1;

/* Extra height used for menu and status bars */
static int extra_height = 0;

static int init_colours( colour_format_t format );
static void gtkdisplay_area(int x, int y, int width, int height);
static void register_scalers( int force_scaler );
static void gtkdisplay_load_gfx_mode( void );

/* Callbacks */

#if !GTK_CHECK_VERSION( 3, 0, 0 )
static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
                              gpointer data);
#else
static gboolean gtkdisplay_draw( GtkWidget *widget, cairo_t *cr,
                                 gpointer user_data );
#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

static gint drawing_area_resize_callback( GtkWidget *widget, GdkEvent *event,
                                          gpointer data );

static int
init_colours( colour_format_t format )
{
  size_t i;

  for( i = 0; i < 16; i++ ) {


    guchar red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

#ifdef WORDS_BIGENDIAN

    switch( format ) {
    case FORMAT_x8b8g8r8:
      gtkdisplay_colours[i] =  red << 24 | green << 16 | blue << 8;
      break;
    case FORMAT_x8r8g8b8:
      gtkdisplay_colours[i] = blue << 24 | green << 16 |  red << 8;
      break;
    }

              bw_colours[i] = grey << 24 |  grey << 16 | grey << 8;

#else                           /* #ifdef WORDS_BIGENDIAN */

    switch( format ) {
    case FORMAT_x8b8g8r8:
      gtkdisplay_colours[i] =  red | green << 8 | blue << 16;
      break;
    case FORMAT_x8r8g8b8:
      gtkdisplay_colours[i] = blue | green << 8 |  red << 16;
      break;
    }

              bw_colours[i] = grey |  grey << 8 | grey << 16;

#endif                          /* #ifdef WORDS_BIGENDIAN */

  }

  return 0;
}

int
uidisplay_init( int width, int height )
{
  int x, y, error;
  libspectrum_dword black;
  const char *machine_name;
  colour_format_t colour_format;

#if !GTK_CHECK_VERSION( 3, 0, 0 )

  g_signal_connect( G_OBJECT( gtkui_drawing_area ), "expose_event",
                    G_CALLBACK( gtkdisplay_expose ), NULL );

  colour_format = FORMAT_x8b8g8r8;

  g_signal_connect( G_OBJECT( gtkui_drawing_area ), "configure_event",
                    G_CALLBACK( drawing_area_resize_callback ), NULL );

#else

  g_signal_connect( G_OBJECT( gtkui_drawing_area ), "draw",
                    G_CALLBACK( gtkdisplay_draw ), NULL );

  colour_format = FORMAT_x8r8g8b8;

  g_signal_connect( G_OBJECT( gtkui_window ), "configure_event",
                    G_CALLBACK( drawing_area_resize_callback ), NULL );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

  error = init_colours( colour_format ); if( error ) return error;

  black = settings_current.bw_tv ? bw_colours[0] : gtkdisplay_colours[0];

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT + 4; y++ )
    for( x = 0; x < DISPLAY_SCREEN_WIDTH + 3; x++ )
      *(libspectrum_dword*)( rgb_image + y * rgb_pitch + 4 * x ) = black;

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  register_scalers( 0 );

  display_refresh_all();

  if ( scaler_select_scaler( current_scaler ) )
        scaler_select_scaler( SCALER_NORMAL );

  gtkdisplay_load_gfx_mode();

  machine_name = libspectrum_machine_name( machine_current->machine );
  gtkstatusbar_update_machine( machine_name );

  display_ui_initialised = 1;

  return 0;
}

/* Ensure that an appropriate Cairo surface exists */
static void
ensure_appropriate_surface( void )
{
#if GTK_CHECK_VERSION( 3, 0, 0 )

  /* Create a bigger surface for the new display size */
  float scale = (float)gtkdisplay_current_size / image_scale;
  if( surface ) cairo_surface_destroy( surface );

  surface =
      cairo_image_surface_create_for_data( scaled_image,
                                           CAIRO_FORMAT_RGB24,
                                           scale * image_width,
                                           scale * image_height,
                                           scaled_pitch );

#endif                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */
}

static int
drawing_area_resize( int width, int height, int force_scaler )
{
  int size;

  size = width / DISPLAY_ASPECT_WIDTH;
  if( size > height / DISPLAY_SCREEN_HEIGHT )
    size = height / DISPLAY_SCREEN_HEIGHT;

  /* If we're the same size as before, no need to do anything else */
  if( size == gtkdisplay_current_size ) return 0;

  gtkdisplay_current_size = size;

  register_scalers( force_scaler );

  memset( scaled_image, 0, sizeof( scaled_image ) );

  ensure_appropriate_surface();

  display_refresh_all();

  return 0;
}

static void
register_scalers( int force_scaler )
{
  scaler_type scaler;
  float drawing_area_scale, scaling_factor;

  scaler_register_clear();

  if( machine_current->timex ) {
    scaler_register( SCALER_HALF );
    scaler_register( SCALER_HALFSKIP );
    scaler_register( SCALER_TIMEXTV );
    scaler_register( SCALER_TIMEX1_5X );
  } else {
    scaler_register( SCALER_DOUBLESIZE );
    scaler_register( SCALER_TRIPLESIZE );
    scaler_register( SCALER_TV2X );
    scaler_register( SCALER_TV3X );
    scaler_register( SCALER_PALTV2X );
    scaler_register( SCALER_PALTV3X );
    scaler_register( SCALER_HQ2X );
    scaler_register( SCALER_HQ3X );
    scaler_register( SCALER_ADVMAME2X );
    scaler_register( SCALER_ADVMAME3X );
    scaler_register( SCALER_2XSAI );
    scaler_register( SCALER_SUPER2XSAI );
    scaler_register( SCALER_SUPEREAGLE );
    scaler_register( SCALER_DOTMATRIX );
  }
  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_PALTV );

  scaler =
    scaler_is_supported( current_scaler ) ? current_scaler : SCALER_NORMAL;

  drawing_area_scale = (float)gtkdisplay_current_size / image_scale;
  scaling_factor = scaler_get_scaling_factor( current_scaler );

  /* Override scaler if the image doesn't fit well in the drawing area */
  if( force_scaler && drawing_area_scale != scaling_factor ) {

    switch( gtkdisplay_current_size ) {
    case 1: scaler = machine_current->timex ? SCALER_HALF : SCALER_NORMAL;
      break;
    case 2: scaler = machine_current->timex ? SCALER_NORMAL : SCALER_DOUBLESIZE;
      break;
    case 3: scaler = machine_current->timex ? SCALER_TIMEX1_5X :
                                              SCALER_TRIPLESIZE;
      break;
    }
  }

  scaler_select_scaler( scaler );
}

void
uidisplay_frame_end( void )
{
#if GTK_CHECK_VERSION( 3, 0, 0 )
  if( display_updated ) {
    gdk_window_process_updates( gtk_widget_get_window( gtkui_drawing_area ),
                                FALSE );
    display_updated = 0;
  }
#endif                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */

  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  float scale = (float)gtkdisplay_current_size / image_scale;
  int scaled_x, scaled_y, i, yy;
  libspectrum_dword *palette;

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  scaled_x = scale * x; scaled_y = scale * y;

  palette = settings_current.bw_tv ? bw_colours : gtkdisplay_colours;

  /* Create the RGB image */
  for( yy = y; yy < y + h; yy++ ) {

    libspectrum_dword *rgb; libspectrum_word *display;

    rgb = (libspectrum_dword*)( rgb_image + ( yy + 2 ) * rgb_pitch );
    rgb += x + 1;

    display = &gtkdisplay_image[yy][x];

    for( i = 0; i < w; i++, rgb++, display++ ) *rgb = palette[ *display ];
  }

  /* Create scaled image */
  scaler_proc32( &rgb_image[ ( y + 2 ) * rgb_pitch + 4 * ( x + 1 ) ],
                 rgb_pitch,
                 &scaled_image[ scaled_y * scaled_pitch + 4 * scaled_x ],
                 scaled_pitch, w, h );

  w *= scale; h *= scale;

  /* Blit to the real screen */
  gtkdisplay_area( scaled_x, scaled_y, w, h );
}

static void gtkdisplay_area(int x, int y, int width, int height)
{
#if !GTK_CHECK_VERSION( 3, 0, 0 )

  gdk_draw_rgb_32_image( gtkui_drawing_area->window,
                         gtkui_drawing_area->style->fg_gc[GTK_STATE_NORMAL],
                         x, y, width, height, GDK_RGB_DITHER_NONE,
                         &scaled_image[ y * scaled_pitch + 4 * x ],
                         scaled_pitch );

#else
  display_updated = 1;

  gtk_widget_queue_draw_area( gtkui_drawing_area, x, y, width, height );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */
}

int
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Setup the new GFX mode */
  gtkdisplay_load_gfx_mode();

  fuse_emulation_unpause();

  return 0;
}

int
uidisplay_end( void )
{
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    gtkdisplay_image[y  ][x  ] = colour;
    gtkdisplay_image[y  ][x+1] = colour;
    gtkdisplay_image[y+1][x  ] = colour;
    gtkdisplay_image[y+1][x+1] = colour;
  } else {
    gtkdisplay_image[y][x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      gtkdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      gtkdisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      gtkdisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      gtkdisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      gtkdisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      gtkdisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      gtkdisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      gtkdisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      gtkdisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      gtkdisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      gtkdisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      gtkdisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      gtkdisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      gtkdisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      gtkdisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      gtkdisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    gtkdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    gtkdisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    gtkdisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    gtkdisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    gtkdisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    gtkdisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    gtkdisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    gtkdisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4; y <<= 1;

  for( i=0; i<2; i++,y++ ) {
    gtkdisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    gtkdisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    gtkdisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    gtkdisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    gtkdisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    gtkdisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    gtkdisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    gtkdisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    gtkdisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    gtkdisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    gtkdisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    gtkdisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

/* Callbacks */

#if !GTK_CHECK_VERSION( 3, 0, 0 )

/* Called by gtkui_drawing_area on "expose_event" */
static gint
gtkdisplay_expose( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
                   gpointer data GCC_UNUSED )
{
  gtkdisplay_area(event->expose.area.x, event->expose.area.y,
                  event->expose.area.width, event->expose.area.height);
  return TRUE;
}

/* Called by gtkui_drawing_area on "configure_event".
   On GTK+ 2 the drawing_area determines the size of the window */
static gint
drawing_area_resize_callback( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
                              gpointer data GCC_UNUSED )
{
  drawing_area_resize( event->configure.width, event->configure.height, 1 );

  return TRUE;
}

#else                 /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

/* Called by gtkui_drawing_area on "draw" event */
static gboolean
gtkdisplay_draw( GtkWidget *widget, cairo_t *cr, gpointer user_data )
{
  /* Create a new surface for this gfx mode */
  if( !surface ) ensure_appropriate_surface();

  /* Repaint the drawing area */
  cairo_set_source_surface( cr, surface, 0, 0 );
  cairo_set_operator( cr, CAIRO_OPERATOR_SOURCE );
  cairo_paint( cr );

  return FALSE;
}

/* Called by gtkui_window on "configure_event".
   On GTK+ 3 the window determines the size of the drawing area */
static gint
drawing_area_resize_callback( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
                              gpointer data GCC_UNUSED )
{
  drawing_area_resize( event->configure.width,
                       event->configure.height - extra_height, 1 );

  return FALSE;
}

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

void
gtkdisplay_update_geometry( void )
{
  GdkGeometry geometry;
  GdkWindowHints hints;
  GtkWidget *geometry_widget;
  float scale;

  if( !scalers_registered ) return;

  scale = scaler_get_scaling_factor( current_scaler );

  hints = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE |
          GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC;

#if GTK_CHECK_VERSION( 3, 0, 0 )

  /* Since GTK+ 3.20 it is intended that gtk_window_set_geometry_hints
     don't set geometry of widgets. See [bugs:#344] */
  geometry_widget = NULL;

  /* Add extra space for menu bar */
  extra_height = gtkui_menubar_get_height();

  /* Add extra space for status bar + padding */
  if( settings_current.statusbar ) {
    extra_height += gtkstatusbar_get_height();
  }

#ifdef GDK_WINDOWING_WAYLAND
  /* We don't calculate the window size enough accurately on wayland
     backend to force the window geometry (bug #367) */
  GdkDisplay *display = gdk_display_get_default();

  if( GDK_IS_WAYLAND_DISPLAY( display ) ) {
    hints &= ~GDK_HINT_RESIZE_INC;
  }
#endif                /* #ifdef GDK_WINDOWING_WAYLAND */

#else                 /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */

  geometry_widget = gtkui_drawing_area;

#endif                /* #if GTK_CHECK_VERSION( 3, 0, 0 ) */


  geometry.min_width = DISPLAY_ASPECT_WIDTH;
  geometry.min_height = DISPLAY_SCREEN_HEIGHT + extra_height;
  geometry.max_width = 3 * DISPLAY_ASPECT_WIDTH;
  geometry.max_height = 3 * DISPLAY_SCREEN_HEIGHT + extra_height;
  geometry.base_width = scale * image_width;
  geometry.base_height = scale * image_height + extra_height;
  geometry.width_inc = DISPLAY_ASPECT_WIDTH;
  geometry.height_inc = DISPLAY_SCREEN_HEIGHT;

  if( settings_current.aspect_hint ) {
    hints |= GDK_HINT_ASPECT;

    geometry.min_aspect = geometry.max_aspect =
      ( scale * DISPLAY_ASPECT_WIDTH ) /
      ( scale * DISPLAY_SCREEN_HEIGHT + extra_height );

    if( !settings_current.strict_aspect_hint ) {
      geometry.min_aspect *= 0.9;
      geometry.max_aspect *= 1.125;
    }
  }

  gtk_window_set_geometry_hints( GTK_WINDOW( gtkui_window ),
                                 geometry_widget,
                                 &geometry, hints );
}

static void
gtkdisplay_load_gfx_mode( void )
{
  float scale;

  scale = scaler_get_scaling_factor( current_scaler );

  gtkdisplay_update_geometry();

#if !GTK_CHECK_VERSION( 3, 0, 0 )

  /* This function should be innocuous when the main window is shown */
  gtk_window_set_default_size( GTK_WINDOW( gtkui_window ),
                               scale * image_width,
                               scale * image_height + extra_height );

  drawing_area_resize( scale * image_width, scale * image_height, 0 );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */
 
  gtk_window_resize( GTK_WINDOW( gtkui_window ), scale * image_width,
                     scale * image_height + extra_height );

  /* Redraw the entire screen... */
  display_refresh_all();
}
