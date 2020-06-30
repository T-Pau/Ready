/* xdisplay.c: Routines for dealing with drawing the Speccy's screen via Xlib
   Copyright (c) 2000-2005 Philip Kendall, Darren Salt, Gergely Szász
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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_SIGINFO_H
#include <siginfo.h>		/* Needed for psignal on Solaris */
#endif				/* #ifdef HAVE_SIGINFO_H */

#ifdef HAVE_X11_EXTENSIONS_XSHM_H
#define X_USE_SHM
#endif				/* #ifdef HAVE_X11_EXTENSIONS_XSHM_H */

#ifdef X_USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif				/* #ifdef X_USE_SHM */

#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "peripherals/scld.h"
#include "screenshot.h"
#include "settings.h"
#include "xdisplay.h"
#include "xui.h"
#include "ui/scaler/scaler.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

void xstatusbar_init( int size );

typedef enum {
  MSB_RED = 0,			/* 0RGB */
  LSB_RED,			/* 0BGR */
				/* ARGB/ABGR RGBA/BGRA ????*/
} redmask_t;

static XImage *image = NULL;	/* The image structure to draw the
				   Speccy's screen on */
static GC gc;			/* A graphics context to draw with */

Colormap currentMap;		/* The used colormap */

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT
   scale * 4, so 2 => 0.5, 6 => 1.5, 4 => 1.0 */
static int image_scale = 1;

/* The height and width of a 1x1 image in pixels */
static int image_width, image_height, scaled_image_w, scaled_image_h;

/* An RGB image of the Spectrum screen; slightly bigger than the real
   screen to handle the smoothing filters which read around each pixel 16bpp */
static libspectrum_word
  rgb_image[2 * ( DISPLAY_SCREEN_HEIGHT + 4 )][2 * ( DISPLAY_SCREEN_WIDTH  + 3 )];
static const int rgb_pitch = 2 * ( DISPLAY_SCREEN_WIDTH + 3 );

/* A scaled copy of the image displayed on the Spectrum's screen */
static libspectrum_word
  scaled_image[3 * DISPLAY_SCREEN_HEIGHT][3 * DISPLAY_SCREEN_WIDTH];
static const ptrdiff_t scaled_pitch =
  3 * DISPLAY_SCREEN_WIDTH * 2;

/* A scaled copy of the image displayed on the Spectrum's screen */
static libspectrum_word
  rgb_image_backup[2 * ( DISPLAY_SCREEN_HEIGHT + 4 )][2 * ( DISPLAY_SCREEN_WIDTH  + 3 )];

static unsigned long colours[128];
static int colours_allocated = 0;

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int xdisplay_current_size = 1;
static int xdisplay_depth = -1;
static Visual *xdisplay_visual = NULL;
static int xdisplay_bw = -1;
static redmask_t xdisplay_redpos = MSB_RED;	/* red_mask 0xff000 ...*/
static int rShift = 16, gShift = 8, bShift = 0;

/* This is a rule of thumb for the maximum number of rects that can be updated
   each frame. If more are generated we just update the whole screen */
typedef struct {
  int x; int y; int w; int h;
} X_Rect;
#define MAX_UPDATE_RECT 300
static X_Rect updated_rects[MAX_UPDATE_RECT];
static int num_rects = 0;
static libspectrum_byte xdisplay_force_full_refresh = 1;

#ifdef X_USE_SHM
static XShmSegmentInfo shm_info;
int shm_eventtype;

static int try_shm( void );
static int get_shm_id( const int size );
#endif				/* #ifdef X_USE_SHM */

static int shm_used = 0;

typedef void xdisplay_update_rect_t( int x, int y, int w, int h );

static xdisplay_update_rect_t *xdisplay_update_rect;
static xdisplay_update_rect_t xdisplay_update_rect_noscale;
static xdisplay_update_rect_t xdisplay_update_rect_scale;

static int xdisplay_find_visual( void );
static int xdisplay_allocate_colours4( void );
static int xdisplay_allocate_colours8( void );
static int xdisplay_allocate_gc( Window window, GC *new_gc );

static int xdisplay_allocate_image( void );
static void register_scalers( void );
static void xdisplay_destroy_image( void );
static void xdisplay_catch_signal( int sig );

typedef void xdisplay_putpixel_t( int x, int y, libspectrum_word *color );

static xdisplay_putpixel_t *xdisplay_putpixel;

static xdisplay_putpixel_t xdisplay_putpixel_4;
static xdisplay_putpixel_t xdisplay_putpixel_8;
static xdisplay_putpixel_t xdisplay_putpixel_15;
static xdisplay_putpixel_t xdisplay_putpixel_16;
static xdisplay_putpixel_t xdisplay_putpixel_24;

#include "ui/xlib/xpixmaps.c"
void xstatusbar_overlay( void );

static libspectrum_word pal_colour[16] = {
  0x0000, 0x0017, 0xb800, 0xb817, 0x05e0, 0x05f7, 0xbde0, 0xbdf7,
  0x0000, 0x001f, 0xf800, 0xf81f, 0x07e0, 0x07ff, 0xffe0, 0xffff,
};

static libspectrum_word pal_grey[16] = {
  0x0000, 0x39c7, 0x18a3, 0x526a, 0x738e, 0xad55, 0x8430, 0xbdf7,
  0x0000, 0x4a69, 0x20e4, 0x6b4d, 0x94b2, 0xdf1b, 0xb596, 0xffff,
};

static  int rgb_for_4[] = {
  0x00, 0x00, 0x00,
  0x00, 0x00, 0xBF,
  0xBF, 0x00, 0x00,
  0xBF, 0x00, 0xBF,
  0x00, 0xBF, 0x00,
  0x00, 0xBF, 0xBF,
  0xBF, 0xBF, 0x00,
  0xBF, 0xBF, 0xBF,
  0x00, 0x00, 0x00,
  0x00, 0x00, 0xFF,
  0xFF, 0x00, 0x00,
  0xFF, 0x00, 0xFF,
  0x00, 0xFF, 0x00,
  0x00, 0xFF, 0xFF,
  0xFF, 0xFF, 0x00,
  0xFF, 0xFF, 0xFF
};

int
xdisplay_init( void )
{
  if( xdisplay_find_visual() ) return 1;
  if( xdisplay_depth == 4 && xdisplay_allocate_colours4() ) return 1;
  if( xdisplay_depth == 8 && xdisplay_allocate_colours8() ) return 1;
  if( xdisplay_allocate_gc( xui_mainWindow,&gc ) ) return 1;
  if( xdisplay_allocate_image() ) return 1;
  ui_statusbar_update( UI_STATUSBAR_ITEM_TAPE, UI_STATUSBAR_STATE_INACTIVE );

  return 0;
}

static int
xdisplay_find_visual( void )
{
  XVisualInfo visual_tmpl;
  XVisualInfo *vis;
  int nvis, i;
  int sel_v = -1;
  int sel_v_depth = -1;
  int sel_v_class = -1;

  visual_tmpl.screen = xui_screenNum;
  vis = XGetVisualInfo( display,
                             VisualScreenMask,
                             &visual_tmpl, &nvis );
  if( vis != NULL ) {
    for( i = 0; i < nvis; i++ ) {
            /*
             * Save the visual index and its depth, if this is the first
             * truecolor visual, or a visual that is 'preferred' over the
             * previous 'best' visual.
             */
      if( ( sel_v_depth == -1 && vis[i].depth >= 4 ) || /* depth >= 4  */
    	  ( vis[i].depth > sel_v_depth &&
		vis[i].depth <= 16 ) ||			/* depth up to 16 */
    	  ( vis[i].depth <= 8 && vis[i].depth == sel_v_depth &&
		vis[i].class != sel_v_class && 
		    vis[i].class == PseudoColor ) || /* indexed changable palette */
    	  ( vis[i].depth > 8 && vis[i].depth == sel_v_depth &&
		vis[i].class != sel_v_class && 
		    vis[i].class == TrueColor ) /* decomposed constatant colors */
      ) {
        sel_v = i;
        sel_v_depth = vis[i].depth;
	sel_v_class = vis[i].class;
      }
    }

    if ( sel_v != -1 ) {
      xdisplay_visual = vis[sel_v].visual;
      xdisplay_depth = vis[sel_v].depth;
    }
    XFree(vis);
  }
  return sel_v == -1 ? 1 : 0;
}

static void
xdisplay_putpixel_4( int x, int y, libspectrum_word *colour)
{
  XPutPixel( image, x, y, *colour );
}

static void
xdisplay_putpixel_8( int x, int y, libspectrum_word *colour)
{
  unsigned long c = *colour;

  c = (       c         & 0x1f ) * 3 / 31 +
      ( ( ( ( c >>  5 ) & 0x3f ) * 7 / 63 ) << 2 ) +
        ( ( ( c >> 11 )          * 3 / 31 ) << 5 );
  XPutPixel( image, x, y, colours[ c ] );
}

static void
xdisplay_putpixel_15( int x, int y, libspectrum_word *colour)
{
  unsigned long c = *colour;

  c = ( c & 0x1f ) + ( ( c >>  1 ) & 0xffe0 );
  XPutPixel( image, x, y, c );
}

static void
xdisplay_putpixel_16( int x, int y, libspectrum_word *colour)
{
  XPutPixel( image, x, y, *colour );
}

static void
xdisplay_putpixel_24( int x, int y, libspectrum_word *colour)
{
  unsigned long c = *colour;

  c =   ( ( ( c         & 0x1f ) * 255 / 31 ) << bShift ) +
      ( ( ( ( c >>  5 ) & 0x3f ) * 255 / 63 ) << gShift ) +
        ( ( ( c >> 11 )          * 255 / 31 ) << rShift );
  XPutPixel( image, x, y, c );
}

static int
xdisplay_alloc_colour( Colormap *map, XColor *colour )
{
  for(;;) {
    if( XAllocColor( display, *map, colour ) )
      return 0;

    fprintf(stderr,"%s: XAllocColor failed (%04x %04x %04x)\n", fuse_progname,
				colour->red, colour->green, colour->blue );
    if( *map == DefaultColormap( display, xui_screenNum ) ) {
      fprintf( stderr,"%s: switching to private colour map\n", fuse_progname );
      *map = XCopyColormapAndFree( display, *map );
      XSetWindowColormap( display, xui_mainWindow, *map );
      /* Need to repeat the failed allocation */
    } else {
      return 1;
    }
  }
  return 0;
}

static int
xdisplay_allocate_colours4( void )
{
  XColor c;
  int i;

  currentMap = DefaultColormap( display, xui_screenNum );

  if( colours_allocated ) {	/* free it */
    XFreeColors( display, currentMap, colours, 16, 0x00 );
    colours_allocated = 0;
  }
  if( settings_current.bw_tv ) {
    for( i=0; i<16; i++ ) {	/* grey */
      c.red = c.green = c.blue = 
	    rgb_for_4[i * 3    ] * 19595 / 255 +
	    rgb_for_4[i * 3 + 1] * 38469 / 255 +
	    rgb_for_4[i * 3 + 2] *  7471 / 255;
      if( xdisplay_alloc_colour( &currentMap, &c ) )
    	    return 1;
      colours[i] = pal_grey[i] = c.pixel;
    }
  } else {
    for( i=0; i<16; i++ ) {	/* rgb */
      c.red   = rgb_for_4[i * 3    ] * 65535 / 255;
      c.green = rgb_for_4[i * 3 + 1] * 65535 / 255;
      c.blue  = rgb_for_4[i * 3 + 2] * 65535 / 255;
      if( xdisplay_alloc_colour( &currentMap, &c ) )
        return 1;
      colours[i] = pal_colour[i] = c.pixel;
    }
  }

  colours_allocated = 1;
  return 0;
}

static int
xdisplay_allocate_colours8( void )
{
  XColor c;
  int i, r, g, b;

  currentMap = DefaultColormap( display, xui_screenNum );
  
  if( colours_allocated ) {	/* free it */
    XFreeColors( display, currentMap, colours, 128, 0x00 );
    colours_allocated = 0;
  }
  i = 0;
  for( r=0; r<4; r++ )		/* rgb232 => 128 */
    for( g=0; g<8; g++ )
      for( b=0; b<4; b++ ) {
        if( settings_current.bw_tv ) {
          c.red = c.green = c.blue = r * 19595 / 3 +
				     g * 38469 / 7 +
				     b *  7471 / 3;
	} else {
          c.red = r * 65535 / 3;
	  c.green = g * 65535 / 7;
          c.blue = b * 65535 / 3;
	}
	if( xdisplay_alloc_colour( &currentMap, &c ) )
    	    return 1;
	colours[i++] = c.pixel;
      }
  colours_allocated = 1;

  return 0;
}
  
static int
xdisplay_allocate_gc( Window window, GC *new_gc )
{
  unsigned valuemask=0;
  XGCValues values;

  *new_gc = XCreateGC( display, window, valuemask, &values );

  return 0;
}

static int
xdisplay_allocate_image( void )
{
  struct sigaction handler;

  handler.sa_handler = xdisplay_catch_signal;
  sigemptyset( &handler.sa_mask );
  handler.sa_flags = 0;
  sigaction( SIGINT, &handler, NULL );

#ifdef X_USE_SHM
  shm_used = try_shm();
#endif				/* #ifdef X_USE_SHM */

  /* If SHM isn't available, or we're not using it for some reason,
     just get a normal image */
  if( !shm_used ) {
    image = XCreateImage( display, xdisplay_visual,
		       xdisplay_depth, ZPixmap, 0, NULL,
		       3 * DISPLAY_ASPECT_WIDTH,
		       3 * DISPLAY_SCREEN_HEIGHT + 3 * PIXMAPS_H, 8, 0 );
/*
   we allocate extra space after the screen for status bar icons
   status bar icons total width always smaller than 3xDISPLAY_ASPECT_WIDTH
*/
    if(!image) {
      fprintf(stderr,"%s: couldn't create image\n",fuse_progname);
      return 1;
    }

    if( ( image->data = malloc( image->bytes_per_line *
						 image->height ) ) == NULL ) {
      fprintf(stderr, "%s: out of memory for image data\n", fuse_progname);
      return 1;
    }
  }
  if( image ) {
    switch( image->red_mask ) {
    case 0xff0000:		/* 24bit/32bit 0RGB */
      rShift = 16, gShift = 8, bShift = 0;
      break;
    case 0x0000ff:		/* 24bit/32bit 0BGR */
      rShift = 0, gShift = 8, bShift = 16;
      break;
    case 0xff000000:		/* 24bit/32bit RGB0 */
      rShift = 24, gShift = 16, bShift = 8;
      break;
    case 0x0000ff00:		/* 24bit/32bit BGR0 */
      rShift = 8, gShift = 16, bShift = 24;
      break;
    case 0xf800:		/* 16 RGB */
    case 0x7c00:		/* 15 RGB */
      xdisplay_redpos = MSB_RED;
      break;
    case 0x001f:		/* 16/15 BGR */
      xdisplay_redpos = LSB_RED;
      break;
    }
  }

  return 0;
}

#ifdef X_USE_SHM
static int
try_shm( void )
{
  int id;
  int error;

  if( !XShmQueryExtension( display ) ) return 0;

  shm_eventtype = XShmGetEventBase( display ) + ShmCompletion;
  image = XShmCreateImage( display, xdisplay_visual,
			   xdisplay_depth, ZPixmap,
			   NULL, &shm_info,
			   3 * DISPLAY_ASPECT_WIDTH,
			   3 * DISPLAY_SCREEN_HEIGHT + 3 * PIXMAPS_H);
/*
   we allocate extra space after the screen for status bar icons
   status bar icons total width always smaller than 3xDISPLAY_ASPECT_WIDTH
*/
  if( !image ) return 0;

  /* Get an SHM to work with */
  id = get_shm_id( image->bytes_per_line * image->height );
  if( id == -1 ) return 0;

  /* Attempt to attach to the shared memory */
  shm_info.shmid = id;
  image->data = shm_info.shmaddr = shmat( id, 0, 0 );

  /* If we couldn't attach, remove the chunk and give up */
  if( image->data == (void*)-1 ) {
    shmctl( id, IPC_RMID, NULL );
    image->data = NULL;
    return 0;
  }

  /* This may generate an X error */
  xerror_error = 0; xerror_expecting = 1;
  error = !XShmAttach( display, &shm_info );

  /* Force any X errors to occur before we disable traps */
  XSync( display, False );
  xerror_expecting = 0;

  /* If we caught an error, don't use SHM */
  if( error || xerror_error ) {
    shmctl( id, IPC_RMID, NULL );
    shmdt( image->data ); image->data = NULL;
    return 0;
  }

  /* Now flag the chunk for deletion; this will take effect when
     everything has detached from it */
  shmctl( id, IPC_RMID, NULL );

  return 1;
}  

/* Get an SHM ID; also attempt to reclaim any stale chunks we find */
static int
get_shm_id( const int size )
{
  key_t key = 'F' << 24 | 'u' << 16 | 's' << 8 | 'e';
  struct shmid_ds shm;

  int id;

  int pollution = 5;
  
  do {
    /* See if a chunk already exists with this key */
    id = shmget( key, size, 0777 );

    /* If the chunk didn't already exist, try and create one for our
       use */
    if( id == -1 ) {
      id = shmget( key, size, IPC_CREAT | 0777 );
      continue;			/* And then jump to the end of the loop */
    }

    /* If the chunk already exists, try and get information about it */
    if( shmctl( id, IPC_STAT, &shm ) != -1 ) {

      /* If something's actively using this chunk, try another key */
      if( shm.shm_nattch ) {
	key++;
      } else {		/* Otherwise, attempt to remove the chunk */
	
	/* If we couldn't remove that chunk, try another key. If we
	   could, just try again */
	if( shmctl( id, IPC_RMID, NULL ) != 0 ) key++;
      }
    } else {		/* Couldn't get info on the chunk, so try next key */
      key++;
    }
    
    id = -1;		/* To prevent early exit from loop */

  } while( id == -1 && --pollution );

  return id;
}
#endif			/* #ifdef X_USE_SHM */

int
uidisplay_init( int width, int height )
{
  image_width  = width;
  image_height = height;
  if( !scaler_is_supported( current_scaler ) ) {
    if( machine_current->timex )
      scaler_select_scaler( SCALER_HALFSKIP );
    else
      scaler_select_scaler( SCALER_NORMAL );
  }

  register_scalers();
  display_ui_initialised = 1;

  display_refresh_all();
  return 0;
}

static void
resize_window( int w, int h )
{
    if( xdisplay_current_size != w / DISPLAY_ASPECT_WIDTH ) {
      XResizeWindow( display, xui_mainWindow, w, h );
      xdisplay_current_size = w / DISPLAY_ASPECT_WIDTH;
    }
}

