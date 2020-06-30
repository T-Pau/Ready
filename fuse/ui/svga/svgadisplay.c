/* svgadisplay.c: Routines for dealing with the svgalib display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Witold Filipczyk,
			   Russell Marks
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgakeyboard.h>

#include "fuse.h"
#include "display.h"
#include "screenshot.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT
   scale * 4, so 2 => 0.5, 6 => 1.5, 4 => 1.0 */
static int image_scale = 1;

/* The height and width of a 1x1 image in pixels */
static int image_width, image_height, scaled_image_w, scaled_image_h;

/* A copy of every pixel on the screen */
static libspectrum_word
  rgb_image[2 * ( DISPLAY_SCREEN_HEIGHT + 4 )][2 * ( DISPLAY_SCREEN_WIDTH  + 3 )];
static const int rgb_pitch = 2 * ( DISPLAY_SCREEN_WIDTH + 3 );

/* A scaled copy of the image displayed on the Spectrum's screen */
static libspectrum_word
  scaled_image[3*DISPLAY_SCREEN_HEIGHT][3*DISPLAY_SCREEN_WIDTH];
static const ptrdiff_t scaled_pitch =
  3 * DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* The line buffer for svga_putpixel ...*/
static libspectrum_byte
  line_buff[3*4*DISPLAY_SCREEN_WIDTH];
static libspectrum_byte *line_buff_ptr;
static int bytesperpixel = 0;
static int xoffs, yoffs, xclip, yclip;

static int svgadisplay_current_size = 0;
static int svgadisplay_depth = -1;
static int svgadisplay_bw = -1;
static int rShift = 16, gShift = 8, bShift = 0;

/* This is a rule of thumb for the maximum number of rects that can be updated
   each frame. If more are generated we just update the whole screen */
typedef struct {
  int x; int y; int w; int h;
} SVGA_Rect;
#define MAX_UPDATE_RECT 300
static SVGA_Rect updated_rects[MAX_UPDATE_RECT];
static int num_rects = 0;
static libspectrum_byte svgadisplay_force_full_refresh = 1;

typedef void svgadisplay_update_rect_t( int x, int y, int w, int h );

static svgadisplay_update_rect_t *svgadisplay_update_rect;
static svgadisplay_update_rect_t svgadisplay_update_rect_noscale;
static svgadisplay_update_rect_t svgadisplay_update_rect_scale;

typedef void svgadisplay_putpixel_t( int x, int y, libspectrum_word *color );

static svgadisplay_putpixel_t *svgadisplay_putpixel;

static svgadisplay_putpixel_t svgadisplay_putpixel_4;
static svgadisplay_putpixel_t svgadisplay_putpixel_8;
static svgadisplay_putpixel_t svgadisplay_putpixel_15;
static svgadisplay_putpixel_t svgadisplay_putpixel_16;
static svgadisplay_putpixel_t svgadisplay_putpixel_24;

static int svgadisplay_allocate_colours4( void );
static int svgadisplay_allocate_colours8( void );


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

static void register_scalers( void );

typedef struct svga_mode_t {
  int n;
  /* vga_modeinfo *inf; */
  int width, height, bytesperpixel, colors;
  int depth;
} svga_mode_t;

#define SIZE_1x_REGISTER(p,s) if( modes[0].n != -1 && ( p || modes[0].depth > 4 ) ) scaler_register( s )
#define SIZE_2x_REGISTER(p,s) if( modes[1].n != -1 && ( p || modes[1].depth > 4 ) ) scaler_register( s )
#define SIZE_3x_REGISTER(p,s) if( modes[2].n != -1 && ( p || modes[2].depth > 4 ) ) scaler_register( s )

svga_mode_t modes[] = {
  { -1, 0, 0, 0, 0, 0, },		/* 320x240 */
  { -1, 0, 0, 0, 0, 0, },		/* 640x480 */
  { -1, 0, 0, 0, 0, 0, },		/* 960x720 */
};

void
set_mode( int i, int n, vga_modeinfo *inf )
{
  if( inf->colors >= 16 && !( inf->flags & IS_MODEX ) &&
	inf->width >= 256 * ( i + 1 ) &&
	  inf->height >= 192 * ( i + 1 ) ) {
    modes[i].n = n;
    modes[i].width  = inf->width;
    modes[i].height = inf->height;
    modes[i].colors = inf->colors;
    modes[i].bytesperpixel = inf->bytesperpixel ? inf->bytesperpixel : 1;
    modes[i].depth = inf->colors == 16 ? 4 : 
		    ( inf->colors == 256 ? 8 : 
			( inf->colors == 32768 ? 15 :
			    ( inf->colors == 65536 ? 16 :
				inf->bytesperpixel == 3 ? 24 : 32 ) ) );
  }
}

void
find_mode( int exact )
{
  vga_modeinfo *inf;
  int i, j, w, h;

  for( i = 0; i <= vga_lastmodenumber(); i++ ) {
    if( vga_hasmode( i ) ) {
      inf = vga_getmodeinfo( i );
      if( inf->colors >= 16 && !( inf->flags & IS_MODEX ) &&
    	    inf->width >= 320 && inf->height >= 240 &&
		inf->width <= 1280 && inf->height <= 1024 ) {
	/* try exact match */
	for( j = 0; j < 3; j++ ) {
	  w = DISPLAY_ASPECT_WIDTH * ( j + 1 );
	  h = DISPLAY_SCREEN_HEIGHT * ( j + 1 );
	  if( exact == 0 && inf->width == w && inf->height == h &&
	    ( modes[j].n == -1 ||
		( ( modes[j].width != w || modes[j].height != h ||
		    modes[j].depth != 16 ) && (
			inf->colors == 65536 || 
			    inf->colors > modes[j].colors ) ) ) ) {
	    set_mode( j, i, inf );
	  }

	  if( exact == 1 && inf->width >= w && inf->height >= h &&
		inf->width < w * 5 / 4 && inf->height < h * 5 / 4 &&
	    ( modes[j].n == -1 ||
		( ( modes[j].width > inf->width ||
			     modes[j].height > inf->height ||
		    modes[j].depth != 16 ) && (
			inf->colors == 65536 || 
			    inf->colors > modes[j].colors ) ) ) ) {
	    set_mode( j, i, inf );
	  }

	  if( exact == -1 && inf->width <= w && inf->height <= h &&
		inf->width > w * 3 / 4 && inf->height > h * 3 / 4 &&
	    ( modes[j].n == -1 ||
		( ( modes[j].width < inf->width ||
			     modes[j].height < inf->height ||
		    modes[j].depth != 16 ) && (
			inf->colors == 65536 || 
			    inf->colors > modes[j].colors ) ) ) ) {
	    set_mode( j, i, inf );
	  }
	}
      }
    }
  }
}

static void
svgadisplay_setmode( int i )
{
  int w, h;
  w = DISPLAY_ASPECT_WIDTH  * ( i + 1 );
  h = DISPLAY_SCREEN_HEIGHT * ( i + 1 );
  vga_setmode( modes[i].n );
  svgadisplay_depth = modes[i].depth;
  bytesperpixel = modes[i].bytesperpixel;
  xoffs = xclip = 0;
  if( modes[i].width > w ) {
    xoffs = ( modes[i].width - w ) >> 1;
  } else if( modes[i].width < w ) {
    xclip = ( w - modes[i].width ) >> 1;
  }
  yoffs = yclip = 0;
  if( modes[i].height > h ) {
    yoffs = ( modes[i].height - h ) >> 1;
  } else if( modes[i].height < h ) {
    yclip = ( h - modes[i].height ) >> 1;
  }
  if( svgadisplay_depth == 4 )
    svgadisplay_allocate_colours4();
  if( svgadisplay_depth == 8 )
    svgadisplay_allocate_colours8();
}

int
svgadisplay_init( void )
{
  int i, j, found_mode = 0;
  int n0 = -1, n1 = -1, n2 = -1;
  vga_modeinfo *inf;
  vga_init();
  
  if( settings_current.svga_modes && 
	strcmp( settings_current.svga_modes, "list" ) == 0 ) {
    fprintf( stderr, 
    "=====================================================================\n"
    " List of available SVGA modes:\n" 
    "---------------------------------------------------------------------\n"
    "  No. width height colors     Normal       Double       Triple\n"
    "---------------------------------------------------------------------\n"
    );
    for( j = 0; j <= vga_lastmodenumber(); j++ ) {
      if( vga_hasmode( j ) ) {
        inf = vga_getmodeinfo( j );
        if( inf->colors >= 16 && !( inf->flags & IS_MODEX ) &&
		inf->width >= 256 &&
		inf->height >= 192 ) {
	  fprintf( stderr, "% 4d  % 5d% 5d    %s   % 4d%%% 4d%%", j, inf->width, inf->height,
		( inf->colors == 16 ? " 16 " : 
		    ( inf->colors == 256 ? "256 " : 
			( inf->colors == 32768 ? " 32k" :
			    ( inf->colors == 65536 ? " 64k" : " 16M" ) ) ) ),
			inf->width  * 100 / DISPLAY_ASPECT_WIDTH,
			inf->height * 100 / DISPLAY_SCREEN_HEIGHT );
	  if( inf->width >= 512 && inf->height >= 384 ) {
	    fprintf( stderr, "   % 4d%%% 4d%%",
	    		inf->width  * 50 / DISPLAY_ASPECT_WIDTH,
			inf->height * 50 / DISPLAY_SCREEN_HEIGHT );
	  } else {
	    fprintf( stderr, "       N/A   " );
	  }
	  if( inf->width >= 768 && inf->height >= 576 ) {
	    fprintf( stderr, "   % 4d%%% 4d%%\n",
	    		inf->width  * 100 / DISPLAY_ASPECT_WIDTH / 3,
			inf->height * 100 / DISPLAY_SCREEN_HEIGHT / 3 );
	  } else {
	    fprintf( stderr, "       N/A\n" );
	  }
	}
      }
    }
    return 1;
  }

  find_mode( 0 );
  find_mode( 1 );
  find_mode( -1 );
  

  if( settings_current.svga_modes ) {
    sscanf( settings_current.svga_modes, " %i%*[ ,;/|] %i%*[ ,;/|] %i", &n0, &n1, &n2 );
    if( n0 > 0 && vga_hasmode( n0 ) ) set_mode( 0, n0, vga_getmodeinfo( n0 ) );
    if( n1 > 0 && vga_hasmode( n1 ) ) set_mode( 1, n1, vga_getmodeinfo( n1 ) );
    if( n2 > 0 && vga_hasmode( n2 ) ) set_mode( 2, n2, vga_getmodeinfo( n2 ) );
  }

#if 0		/* for debugging */
  for( i = 0; i < 3; i++ ) {
    fprintf( stderr, "svgadisplay_size: %d:", i );
    if( modes[i].n != -1 ) {
      fprintf( stderr, " %d %dx%d %s (%d)", modes[i].n, modes[i].width, modes[i].height, 
    			vga_getmodename( modes[i].n ), modes[i].depth );
    }
    fprintf( stderr, "\n" );
  }
#endif

  for( i = 0; i < 3; i++ ) {
    if( modes[i].n != -1 ) {
      svgadisplay_current_size = i + 1;
      svgadisplay_setmode( i );
      found_mode = 1;
      break;
    }
  }

  /* Error out if we couldn't find a VGA mode */
  if( !found_mode ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a mode to start in" );
    return 1;
  }

  return 0;
}

