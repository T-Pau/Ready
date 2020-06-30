/* picture.c: GTK+ routines to draw the keyboard picture
   Copyright (c) 2002-2005 Philip Kendall

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "ui/ui.h"
#include "utils.h"

/* An RGB image of the keyboard picture */
static guchar picture[ DISPLAY_SCREEN_HEIGHT * DISPLAY_ASPECT_WIDTH * 4 ];
static const gint picture_pitch = DISPLAY_ASPECT_WIDTH * 4;

static int dialog_created = 0;

static void draw_screen( libspectrum_byte *screen, int border );

#if !GTK_CHECK_VERSION( 3, 0, 0 )

static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data );

#else

static gboolean
picture_draw( GtkWidget *widget, cairo_t *cr, gpointer user_data );

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

static GtkWidget *dialog;

int
gtkui_picture( const char *filename, int border )
{
  utils_file screen;

  GtkWidget *drawing_area, *content_area;

  if( !dialog_created ) {

    if( utils_read_screen( filename, &screen ) ) {
      return 1;
    }

    draw_screen( screen.buffer, border );

    utils_close_file( &screen );

    dialog = gtkstock_dialog_new( "Fuse - Keyboard",
				  G_CALLBACK( gtk_widget_hide ) );
  
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request( drawing_area, DISPLAY_ASPECT_WIDTH,
                                 DISPLAY_SCREEN_HEIGHT );

#if !GTK_CHECK_VERSION( 3, 0, 0 )
    g_signal_connect( G_OBJECT( drawing_area ),
		      "expose_event", G_CALLBACK( picture_expose ),
		      NULL );
#else
    g_signal_connect( G_OBJECT( drawing_area ),
		      "draw", G_CALLBACK( picture_draw ),
		      NULL );
#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

    content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
    gtk_container_add( GTK_CONTAINER( content_area ), drawing_area );

    gtkstock_create_close( dialog, NULL, G_CALLBACK( gtk_widget_hide ),
			   FALSE );

    /* Stop users resizing this window */
    gtk_window_set_resizable( GTK_WINDOW( dialog ), FALSE );

    dialog_created = 1;
  }

  gtk_widget_show_all( dialog );

  return 0;
}

static void
draw_screen( libspectrum_byte *screen, int border )
{
  int i, x, y, ink, paper;
  libspectrum_byte attr, data; 

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)( picture + y * picture_pitch + 4 * x ) =
	gtkdisplay_colours[border];
      *(libspectrum_dword*)(
          picture +
	  ( y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) * picture_pitch +
	  4 * x
	) = gtkdisplay_colours[ border ];
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)
	(picture + ( y + DISPLAY_BORDER_HEIGHT) * picture_pitch + 4 * x) =
	gtkdisplay_colours[ border ];
      *(libspectrum_dword*)(
          picture +
	  ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
	  4 * ( x+DISPLAY_ASPECT_WIDTH-DISPLAY_BORDER_ASPECT_WIDTH )
	) = gtkdisplay_colours[ border ];
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      attr = screen[ display_attr_start[y] + x ];

      ink = ( attr & 0x07 ) + ( ( attr & 0x40 ) >> 3 );
      paper = ( attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      for( i=0; i<8; i++ ) {
	*(libspectrum_dword*)(
	    picture +
	    ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
	    4 * ( 8 * x + DISPLAY_BORDER_ASPECT_WIDTH + i )
	  ) = ( data & 0x80 ) ? gtkdisplay_colours[ ink ]
	                      : gtkdisplay_colours[ paper ];
	data <<= 1;
      }
    }

  }
}

#if !GTK_CHECK_VERSION( 3, 0, 0 )

static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data GCC_UNUSED )
{
  int x = event->expose.area.x, y = event->expose.area.y;

  gdk_draw_rgb_32_image( widget->window,
			 widget->style->fg_gc[ GTK_STATE_NORMAL ],
			 x, y,
			 event->expose.area.width, event->expose.area.height,
			 GDK_RGB_DITHER_NONE,
			 picture + y * picture_pitch + 4 * x, picture_pitch );

  return TRUE;
}

#else                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

static gboolean
picture_draw( GtkWidget *widget, cairo_t *cr, gpointer user_data )
{
  cairo_surface_t *surface;

  surface = cairo_image_surface_create_for_data( picture,
                                                 CAIRO_FORMAT_RGB24,
                                                 DISPLAY_ASPECT_WIDTH,
                                                 DISPLAY_SCREEN_HEIGHT,
                                                 picture_pitch );

  cairo_set_source_surface( cr, surface, 0, 0 );
  cairo_set_operator( cr, CAIRO_OPERATOR_SOURCE );
  cairo_paint( cr );

  cairo_surface_destroy( surface );

  return FALSE;
}

#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */
