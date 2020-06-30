/* screenshot.c: Routines for handling .png and .scr screenshots
   Copyright (c) 2002-2018 Philip Kendall, Fredrick Meunier

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
#include <limits.h>
#include <string.h>

#include <libspectrum.h>

#include "display.h"
#include "machine.h"
#include "peripherals/scld.h"
#include "screenshot.h"
#include "settings.h"
#include "ui/scaler/scaler.h"
#include "ui/ui.h"
#include "utils.h"

#define MONO_BITMAP_SIZE 6144
#define HICOLOUR_SCR_SIZE (2 * MONO_BITMAP_SIZE)
#define MLT_SIZE (2 * MONO_BITMAP_SIZE)
#define HIRES_ATTR HICOLOUR_SCR_SIZE
#define HIRES_SCR_SIZE (HICOLOUR_SCR_SIZE + 1)

#ifdef USE_LIBPNG

#include <png.h>
#ifdef HAVE_ZLIB_H
#define ZLIB_CONST
#include <zlib.h>
#endif				/* #ifdef HAVE_ZLIB_H */

static int get_rgb32_data( libspectrum_byte *rgb32_data, size_t stride,
			   size_t height, size_t width );
static int rgb32_to_rgb24( libspectrum_byte *rgb24_data, size_t rgb24_stride,
			   libspectrum_byte *rgb32_data, size_t rgb32_stride,
			   size_t height, size_t width );

/* The biggest size screen (in units of DISPLAY_ASPECT_WIDTH x
   DISPLAY_SCREEN_HEIGHT ie a Timex screen is size 2) we will be
   creating via the scalers */
#define MAX_SIZE 3

/* The space used for drawing the screen image on. Out here to avoid placing
   these large objects on the stack */
static libspectrum_byte
  rgb_data1[ MAX_SIZE * DISPLAY_SCREEN_HEIGHT * 3 * DISPLAY_ASPECT_WIDTH * 4 ],
  rgb_data2[ MAX_SIZE * DISPLAY_SCREEN_HEIGHT * 3 * DISPLAY_ASPECT_WIDTH * 4 ],
   png_data[ MAX_SIZE * DISPLAY_SCREEN_HEIGHT * 3 * DISPLAY_ASPECT_WIDTH * 3 ];

int
screenshot_write( const char *filename, scaler_type scaler )
{
  FILE *f;

  png_structp png_ptr;
  png_infop info_ptr;

  libspectrum_byte *row_pointers[ MAX_SIZE * DISPLAY_SCREEN_HEIGHT ];
  size_t rgb_stride = MAX_SIZE * DISPLAY_ASPECT_WIDTH * 4,
         png_stride = MAX_SIZE * DISPLAY_ASPECT_WIDTH * 3;
  size_t y, base_height, base_width, height, width;
  int error;

  if( machine_current->timex ) {
    base_height = 2 * DISPLAY_SCREEN_HEIGHT;
    base_width = DISPLAY_SCREEN_WIDTH; 
  } else {
    base_height = DISPLAY_SCREEN_HEIGHT;
    base_width = DISPLAY_ASPECT_WIDTH;
  }

  /* Change from paletted data to RGB data */
  error = get_rgb32_data( rgb_data1, rgb_stride, base_height, base_width );
  if( error ) return error;

  /* Actually scale the data here */
  scaler_get_proc32( scaler )( rgb_data1, rgb_stride, rgb_data2, rgb_stride,
			       base_width, base_height );

  height = base_height * scaler_get_scaling_factor( scaler );
  width  = base_width  * scaler_get_scaling_factor( scaler );

  /* Reduce from RGB(padding byte) to just RGB */
  error = rgb32_to_rgb24( png_data, png_stride, rgb_data2, rgb_stride,
			  height, width );
  if( error ) return error;

  for( y = 0; y < height; y++ )
    row_pointers[y] = &png_data[ y * png_stride ];

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "Couldn't open `%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING,
				     NULL, NULL, NULL );
  if( !png_ptr ) {
    ui_error( UI_ERROR_ERROR, "Couldn't allocate png_ptr" );
    fclose( f );
    return 1;
  }

  info_ptr = png_create_info_struct( png_ptr );
  if( !info_ptr ) {
    ui_error( UI_ERROR_ERROR, "Couldn't allocate info_ptr" );
    png_destroy_write_struct( &png_ptr, NULL );
    fclose( f );
    return 1;
  }

  /* Set up the error handling; libpng will return to here if it
     encounters an error */
  if( setjmp( png_jmpbuf( png_ptr ) ) ) {
    ui_error( UI_ERROR_ERROR, "Error from libpng" );
    png_destroy_write_struct( &png_ptr, &info_ptr );
    fclose( f );
    return 1;
  }

  png_init_io( png_ptr, f );

  /* Make files as small as possible */
  png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );

  png_set_IHDR( png_ptr, info_ptr,
		width, height, 8, 
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT );

  png_set_rows( png_ptr, info_ptr, row_pointers );

  png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );

  png_destroy_write_struct( &png_ptr, &info_ptr );

  if( fclose( f ) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close `%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  return 0;
}

static int
get_rgb32_data( libspectrum_byte *rgb32_data, size_t stride,
		size_t height, size_t width )
{
  size_t i, x, y;

  static const			      /*  R    G    B */
  libspectrum_byte palette[16][3] = { {   0,   0,   0 },
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
				      { 255, 255, 255 } };

  libspectrum_byte grey_palette[16];

  /* Addition of 0.5 is to avoid rounding errors */
  for( i = 0; i < 16; i++ )
    grey_palette[i] = ( 0.299 * palette[i][0] +
			0.587 * palette[i][1] +
			0.114 * palette[i][2]   ) + 0.5;

  for( y = 0; y < height; y++ ) {
    for( x = 0; x < width; x++ ) {

      size_t colour;
      libspectrum_byte red, green, blue;

      colour = display_getpixel( x, y );

      if( settings_current.bw_tv ) {

	red = green = blue = grey_palette[colour];

      } else {

	red   = palette[colour][0];
	green = palette[colour][1];
	blue  = palette[colour][2];

      }
      
      rgb32_data[ y * stride + 4 * x     ] = red;
      rgb32_data[ y * stride + 4 * x + 1 ] = green;
      rgb32_data[ y * stride + 4 * x + 2 ] = blue;
      rgb32_data[ y * stride + 4 * x + 3 ] = 0;		 /* padding */

    }
  }

  return 0;
}

static int
rgb32_to_rgb24( libspectrum_byte *rgb24_data, size_t rgb24_stride,
		libspectrum_byte *rgb32_data, size_t rgb32_stride,
		size_t height, size_t width )
{
  size_t x, y;

  for( y = 0; y < height; y++ ) {
    for( x = 0; x < width; x++ ) {
      rgb24_data[y*rgb24_stride +3*x   ] = rgb32_data[y*rgb32_stride +4*x   ];
      rgb24_data[y*rgb24_stride +3*x +1] = rgb32_data[y*rgb32_stride +4*x +1];
      rgb24_data[y*rgb24_stride +3*x +2] = rgb32_data[y*rgb32_stride +4*x +2];
    }
  }

  return 0;
}

int
screenshot_available_scalers( scaler_type scaler )
{
  if( machine_current->timex ) {

    switch( scaler ) {

    case SCALER_HALF: case SCALER_HALFSKIP: case SCALER_NORMAL:
    case SCALER_TIMEXTV: case SCALER_PALTV:
      return 1;
    default:
      return 0;

    }

  } else {
    
    switch( scaler ) {

    case SCALER_NORMAL: case SCALER_DOUBLESIZE: case SCALER_TRIPLESIZE:
    case SCALER_2XSAI: case SCALER_SUPER2XSAI: case SCALER_SUPEREAGLE:
    case SCALER_ADVMAME2X: case SCALER_ADVMAME3X: case SCALER_TV2X:
    case SCALER_DOTMATRIX: case SCALER_PALTV2X: case SCALER_PALTV3X:
    case SCALER_HQ2X: case SCALER_HQ3X:
      return 1;
    default:
      return 0;

    }
  }
}

#endif				/* #ifdef USE_LIBPNG */

static int
screenshot_scr_hires_write( const char *filename )
{
  libspectrum_byte scr_data[ HIRES_SCR_SIZE ];

  memset( scr_data, 0, HIRES_SCR_SIZE );

  memcpy( scr_data,
          &RAM[ memory_current_screen ][ display_get_addr(0,0) ],
          MONO_BITMAP_SIZE );
  memcpy( scr_data + MONO_BITMAP_SIZE,
          &RAM[ memory_current_screen ][ display_get_addr(0,0) +
          ALTDFILE_OFFSET], MONO_BITMAP_SIZE );
  scr_data[ HIRES_ATTR ] = ( scld_last_dec.byte & HIRESCOLMASK ) |
                             scld_last_dec.mask.scrnmode;

  return utils_write_file( filename, scr_data, HIRES_SCR_SIZE );
}

static int
get_display_last_screen_index( int x, int y )
{
  int beam_x = x + DISPLAY_BORDER_WIDTH_COLS;
  int beam_y = y + DISPLAY_BORDER_HEIGHT;

  return beam_x + beam_y * DISPLAY_SCREEN_WIDTH_COLS;
}

static void
set_hicolor_pixels_and_attribute( int x, int y, libspectrum_byte* scr_data )
{
  int index = get_display_last_screen_index( x, y );
  libspectrum_byte pixel_data = display_last_screen[ index ] & 0xff;
  libspectrum_byte attribute_data = (display_last_screen[ index ] & 0xff00)>>8;

  libspectrum_word offset = display_get_offset( x, y );
  scr_data[ offset ] = pixel_data;
  /* write attribute into bitmap order buffer after bitmap */
  scr_data[ MONO_BITMAP_SIZE + offset ] = attribute_data;
}

typedef void (*data_write_fn)( int x, int y, libspectrum_byte* scr_data );

static int
scr_write( const char *filename, const int data_size,
           data_write_fn scr_data_write )
{
  libspectrum_byte scr_data[ data_size ];
  int x, y;

  memset( scr_data, 0, data_size );

  for( y = 0; y < DISPLAY_HEIGHT; y++ ) {
    for( x = 0; x < DISPLAY_WIDTH_COLS; x++ ) {
      scr_data_write( x, y, scr_data );
    }
  }

  return utils_write_file( filename, scr_data, data_size );
}

static void
set_standard_pixels_and_attribute( int x, int y, libspectrum_byte* scr_data )
{
  int index = get_display_last_screen_index( x, y );
  libspectrum_byte pixel_data = display_last_screen[ index ] & 0xff;
  libspectrum_byte attribute_data = (display_last_screen[ index ] & 0xff00)>>8;
  int attribute_offset;

  scr_data[ display_get_offset( x, y ) ] = pixel_data;

  if( y%8 == 0 ) {
    /* write attribute into standard attribute order buffer after bitmap */
    attribute_offset = x + (y/8 * DISPLAY_WIDTH_COLS);
    scr_data[ MONO_BITMAP_SIZE + attribute_offset ] = attribute_data;
  }
}

int
screenshot_scr_write( const char *filename )
{
  if( scld_last_dec.name.hires ) {
    return screenshot_scr_hires_write( filename );
  }
  else if( scld_last_dec.name.b1 ) {
    return scr_write( filename, HICOLOUR_SCR_SIZE,
                      &set_hicolor_pixels_and_attribute );
  }
  else { /* ALTDFILE and default */
    return scr_write( filename, STANDARD_SCR_SIZE,
                      &set_standard_pixels_and_attribute );
  }
}

static void
set_mlt_pixels_and_attribute( int x, int y, libspectrum_byte* mlt_data )
{
  int index = get_display_last_screen_index( x, y );
  libspectrum_byte pixel_data = display_last_screen[ index ] & 0xff;
  libspectrum_byte attribute_data = (display_last_screen[ index ] & 0xff00)>>8;

  mlt_data[ display_get_offset( x, y ) ] = pixel_data;
  /* write attribute into linear buffer following bitmap */
  mlt_data[ MONO_BITMAP_SIZE + x + y * DISPLAY_WIDTH_COLS ] = attribute_data;
}

int
screenshot_mlt_write( const char *filename )
{
  if( machine_current->timex && scld_last_dec.name.hires ) {
    ui_error( UI_ERROR_ERROR,
              "MLT format not supported for Timex hi-res screen mode" );
    return 1;
  }

  return scr_write( filename, MLT_SIZE, &set_mlt_pixels_and_attribute );
}

#ifdef WORDS_BIGENDIAN

typedef struct {
  unsigned b0 : 1;
  unsigned b1 : 1;
  unsigned b2 : 1;
  unsigned b3 : 1;
  unsigned b4 : 1;
  unsigned b5 : 1;
  unsigned b6 : 1;
  unsigned b7 : 1;
} byte_field_type;

#else			/* #ifdef WORDS_BIGENDIAN */

typedef struct {
  unsigned b7 : 1;
  unsigned b6 : 1;
  unsigned b5 : 1;
  unsigned b4 : 1;
  unsigned b3 : 1;
  unsigned b2 : 1;
  unsigned b1 : 1;
  unsigned b0 : 1;
} byte_field_type;

#endif			/* #ifdef WORDS_BIGENDIAN */

typedef union {
  libspectrum_byte byte;
  byte_field_type bit;
} bft_union;

static libspectrum_byte
convert_hires_to_lores( libspectrum_byte high, libspectrum_byte low )
{
  bft_union ret, h, l;

  h.byte = high;
  l.byte = low;

  ret.bit.b0 = l.bit.b0;
  ret.bit.b1 = l.bit.b2;
  ret.bit.b2 = l.bit.b4;
  ret.bit.b3 = l.bit.b6;
  ret.bit.b4 = h.bit.b0;
  ret.bit.b5 = h.bit.b2;
  ret.bit.b6 = h.bit.b4;
  ret.bit.b7 = h.bit.b6;

  return ret.byte;
}

int
screenshot_scr_read( const char *filename )
{
  int error = 0;
  int i;
  utils_file screen;

  error =  utils_read_file( filename, &screen );
  if( error ) return error;

  switch( screen.length ) {
  case STANDARD_SCR_SIZE:
    memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0)],
	    screen.buffer, screen.length );

    /* If it is a Timex and it is in hi colour or hires mode, switch out of
       hires or hicolour mode */
    if( scld_last_dec.name.b1 || scld_last_dec.name.hires )
      scld_dec_write( 0xff, scld_last_dec.byte & ~HIRES );
    break;

  case HICOLOUR_SCR_SIZE:
    /* If it is a Timex and it is not in hi colour mode, copy screen and switch
        mode if neccesary */
    /* If it is not a Timex copy the mono bitmap and raise an error */
    if( machine_current->timex ) {
      if( !scld_last_dec.name.b1 )
        scld_dec_write( 0xff, ( scld_last_dec.byte & ~HIRESATTR ) | EXTCOLOUR );
      memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0) +
              ALTDFILE_OFFSET], screen.buffer + MONO_BITMAP_SIZE,
	      MONO_BITMAP_SIZE );
    } else
      ui_error( UI_ERROR_INFO,
            "The file contained a TC2048 high-colour screen, loaded as mono");

    memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0)],
              screen.buffer, MONO_BITMAP_SIZE );
    break;

  case HIRES_SCR_SIZE:
    /* If it is a Timex and it is not in hi res mode, copy screen and switch
        mode if neccesary */
    /* If it is not a Timex scale the bitmap and raise an error */
    if( machine_current->timex ) {
      memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0)],
                screen.buffer, MONO_BITMAP_SIZE );

      memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0)] +
              ALTDFILE_OFFSET, screen.buffer + MONO_BITMAP_SIZE,
	      MONO_BITMAP_SIZE );
      if( !scld_last_dec.name.hires )
        scld_dec_write( 0xff,
            ( scld_last_dec.byte & ~( HIRESCOLMASK | HIRES ) ) |
            ( *(screen.buffer + HIRES_ATTR) & ( HIRESCOLMASK | HIRES ) ) );
    } else {
      libspectrum_byte attr = hires_convert_dec( *(screen.buffer + HIRES_ATTR) );

      for( i = 0; i < MONO_BITMAP_SIZE; i++ )
        RAM[ memory_current_screen ][display_get_addr(0,0) + i] =
          convert_hires_to_lores( *(screen.buffer + MONO_BITMAP_SIZE + i),
                                  *(screen.buffer + i) );

      /* set attributes based on hires attribute byte */
      for( i = 0; i < 768; i++ )
        RAM[ memory_current_screen ][display_get_addr(0,0) +
            MONO_BITMAP_SIZE + i] = attr;

      ui_error( UI_ERROR_INFO,
            "The file contained a TC2048 high-res screen, converted to lores");
    }
    break;

  default:
    ui_error( UI_ERROR_ERROR, "'%s' is not a valid scr file", filename );
    error = 1;
  }

  utils_close_file( &screen );

  display_refresh_all();

  return error;
}

int
screenshot_mlt_read( const char *filename )
{
  int error = 0;
  int x, y;
  utils_file screen;

  error =  utils_read_file( filename, &screen );
  if( error ) return error;

  if( screen.length != MLT_SIZE ) {
    ui_error( UI_ERROR_ERROR, "MLT picture ('%s') is not %d bytes long",
	      filename, MLT_SIZE );
    return 1;
  }

  /* If it is a Timex and it is not in hi colour mode, copy screen and switch
      mode if neccesary */
  /* If it is not a Timex copy the mono bitmap and raise an error */
  if( machine_current->timex ) {
    if( !scld_last_dec.name.b1 )
      scld_dec_write( 0xff, ( scld_last_dec.byte & ~HIRESATTR ) | EXTCOLOUR );

    for( y = 0; y < DISPLAY_HEIGHT; y++ ) {
      for( x = 0; x < DISPLAY_WIDTH_COLS; x++ ) {
        RAM[ memory_current_screen ][display_get_addr(x,y) + ALTDFILE_OFFSET] =
          *(screen.buffer + MONO_BITMAP_SIZE + x + y * DISPLAY_WIDTH_COLS);
      }
    }
  } else
    ui_error( UI_ERROR_INFO,
          "The file contained a MLT high-colour screen, loaded as mono");

  memcpy( &RAM[ memory_current_screen ][display_get_addr(0,0)],
            screen.buffer, MONO_BITMAP_SIZE );

  utils_close_file( &screen );

  display_refresh_all();

  return error;
}