static void
svgadisplay_putpixel_4( int x, int y, libspectrum_word *colour)
{
  *line_buff_ptr++ = *colour;
}

static void
svgadisplay_putpixel_8( int x, int y, libspectrum_word *colour)
{
  libspectrum_word c = *colour;

  c = (       c         & 0x1f ) * 3 / 31 +
      ( ( ( ( c >>  5 ) & 0x3f ) * 7 / 63 ) << 2 ) +
        ( ( ( c >> 11 )          * 3 / 31 ) << 5 );
  *line_buff_ptr++ = c;
}

static void
svgadisplay_putpixel_15( int x, int y, libspectrum_word *colour )
{
  libspectrum_word c = *colour;

  *(libspectrum_word *)line_buff_ptr = 
		( c & 0x1f ) + ( ( c >>  1 ) & 0xffe0 );
  line_buff_ptr += 2;
}

static void
svgadisplay_putpixel_16( int x, int y, libspectrum_word *colour )
{
  libspectrum_word c = *colour;

  *(libspectrum_word *)line_buff_ptr = c;
  line_buff_ptr += 2;
}

static void
svgadisplay_putpixel_24( int x, int y, libspectrum_word *colour )
{
  libspectrum_dword c = *colour;

  c =   ( ( ( c >> 11 )          * 255 / 31 ) << rShift ) +
        ( ( ( ( c >>  5 ) & 0x3f ) * 255 / 63 ) << gShift ) +
          ( ( ( c         & 0x1f ) * 255 / 31 ) << bShift );
  *(libspectrum_dword *)line_buff_ptr = c;
  line_buff_ptr += 3;
}

static void
svgadisplay_putpixel_32( int x, int y, libspectrum_word *colour )
{
  libspectrum_dword c = *colour;

  c =   ( ( ( c >> 11 )          * 255 / 31 ) << rShift ) +
        ( ( ( ( c >>  5 ) & 0x3f ) * 255 / 63 ) << gShift ) +
          ( ( ( c         & 0x1f ) * 255 / 31 ) << bShift );
  *(libspectrum_dword *)line_buff_ptr = c;
  line_buff_ptr += 4;
}

static int
svgadisplay_allocate_colours4( void )
{
  int i;
  int red, green, blue;

/*    Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B */

  if( settings_current.bw_tv ) {
    for( i=0; i<16; i++ ) {	/* grey */
      red = green = blue = (
	    rgb_for_4[i * 3    ] * 4822 / 255 +
	    rgb_for_4[i * 3 + 1] * 9467 / 255 +
	    rgb_for_4[i * 3 + 2] * 1839 / 255 ) >> 8;
      vga_setpalette( i, red, green, blue );
    }
  } else {
    for( i=0; i<16; i++ ) {	/* rgb */
      red   = rgb_for_4[i * 3    ] * 63 / 255;
      green = rgb_for_4[i * 3 + 1] * 63 / 255;
      blue  = rgb_for_4[i * 3 + 2] * 63 / 255;
      vga_setpalette( i, red, green, blue );
    }
  }

  return 0;
}