static void
register_scalers( void )
{
  int f = -1;

  scaler_register_clear();
  scaler_select_bitformat( 565 );		/* 16bit always */

    if( xdisplay_depth == 4 ) {
      scaler_register( SCALER_NORMAL );
      if( machine_current->timex ) {
        scaler_register( SCALER_HALFSKIP );
        scaler_register( SCALER_TIMEX1_5X );
      } else {
        scaler_register( SCALER_DOUBLESIZE );
        scaler_register( SCALER_ADVMAME2X );

        scaler_register( SCALER_TRIPLESIZE );
        scaler_register( SCALER_ADVMAME3X );
      }
    } else {
      scaler_register( SCALER_NORMAL );
      scaler_register( SCALER_PALTV );
      if( machine_current->timex ) {
        scaler_register( SCALER_HALF ); 
        scaler_register( SCALER_HALFSKIP );
        scaler_register( SCALER_TIMEXTV );
        scaler_register( SCALER_TIMEX1_5X );
      } else {
        scaler_register( SCALER_DOUBLESIZE );
        scaler_register( SCALER_2XSAI );
        scaler_register( SCALER_SUPER2XSAI );
        scaler_register( SCALER_SUPEREAGLE );
        scaler_register( SCALER_ADVMAME2X );
        scaler_register( SCALER_TV2X );
        scaler_register( SCALER_DOTMATRIX );
        scaler_register( SCALER_PALTV2X );
        scaler_register( SCALER_HQ2X );

        scaler_register( SCALER_TRIPLESIZE );
        scaler_register( SCALER_ADVMAME3X );
        scaler_register( SCALER_TV3X );
        scaler_register( SCALER_PALTV3X );
        scaler_register( SCALER_HQ3X );
      }
    }
  if( current_scaler != SCALER_NUM )
    f = 4.0 * scaler_get_scaling_factor( current_scaler ) * 
	    ( machine_current->timex ? 2 : 1 );
  if( scaler_is_supported( current_scaler ) &&
	( xdisplay_current_size * 4 == f ) ) {
    uidisplay_hotswap_gfx_mode();
  } else {
    switch( xdisplay_current_size ) {
    case 1:
      scaler_select_scaler( machine_current->timex ? SCALER_HALF : SCALER_NORMAL );
      break;
    case 2:
      scaler_select_scaler( machine_current->timex ? SCALER_NORMAL : SCALER_DOUBLESIZE );
      break;
    case 3:
      scaler_select_scaler( machine_current->timex ? SCALER_TIMEX1_5X : SCALER_TRIPLESIZE );
      break;
    }
  }
}

int
xdisplay_configure_notify( int width, int height )
{
  int size;

  /* If we're the same size as before, nothing special needed */
  size = width / DISPLAY_ASPECT_WIDTH;
  if( size != height / DISPLAY_SCREEN_HEIGHT ) {	/* out of aspect */
    if( size > height / DISPLAY_SCREEN_HEIGHT ) {
      size = height / DISPLAY_SCREEN_HEIGHT;
      width = size * DISPLAY_ASPECT_WIDTH;
    } else {
      height = size * DISPLAY_SCREEN_HEIGHT;
    }
    xdisplay_current_size = 0;				/* force resize */
    resize_window( width, height );
  } else if( size == xdisplay_current_size ) {
    return 0;
  }

  /* Else set ourselves to the new height */
  xdisplay_current_size = size;

  /* Get a new scaler */
  register_scalers();

  return 0;
}

static void
xdisplay_update_rect_noscale( int x, int y, int w, int h )
{
 int yy, xx;

  /* Call putpixel multiple times */
  for( yy = y; yy < y + h; yy++ )
    for( xx = x; xx < x + w; xx++ )
      xdisplay_putpixel( xx, yy, &rgb_image[yy + 2][xx + 1] );
  /* Blit to the real screen at the frame end end */
  xdisplay_area( x, y, w, h );
}

static void
xdisplay_update_rect_scale( int x, int y, int w, int h )
{
  int yy = y, xx = x;

  y = y * image_scale >> 2;
  x = x * image_scale >> 2;
  scaler_proc16(
        (libspectrum_byte *)&(rgb_image[yy + 2][xx + 1]),
        rgb_pitch * sizeof(rgb_image[0][0]),
        (libspectrum_byte *)&(scaled_image[y][x]),
        scaled_pitch,
        w, h
      );

  w = w * image_scale >> 2;
  h = h * image_scale >> 2;

  /* Call putpixel multiple times */
  for( yy = y; yy < y + h; yy++ )
    for( xx = x; xx < x + w; xx++ )
      xdisplay_putpixel( xx, yy, &scaled_image[yy][xx] );
  /* Blit to the real screen */
  xdisplay_area( x, y, w, h );
}

