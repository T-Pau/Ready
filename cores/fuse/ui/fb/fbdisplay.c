/* fbdisplay.c: Routines for dealing with the linux fbdev display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Darren Salt,
			   Witold Filipczyk
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

#include <config.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "fbdisplay.h"
#include "fuse.h"
#include "display.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "settings.h"

/* A copy of every pixel on the screen */
libspectrum_word
  fbdisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t fbdisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* The environment variable specifying which device to use */
static const char * const DEVICE_VARIABLE = "FRAMEBUFFER";

/* The device we will use if device_env_variable is not specified */
static const char * const DEFAULT_DEVICE = "/dev/fb0";

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* Are we in a Timex display mode? */
static int hires;

static void register_scalers( void );

/* probably 0rrrrrgggggbbbbb */
static short rgbs[16], greys[16];

static int fb_fd = -1;		/* The framebuffer's file descriptor */
static libspectrum_word *gm = 0;

static struct fb_fix_screeninfo fixed;
static struct fb_var_screeninfo orig_display, display;
static int got_orig_display = 0;
static int changed_palette = 0;

unsigned long fb_resolution; /* == xres << 16 | yres */
#define FB_RES(X,Y) ((X) << 16 | (Y))
#define FB_WIDTH (fb_resolution >> 16)
#define IF_FB_WIDTH(X) ((fb_resolution >> 16) == (X))
#define IF_FB_HEIGHT(Y) ((fb_resolution & 0xFFFF) == (Y))
#define IF_FB_RES(X, Y) (fb_resolution == FB_RES ((X), (Y)))

typedef struct {
  int xres, yres, pixclock;
  int left_margin, right_margin, upper_margin, lower_margin;
  int hsync_len, vsync_len;
  int sync, doublescan;
} fuse_fb_mode_t;

/* Test monitor is a Compaq V50, default settings
 * A = working on ATI RV100 (Radeon 7000) (2.6.x, old radeonfb)
 * M = working on Matrox MGA2064W (Millennium I) (2.6.x, matroxfb)
 * Large/tall/wide = size indication (overscan)
 * The 640x480 modes are from /etc/fb.modes and should work anywhere.
 *
 *    x    y  clock   lm  rm  tm  bm   hl  vl  s  d
 */
static const fuse_fb_mode_t fb_modes_singlescan[] = {
  { 640, 480, 32052,  96, 56, 28,  9,  40,  3, 0, 0 }, /* 640x480-72  72.114 */
  { 640, 480, 39722,  48, 16, 33, 10,  96,  2, 0, 0 }, /* 640x480-60  59.940 std */
  { 0 } /* end of list */
};
static const fuse_fb_mode_t fb_modes_doublescan[] = {
  { 640, 240, 32052,  92, 56, 14,  4,  40,  3, 0, 1 }, /* 640x240-72  72.185 */
  { 640, 480, 32052,  96, 56, 28,  9,  40,  3, 0, 0 }, /* 640x480-72  72.114 */
  { 640, 480, 39722,  48, 16, 33, 10,  96,  2, 0, 0 }, /* 640x480-60  59.940 std */
  { 320, 240, 64104,  46, 28, 14,  4,  20,  3, 3, 1 }, /* 320x240-72  72.185 M wide */
  { 0 } /* end of list */
};
static const fuse_fb_mode_t fb_modes_doublescan_alt[] = {
  { 640, 240, 39722,  36, 12, 18,  7,  96,  2, 1, 1 }, /* 640x240-60  60.133 AM large */
  { 640, 480, 32052,  96, 56, 28,  9,  40,  3, 0, 0 }, /* 640x480-72  72.114 */
  { 640, 480, 39722,  48, 16, 33, 10,  96,  2, 0, 0 }, /* 640x480-60  59.940 std */
  { 320, 240, 79444,  18,  6, 18,  7,  48,  2, 1, 1 }, /* 320x240-60  60.133 AM large */
  { 0 } /* end of list */
};
/* Modes not used but which may work are listed here.
 *    x    y  clock   lm  rm  tm  bm   hl  vl  s  d
  { 640, 480, 22272,  48, 32, 17, 22, 128, 12, 0, 0 }, ** 640x480-100 99.713
  { 640, 480, 25057, 120, 32, 14, 25,  40, 14, 0, 0 }, ** 640x480-90  89.995
  { 640, 480, 31747, 120, 16, 16,  1,  64,  3, 0, 0 }, ** 640x480-75  74.998
  { 320, 240, 80000,  40, 28,  9,  2,  20,  3, 0, 1 }, ** 320x240-60  60.310 M tall
  { 320, 240, 55555,  52, 16, 12,  0,  28,  2, 0, 1 }, ** 320x240-85  85.177 M
 */

static unsigned short red16[256], green16[256], blue16[256], transp16[256];
static struct fb_cmap orig_cmap = {0, 256, red16, green16, blue16, transp16};

static int fb_set_mode( void );

int uidisplay_init( int width, int height )
{
  hires = ( width == 640 ? 1 : 0 );

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  register_scalers();

  display_ui_initialised = 1;

  display_refresh_all();

  return 0;
}

