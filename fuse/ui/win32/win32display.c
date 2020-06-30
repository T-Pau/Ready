/* win32display.c: Routines for dealing with the Win32 GDI display
   Copyright (c) 2003 Philip Kendall, Marek Januszewski, Stuart Brady

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

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "win32internals.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* A copy of every pixel on the screen, replaceable by plotting directly into
   rgb_image below */
libspectrum_word
  win32display_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t win32display_pitch = DISPLAY_SCREEN_WIDTH *
                               sizeof( libspectrum_word );

/* An RGB image of the Spectrum screen; slightly bigger than the real
   screen to handle the smoothing filters which read around each pixel */
static unsigned char rgb_image[ 4 * 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) *
                                        ( DISPLAY_SCREEN_WIDTH  + 3 )   ];
static const int rgb_pitch = ( DISPLAY_SCREEN_WIDTH + 3 ) * 4;

/* The scaled image */
static unsigned char scaled_image[ 3 * DISPLAY_SCREEN_HEIGHT *
                                   6 * DISPLAY_SCREEN_WIDTH ];
static const ptrdiff_t scaled_pitch = 6 * DISPLAY_SCREEN_WIDTH;

/* Win32 specific variables */
static void *win32_pixdata;
static BITMAPINFO fuse_BMI;
static HBITMAP fuse_BMP;
static RECT invalidated_area;

static const unsigned char rgb_colours[16][3] = {

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

libspectrum_dword win32display_colours[16];
static libspectrum_dword bw_colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int win32display_current_size=1;

static int init_colours( void );
static void register_scalers( int force_scaler );
static void win32display_load_gfx_mode( void );

void
blit( void )
{
  PAINTSTRUCT ps;
  HDC dest_dc, src_dc;
  HBITMAP src_bmp;
  int x, y, width, height;

  dest_dc = BeginPaint( fuse_hWnd, &ps );

  x = ps.rcPaint.left;
  y = ps.rcPaint.top;
  width = ps.rcPaint.right - ps.rcPaint.left;
  height = ps.rcPaint.bottom - ps.rcPaint.top;

  if( width && height ) {
    src_dc = CreateCompatibleDC( dest_dc );
    src_bmp = SelectObject( src_dc, fuse_BMP );

    BitBlt( dest_dc, x, y, width, height, src_dc, x, y, SRCCOPY );

    SelectObject( src_dc, src_bmp );
    DeleteDC( src_dc );
  }

  EndPaint( fuse_hWnd, &ps );
}

int
win32display_init( void )
{
  int x, y, error;
  libspectrum_dword black;

  error = init_colours(); if( error ) return error;

  black = settings_current.bw_tv ? bw_colours[0] : win32display_colours[0];

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT + 4; y++ )
    for( x = 0; x < DISPLAY_SCREEN_WIDTH + 3; x++ )
      *(libspectrum_dword*)( rgb_image + y * rgb_pitch + 4 * x ) = black;

  /* create the back buffer */

  memset( &fuse_BMI, 0, sizeof( fuse_BMI ) );
  fuse_BMI.bmiHeader.biSize = sizeof( fuse_BMI.bmiHeader );
  fuse_BMI.bmiHeader.biWidth = (size_t)( 1.5 * DISPLAY_SCREEN_WIDTH );
  /* negative to avoid "shep-mode": */
  fuse_BMI.bmiHeader.biHeight = -( 3 * DISPLAY_SCREEN_HEIGHT );
  fuse_BMI.bmiHeader.biPlanes = 1;
  fuse_BMI.bmiHeader.biBitCount = 32;
  fuse_BMI.bmiHeader.biCompression = BI_RGB;
  fuse_BMI.bmiHeader.biSizeImage = 0;
  fuse_BMI.bmiHeader.biXPelsPerMeter = 0;
  fuse_BMI.bmiHeader.biYPelsPerMeter = 0;
  fuse_BMI.bmiHeader.biClrUsed = 0;
  fuse_BMI.bmiHeader.biClrImportant = 0;
  fuse_BMI.bmiColors[0].rgbRed = 0;
  fuse_BMI.bmiColors[0].rgbGreen = 0;
  fuse_BMI.bmiColors[0].rgbBlue = 0;
  fuse_BMI.bmiColors[0].rgbReserved = 0;

  HDC dc = GetDC( fuse_hWnd );

  fuse_BMP = CreateDIBSection( dc, &fuse_BMI, DIB_RGB_COLORS, &win32_pixdata,
                               NULL, 0 );

  ReleaseDC( fuse_hWnd, dc );

  display_ui_initialised = 1;

  return 0;
}

static int
init_colours( void )
{
  size_t i;

  for( i = 0; i < 16; i++ ) {

    unsigned char red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

#ifdef WORDS_BIGENDIAN

    win32display_colours[i] =  red << 24 | green << 16 | blue << 8;
              bw_colours[i] = grey << 24 |  grey << 16 | grey << 8;

#else                           /* #ifdef WORDS_BIGENDIAN */

    win32display_colours[i] =  red | green << 8 | blue << 16;
              bw_colours[i] = grey |  grey << 8 | grey << 16;

#endif                          /* #ifdef WORDS_BIGENDIAN */

  }

  return 0;
}