void
uidisplay_frame_end( void ) 
{
  X_Rect *r, *last_rect;

  /* Force a full redraw if requested */
  if ( xdisplay_force_full_refresh ) {
    num_rects = 1;

    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

  if ( !( ui_widget_level >= 0 ) && num_rects == 0 && !status_updated ) return;

  last_rect = updated_rects + num_rects;

  for( r = updated_rects; r != last_rect; r++ )
    xdisplay_update_rect( r->x, r->y, r->w, r->h );
  if ( settings_current.statusbar )
    xstatusbar_overlay();
  num_rects = 0;
  xdisplay_force_full_refresh = 0;
}

void
uidisplay_frame_save( void )
{
  memcpy( rgb_image_backup, rgb_image, sizeof( rgb_image ) );
}

void
uidisplay_frame_restore( void )
{
  memcpy( rgb_image, rgb_image_backup, sizeof( rgb_image ) );
  xdisplay_update_rect( 0, 0, image_width, image_height );
}

void
uidisplay_area( int x, int y, int w, int h )
{
  if ( xdisplay_force_full_refresh )
    return;

  if( num_rects == MAX_UPDATE_RECT ) {
    xdisplay_force_full_refresh = 1;
    return;
  }

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = w;
  updated_rects[num_rects].h = h;
  num_rects++;
}

void
xdisplay_area( int x, int y, int w, int h )
{
/* e.g. dwm first expose with too big w and h */
  if( x + w > 3 * DISPLAY_ASPECT_WIDTH )
    w = 3 * DISPLAY_ASPECT_WIDTH - x;

  if( y + h > 3 * DISPLAY_SCREEN_HEIGHT )
    h = 3 * DISPLAY_SCREEN_HEIGHT - y;

  if( shm_used ) {
#ifdef X_USE_SHM
    XShmPutImage( display, xui_mainWindow, gc, image, x, y, x, y, w, h, True );
    /* FIXME: should wait for an ShmCompletion event here */
#endif				/* #ifdef X_USE_SHM */
  } else {
    XPutImage( display, xui_mainWindow, gc, image, x, y, x, y, w, h );
  }
}

static void
xdisplay_destroy_image(void)
{
  /* Free the XImage used to store screen data; also frees the malloc'd
     data */
#ifdef X_USE_SHM
  if( shm_used ) {
    XShmDetach( display, &shm_info );
    shmdt( shm_info.shmaddr );
    image->data = NULL;
    shm_used = 0;
  }
#endif
  if( image ) XDestroyImage( image ); image = NULL;
}

static void
xdisplay_setup_rgb_putpixel( void )
{
  switch( xdisplay_depth ) {
  case 4:
    xdisplay_putpixel = xdisplay_putpixel_4;
    break;
  case 8:
    xdisplay_putpixel = xdisplay_putpixel_8;
    break;
  case 15:
    xdisplay_putpixel = xdisplay_putpixel_15;
    break;
  case 16:
    xdisplay_putpixel = xdisplay_putpixel_16;
    break;
  case 24:
  case 32:
    xdisplay_putpixel = xdisplay_putpixel_24;
    break;
  }
  resize_window( scaled_image_w, scaled_image_h );
}

int
uidisplay_hotswap_gfx_mode( void )
{
  image_scale = 4.0 * scaler_get_scaling_factor( current_scaler );
  scaled_image_w = image_width  * image_scale >> 2;
  scaled_image_h = image_height * image_scale >> 2;
  if( current_scaler == SCALER_NORMAL )
    xdisplay_update_rect = xdisplay_update_rect_noscale;
  else
    xdisplay_update_rect = xdisplay_update_rect_scale;

  xdisplay_force_full_refresh = 1;

  if( settings_current.bw_tv != xdisplay_bw ) {
    xdisplay_bw = settings_current.bw_tv;
    if( xdisplay_depth == 4 )
      xdisplay_allocate_colours4();
    else if( xdisplay_depth == 8 )
      xdisplay_allocate_colours8();
  }
  xdisplay_setup_rgb_putpixel();
  xstatusbar_init( xdisplay_current_size );
  display_refresh_all();
  return 0;
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  return 0;
}

static void
xdisplay_catch_signal( int sig )
{
  xdisplay_end();
  psignal( sig, fuse_progname );
  exit( 1 );
}

int
xdisplay_end( void )
{
  xdisplay_destroy_image();
  /* Free the allocated GC */
  if( gc ) {
    XFreeGC( display, gc );
    gc = 0;
  }

  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  libspectrum_word pc = settings_current.bw_tv ? pal_grey[ colour ] :
                        	pal_colour[ colour ];

  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    rgb_image[y + 2][x + 1] = pc;
    rgb_image[y + 2][x + 2] = pc;
    rgb_image[y + 3][x + 1] = pc;
    rgb_image[y + 3][x + 2] = pc;
  } else {
    rgb_image[y + 2][x + 1] = pc;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
	         libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest;
  libspectrum_word pi = settings_current.bw_tv ? pal_grey[ ink ] :
                        	pal_colour[ ink ];
  libspectrum_word pp = settings_current.bw_tv ? pal_grey[ paper ] :
                        	pal_colour[ paper ];

  if( machine_current->timex ) {

    x <<= 4; y <<= 1;

    dest = &(rgb_image[y + 2][x + 1]);

    *(dest + rgb_pitch) = *dest = ( data & 0x80 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x80 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x40 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x40 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x20 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x20 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x10 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x10 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x08 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x08 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x04 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x04 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x02 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x02 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x01 ) ? pi : pp; dest++;
    *(dest + rgb_pitch) = *dest = ( data & 0x01 ) ? pi : pp;
  } else {
    x <<= 3;

    dest = &(rgb_image[y + 2][x + 1]);

    *(dest++) = ( data & 0x80 ) ? pi : pp;
    *(dest++) = ( data & 0x40 ) ? pi : pp;
    *(dest++) = ( data & 0x20 ) ? pi : pp;
    *(dest++) = ( data & 0x10 ) ? pi : pp;
    *(dest++) = ( data & 0x08 ) ? pi : pp;
    *(dest++) = ( data & 0x04 ) ? pi : pp;
    *(dest++) = ( data & 0x02 ) ? pi : pp;
    *dest     = ( data & 0x01 ) ? pi : pp;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest;
  libspectrum_word pi = settings_current.bw_tv ? pal_grey[ ink ] :
                        	pal_colour[ ink ];
  libspectrum_word pp = settings_current.bw_tv ? pal_grey[ paper ] :
                        	pal_colour[ paper ];
  x <<= 4; y <<= 1;

  dest = &(rgb_image[y + 2][x + 1]);
  *(dest + rgb_pitch) = *dest = ( data & 0x8000 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x4000 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x2000 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x1000 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0800 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0400 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0200 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0100 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0080 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0040 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0020 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0010 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0008 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0004 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0002 ) ? pi : pp; dest++;
  *(dest + rgb_pitch) = *dest = ( data & 0x0001 ) ? pi : pp;
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  switch( item ) {

  case UI_STATUSBAR_ITEM_DISK:
    pixmap_disk_state = state;
    status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    /* We don't support pausing this version of Fuse */
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    pixmap_tape_state = state;
    status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    pixmap_mdr_state = state;
    status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    /* We don't support showing a grab icon */
    return 0;

  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
            item );
  return 1;
}

int
ui_statusbar_update_speed( float speed )
{
  char *list[2];
  char buffer[16];
  XTextProperty text;

  list[0] = buffer;
  list[1] = 0;
  snprintf( buffer, 16, "Fuse - %4.0f%%", speed );

  XStringListToTextProperty( list, 1, &text);
  XSetWMName( display, xui_mainWindow, &text );
  XFree( text.value );

  return 0;
}