static void
register_scalers( void )
{
  scaler_register_clear();
  scaler_select_bitformat( 565 );		/* 16bit always */
  scaler_register( SCALER_NORMAL );
}

static void
linear_palette(struct fb_cmap *p_cmap)
{
  int i;
  int rcols = 1 << display.red.length;
  int gcols = 1 << display.green.length;
  int bcols = 1 << display.blue.length;

  for (i = 0; i < rcols; i++)
    p_cmap->red[i] = (65535 / (rcols - 1)) * i;

  for (i = 0; i < gcols; i++)
    p_cmap->green[i] = (65535 / (gcols - 1)) * i;

  for (i = 0; i < bcols; i++)
    p_cmap->blue[i] = (65535 / (bcols - 1)) * i;
}

int fbdisplay_init(void)
{
  int i;
  const char *dev;

  static libspectrum_word r16[256], g16[256], b16[256];
  static struct fb_cmap fb_cmap = {
    0, 256, r16, g16, b16, NULL
  };

  dev = getenv( DEVICE_VARIABLE );
  if( !dev || !*dev ) dev = DEFAULT_DEVICE;

  fb_fd = open( dev, O_RDWR | O_EXCL );
  if( fb_fd == -1 ) {
    fprintf( stderr, "%s: couldn't open framebuffer device '%s'\n",
	     fuse_progname, dev );
    return 1;
  }
  if( ioctl( fb_fd, FBIOGET_FSCREENINFO, &fixed )        ||
      ioctl( fb_fd, FBIOGET_VSCREENINFO, &orig_display )    ) {
    fprintf( stderr, "%s: couldn't read info from framebuffer device '%s'\n",
	     fuse_progname, dev );
    return 1;
  }
  got_orig_display = 1;

  if( fb_set_mode() ) return 1;

  fputs( "\x1B[H\x1B[J", stdout );	/* clear tty */
  memset( gm, 0, display.xres_virtual * display.yres_virtual * 2 );


  display.activate = FB_ACTIVATE_NOW;
  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
    fprintf( stderr, "%s: couldn't set mode for framebuffer device '%s'\n",
	     fuse_progname, dev );
    return 1;
  }

  ioctl( fb_fd, FBIOGET_VSCREENINFO, &display);
  for( i = 0; i < 16; i++ ) {
    int v = ( i & 8 ) ? 0xff : 0xbf;
    int c;
     rgbs[i] = ( ( i & 1 ) ? (v >> (8 - display.blue.length)) << display.blue.offset  : 0 )
           | ( ( i & 2 ) ? (v >> (8 - display.red.length)) << display.red.offset   : 0 )
           | ( ( i & 4 ) ? (v >> (8 - display.green.length)) << display.green.offset : 0 );

     c = (( i & 1 ) ? (v * 0.114) : 0.0) +
     (( i & 2) ? (v * 0.299) : 0.0) +
     (( i & 4) ? (v * 0.587) : 0.0) + 0.5;
     greys[i] = (c >> (8 - display.red.length) << display.red.offset)
     | (c >> (8 - display.green.length) << display.green.offset)
     | (c >> (8 - display.blue.length) << display.blue.offset);
  }
  linear_palette(&fb_cmap);

  if (orig_display.bits_per_pixel == 8 || fixed.visual == FB_VISUAL_DIRECTCOLOR) {
    ioctl( fb_fd, FBIOGETCMAP, &orig_cmap);
    changed_palette = 1;
  }
  ioctl( fb_fd, FBIOGET_FSCREENINFO, &fixed);
  if ( fixed.visual == FB_VISUAL_DIRECTCOLOR) {
    ioctl( fb_fd, FBIOPUTCMAP, &fb_cmap );
  }
  sleep( 1 ); /* give the monitor time to sync before we start emulating */

  fputs( "\x1B[?25l", stdout );		/* hide cursor */
  fflush( stdout );

  return 0;
}