int
win32display_drawing_area_resize( int width, int height, int force_scaler )
{
  int size;

  size = width / DISPLAY_ASPECT_WIDTH;
  if( size > height / DISPLAY_SCREEN_HEIGHT )
    size = height / DISPLAY_SCREEN_HEIGHT;

  /* If we're the same size as before, no need to do anything else */
  if( size == win32display_current_size ) return 0;

  win32display_current_size = size;

  register_scalers( force_scaler );

  memset( scaled_image, 0, sizeof( scaled_image ) );
  display_refresh_all();

  return 0;
}
        
int
uidisplay_init( int width, int height )
{
  const char *machine_name;

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;
  SetRectEmpty( &invalidated_area );

  register_scalers( 0 );

  display_refresh_all();

  if ( scaler_select_scaler( current_scaler ) )
        scaler_select_scaler( SCALER_NORMAL );

  win32display_load_gfx_mode();

  machine_name = libspectrum_machine_name( machine_current->machine );
  win32statusbar_update_machine( machine_name );

  return 0;
}

static void
register_scalers( int force_scaler )
{
  scaler_type scaler;

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

  if( force_scaler ) {
    switch( win32display_current_size ) {
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
  if( !IsRectEmpty( &invalidated_area ) ) {

    InvalidateRect( fuse_hWnd, &invalidated_area, FALSE );

    SetRectEmpty( &invalidated_area );

    UpdateWindow( fuse_hWnd );
  }
}

void
uidisplay_area( int x, int y, int w, int h )
{
  float scale = (float)win32display_current_size / image_scale;
  int scaled_x, scaled_y, i, yy;
  libspectrum_dword *palette;

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  scaled_x = scale * x; scaled_y = scale * y;

  palette = settings_current.bw_tv ? bw_colours : win32display_colours;

  /* Create the RGB image */
  for( yy = y; yy < y + h; yy++ ) {

    libspectrum_dword *rgb; libspectrum_word *display;

    rgb = (libspectrum_dword*)( rgb_image + ( yy + 2 ) * rgb_pitch );
    rgb += x + 1;

    display = &win32display_image[yy][x];

    for( i = 0; i < w; i++, rgb++, display++ ) *rgb = palette[ *display ];
  }

  /* Create scaled image */
  scaler_proc32( &rgb_image[ ( y + 2 ) * rgb_pitch + 4 * ( x + 1 ) ],
                 rgb_pitch,
                 &scaled_image[ scaled_y * scaled_pitch + 4 * scaled_x ],
                 scaled_pitch, w, h );

  w *= scale; h *= scale;

  /* Blit to the real screen */
  win32display_area( scaled_x, scaled_y, w, h );
}

void
win32display_area(int x, int y, int width, int height)
{
  int disp_x,disp_y;
  int bottom, right;
  long ofs;
  RECT r;
  char *pixdata = win32_pixdata;

  bottom = y + height;
  right = x + width;

  for( disp_y = y; disp_y < bottom; disp_y++ ) {
    for( disp_x = x; disp_x < right; disp_x++ ) {
      ofs = ( disp_x << 2 ) + ( disp_y * scaled_pitch );

      pixdata[ ofs + 0 ] = scaled_image[ ofs + 2 ]; /* blue */
      pixdata[ ofs + 1 ] = scaled_image[ ofs + 1 ]; /* green */
      pixdata[ ofs + 2 ] = scaled_image[ ofs + 0 ]; /* red */
      pixdata[ ofs + 3 ] = 0; /* unused */
    }
  }

  /* Mark area for updating */
  SetRect( &r, x, y, right, bottom );
  UnionRect( &invalidated_area, &invalidated_area, &r );
}

int
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Setup the new GFX mode */
  win32display_load_gfx_mode();

  fuse_emulation_unpause();

  return 0;
}

int
uidisplay_end( void )
{
  return 0;
}

int
win32display_end( void )
{
  DeleteObject( fuse_BMP );
        
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    win32display_image[y  ][x  ] = colour;
    win32display_image[y  ][x+1] = colour;
    win32display_image[y+1][x  ] = colour;
    win32display_image[y+1][x+1] = colour;
  } else {
    win32display_image[y][x] = colour;
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
      win32display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      win32display_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    win32display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    win32display_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    win32display_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    win32display_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    win32display_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    win32display_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    win32display_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    win32display_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
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
    win32display_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    win32display_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    win32display_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    win32display_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    win32display_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    win32display_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    win32display_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    win32display_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    win32display_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    win32display_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    win32display_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    win32display_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    win32display_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    win32display_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    win32display_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    win32display_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

static void
win32display_load_gfx_mode( void )
{
  float scale;

  scale = scaler_get_scaling_factor( current_scaler );
  win32display_drawing_area_resize( scale * image_width, scale * image_height, 0 );
  win32ui_fuse_resize( scale * image_width, scale * image_height );

  /* Redraw the entire screen... */
  display_refresh_all();
}

int
win32display_scaled_width( void )
{
  return image_width * win32display_current_size;
}

int
win32display_scaled_height( void )
{
  return image_height * win32display_current_size;
}