static int
svgadisplay_allocate_colours8( void )
{
  int i, r, g, b;
  int red, green, blue;

  i = 0;
  for( r=0; r<4; r++ )		/* rgb232 => 128 */
    for( g=0; g<8; g++ )
      for( b=0; b<4; b++ ) {
        if( settings_current.bw_tv ) {
          red = green = blue = (
		r * 4822 / 3 +
		g * 9467 / 7 +
		b * 1839 / 3 ) >> 8;
	} else {
          red = r * 63 / 3;
	  green = g * 63 / 7;
          blue = b * 63 / 3;
	}
	vga_setpalette( i, red, green, blue );
	i++;
      }

  return 0;
}
  

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
register_scalers( void )
{
  int f = -1;
  
  scaler_register_clear();
  scaler_select_bitformat( 565 );		/* 16bit always */

  if( machine_current->timex ) {
    SIZE_1x_REGISTER( 0, SCALER_HALF );
    SIZE_1x_REGISTER( 1, SCALER_HALFSKIP );
    SIZE_2x_REGISTER( 1, SCALER_NORMAL );
    SIZE_2x_REGISTER( 0, SCALER_PALTV );
    SIZE_3x_REGISTER( 0, SCALER_TIMEXTV );
    SIZE_3x_REGISTER( 1, SCALER_TIMEX1_5X );
  } else {
    SIZE_1x_REGISTER( 1, SCALER_NORMAL );
    SIZE_1x_REGISTER( 0, SCALER_PALTV );
    
    SIZE_2x_REGISTER( 1, SCALER_DOUBLESIZE );
    SIZE_2x_REGISTER( 0, SCALER_2XSAI );
    SIZE_2x_REGISTER( 0, SCALER_SUPER2XSAI );
    SIZE_2x_REGISTER( 0, SCALER_SUPEREAGLE );
    SIZE_2x_REGISTER( 1, SCALER_ADVMAME2X );
    SIZE_2x_REGISTER( 0, SCALER_TV2X );
    SIZE_2x_REGISTER( 0, SCALER_DOTMATRIX );
    SIZE_2x_REGISTER( 0, SCALER_PALTV2X );
    SIZE_2x_REGISTER( 0, SCALER_HQ2X );

    SIZE_3x_REGISTER( 1, SCALER_TRIPLESIZE );
    SIZE_3x_REGISTER( 1, SCALER_ADVMAME3X );
    SIZE_3x_REGISTER( 0, SCALER_TV3X );
    SIZE_3x_REGISTER( 0, SCALER_PALTV3X );
    SIZE_3x_REGISTER( 0, SCALER_HQ3X );
  }
  if( current_scaler != SCALER_NUM )
    f = 4.0 * scaler_get_scaling_factor( current_scaler ) * 
	    ( machine_current->timex ? 2 : 1 );
  if( scaler_is_supported( current_scaler ) &&
	( svgadisplay_current_size * 4 == f ) ) {
    uidisplay_hotswap_gfx_mode();
  } else {
    switch( svgadisplay_current_size ) {
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

static void
svgadisplay_setup_rgb_putpixel( void )
{
  switch( svgadisplay_depth ) {
  case 4:
    svgadisplay_putpixel = svgadisplay_putpixel_4;
    break;
  case 8:
    svgadisplay_putpixel = svgadisplay_putpixel_8;
    break;
  case 15:
    svgadisplay_putpixel = svgadisplay_putpixel_15;
    break;
  case 16:
    svgadisplay_putpixel = svgadisplay_putpixel_16;
    break;
  case 24:
    svgadisplay_putpixel = svgadisplay_putpixel_24;
    break;
  case 32:
    svgadisplay_putpixel = svgadisplay_putpixel_32;
    break;
  }
/*  resize_window( scaled_image_w, scaled_image_h ); */
}

int
uidisplay_hotswap_gfx_mode( void )
{
  image_scale = 4.0 * scaler_get_scaling_factor( current_scaler );
  scaled_image_w = image_width  * image_scale >> 2;
  scaled_image_h = image_height * image_scale >> 2;

  if( svgadisplay_current_size * 4 != 
	image_scale * ( machine_current->timex ? 2 : 1 ) ) {
    svgadisplay_current_size = ( image_scale * ( machine_current->timex ? 2 : 1 ) ) >> 2;
    svgadisplay_setmode( svgadisplay_current_size - 1 );
  }

  if( current_scaler == SCALER_NORMAL )
    svgadisplay_update_rect = svgadisplay_update_rect_noscale;
  else
    svgadisplay_update_rect = svgadisplay_update_rect_scale;

  svgadisplay_force_full_refresh = 1;

  svgadisplay_setup_rgb_putpixel();

  if( settings_current.bw_tv != svgadisplay_bw ) {
    svgadisplay_bw = settings_current.bw_tv;
    if( svgadisplay_depth == 4 )
      svgadisplay_allocate_colours4();
    else if( svgadisplay_depth == 8 )
      svgadisplay_allocate_colours8();
  }
  display_refresh_all();
  return 0;
}

static void
svgadisplay_update_rect_noscale( int x, int y, int w, int h )
{
  int yy, xx;

  if( xclip || yclip ) {
    if( x < xclip ) w -= xclip - x, x = xclip;
    if( y < yclip ) h -= yclip - y, y = yclip;
    if( x + w > scaled_image_w - xclip ) w = scaled_image_w - xclip - x;
    if( y + h > scaled_image_h - yclip ) h = scaled_image_h - yclip - y;
  }
  /* Call putpixel multiple times */
  for( yy = y; yy < y + h; yy++ ) {
    line_buff_ptr = line_buff;
    for( xx = x; xx < x + w; xx++ )
      svgadisplay_putpixel( xx, yy, &rgb_image[yy + 2][xx + 1] );
    if( w > 0 )
      vga_drawscansegment( line_buff, x + xoffs - xclip, yy + yoffs - yclip, w * bytesperpixel );
  }
}

static void
svgadisplay_update_rect_scale( int x, int y, int w, int h )
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
  
  if( xclip || yclip ) {
    if( x < xclip ) w -= xclip - x, x = xclip;
    if( y < yclip ) h -= yclip - y, y = yclip;
    if( x + w > scaled_image_w - xclip ) w = scaled_image_w - xclip - x;
    if( y + h > scaled_image_h - yclip ) h = scaled_image_h - yclip - y;
  }
  /* Call putpixel multiple times */
  for( yy = y; yy < y + h; yy++ ) {
    line_buff_ptr = line_buff;
    for( xx = x; xx < x + w; xx++ )
      svgadisplay_putpixel( xx, yy, &scaled_image[yy][xx] );
    if( w > 0 )
      vga_drawscansegment( line_buff, x + xoffs - xclip, yy + yoffs - yclip, w * bytesperpixel );
  }
}

void
uidisplay_frame_end( void ) 
{
  SVGA_Rect *r, *last_rect;

  /* Force a full redraw if requested */
  if ( svgadisplay_force_full_refresh ) {
    num_rects = 1;

    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

  if ( !( ui_widget_level >= 0 ) && num_rects == 0 ) return;

  last_rect = updated_rects + num_rects;

  for( r = updated_rects; r != last_rect; r++ )
    svgadisplay_update_rect( r->x, r->y, r->w, r->h );
  num_rects = 0;
  svgadisplay_force_full_refresh = 0;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  if ( svgadisplay_force_full_refresh )
    return;

  if( num_rects == MAX_UPDATE_RECT ) {
    svgadisplay_force_full_refresh = 1;
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

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  libspectrum_word pc = svgadisplay_depth == 4 ? colour :
		( settings_current.bw_tv ? pal_grey[ colour ] :
                        	pal_colour[ colour ] );

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
  libspectrum_word pi = svgadisplay_depth == 4 ? ink :
			( settings_current.bw_tv ? pal_grey[ ink ] :
                        	pal_colour[ ink ] );
  libspectrum_word pp = svgadisplay_depth == 4 ? paper :
			( settings_current.bw_tv ? pal_grey[ paper ] :
                        	pal_colour[ paper ] );

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
  libspectrum_word pi = svgadisplay_depth == 4 ? ink :
			( settings_current.bw_tv ? pal_grey[ ink ] :
                        	pal_colour[ ink ] );
  libspectrum_word pp = svgadisplay_depth == 4 ? paper :
			( settings_current.bw_tv ? pal_grey[ paper ] :
                        	pal_colour[ paper ] );
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

int svgadisplay_end( void )
{
  vga_setmode( TEXT );
  return 0;
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