static int
fb_select_mode( const fuse_fb_mode_t *fb_mode )
{
  memset (&display, 0, sizeof (struct fb_var_screeninfo));
  display.xres_virtual = display.xres = fb_mode->xres;
  display.yres_virtual = display.yres = fb_mode->yres;
  display.xoffset = display.yoffset = 0;
  display.grayscale = 0;
  display.nonstd = 0;
  display.accel_flags = 0;
  display.pixclock = fb_mode->pixclock;
  display.left_margin = fb_mode->left_margin;
  display.right_margin = fb_mode->right_margin;
  display.upper_margin = fb_mode->upper_margin;
  display.lower_margin = fb_mode->lower_margin;
  display.hsync_len = fb_mode->hsync_len;
  display.vsync_len = fb_mode->vsync_len;
  display.sync = fb_mode->sync;
  display.vmode &= ~FB_VMODE_MASK;
  if( fb_mode->doublescan ) display.vmode |= FB_VMODE_DOUBLE;
  display.vmode |= FB_VMODE_CONUPDATE;

  display.bits_per_pixel = 16;
  display.red.length = display.green.length = display.blue.length = 5;

  display.red.offset = 0;
  display.green.offset = 5;
  display.blue.offset = 10;

  gm = mmap( 0, fixed.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0 );
  if( gm == (void*)-1 ) {
    fprintf (stderr, "%s: couldn't mmap for framebuffer: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  display.activate = FB_ACTIVATE_TEST;
  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
    munmap( gm, fixed.smem_len );
    return 1;
  }

  fb_resolution = FB_RES( display.xres, display.yres );
  return 0;			/* success */
}

static int
fb_set_mode( void )
{
  size_t i;

  const fuse_fb_mode_t *fb_modes =
    (settings_current.doublescan_mode == 0) ? fb_modes_singlescan :
    (settings_current.doublescan_mode == 1) ? fb_modes_doublescan :
					      fb_modes_doublescan_alt;

  /* First, try to use our preferred mode */
  for( i=0; fb_modes[i].xres; i++ )
    if( fb_modes[i].xres == settings_current.fb_mode )
      if( !fb_select_mode( fb_modes + i ) )
	return 0;

  /* If that failed, try to use the first available mode */
  for( i=0; fb_modes[i].xres; i++ )
    if( !fb_select_mode( fb_modes + i ) )
      return 0;

  /* If that failed, we can't continue :-( */
  fprintf( stderr, "%s: couldn't find a framebuffer mode to start in\n",
	   fuse_progname );
  return 1;
}

int
uidisplay_hotswap_gfx_mode( void )
{
  return 0;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int start, int width, int height)
{
  int y;
  const short *colours = settings_current.bw_tv ? greys : rgbs;

  switch( fb_resolution ) {
  case FB_RES( 640, 480 ):
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

	for( i = 0, point = gm + y * display.xres_virtual + x;
	     i < width;
	     i++, point++ )
	  *point = colours[fbdisplay_image[y][x+i]];

      } else {

	for( i = 0, point = gm + 2 * y * display.xres_virtual + x * 2;
	     i < width;
	     i++, point += 2 )
	  *  point       = *( point +     display.xres_virtual ) =
	  *( point + 1 ) = *( point + 1 + display.xres_virtual ) = 
	    colours[fbdisplay_image[y][x+i]];

      }
    }
    break;

  case FB_RES( 640, 240 ):
    if( hires ) { start >>= 1; height >>= 1; }
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

	for ( i = 0, point = gm + y * display.xres_virtual + x;
	      i < width;
	      i++, point++ )
	  *point = colours[fbdisplay_image[y*2][x+i]];

      } else {

	for( i = 0, point = gm + y * display.xres_virtual + x * 2;
	     i < width;
	     i++, point+=2 )
	  *point = *(point+1) = colours[fbdisplay_image[y][x+i]];

      }
    }
    break;

  case FB_RES( 320, 240 ):
    if( hires ) { start >>= 1; height >>= 1; x >>= 1; width >>= 1; }
    for( y = start; y < start + height; y++ )
    {
      int i;
      libspectrum_word *point;

      if( hires ) {

	/* Drop every second pixel */
	for ( i = 0, point = gm + y * display.xres_virtual + x;
	      i < width;
	      i++, point++ )
	  *point = colours[fbdisplay_image[y*2][(x+i)*2]];

      } else {

	for( i = 0, point = gm + y * display.xres_virtual + x;
	     i < width;
	     i++, point++ )
	  *point = colours[fbdisplay_image[y][x+i]];

      }

    }
    break;

  default:;		/* Shut gcc up */
  }
}

int
uidisplay_end( void )
{
  return 0;
}

int
fbdisplay_end( void )
{
  if( fb_fd != -1 ) {
    if( got_orig_display ) {
      ioctl( fb_fd, FBIOPUT_VSCREENINFO, &orig_display );
      if (changed_palette) {
        ioctl( fb_fd, FBIOPUTCMAP, &orig_cmap);
	changed_palette = 0;
      }
    }
    close( fb_fd );
    fb_fd = -1;
    fputs( "\x1B[H\x1B[J\x1B[?25h", stdout );	/* clear screen, show cursor */
  }

  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    fbdisplay_image[y  ][x  ] = colour;
    fbdisplay_image[y  ][x+1] = colour;
    fbdisplay_image[y+1][x  ] = colour;
    fbdisplay_image[y+1][x+1] = colour;
  } else {
    fbdisplay_image[y][x] = colour;
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
      fbdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      fbdisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      fbdisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      fbdisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      fbdisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      fbdisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      fbdisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      fbdisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      fbdisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      fbdisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      fbdisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      fbdisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      fbdisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      fbdisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      fbdisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      fbdisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    fbdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    fbdisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    fbdisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    fbdisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    fbdisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    fbdisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    fbdisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    fbdisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
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
    fbdisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    fbdisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    fbdisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    fbdisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    fbdisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    fbdisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    fbdisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    fbdisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    fbdisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    fbdisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    fbdisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    fbdisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    fbdisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    fbdisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    fbdisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    fbdisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

void
uidisplay_frame_save( void )
{
  /* FIXME: Save current framebuffer state as the widget UI wants to scribble
     in here */
}

void
uidisplay_frame_restore( void )
{
  /* FIXME: Restore saved framebuffer state as the widget UI wants to draw a
     new menu */
}
