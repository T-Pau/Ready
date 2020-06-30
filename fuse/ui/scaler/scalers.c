/* scalers.c: the actual graphics scalers
 * Copyright (C) 2003-2015 Fredrick Meunier, Philip Kendall, Gergely Szasz
 *
 * Originally taken from ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2003 The ScummVM project
 *
 * HQ2x and HQ3x scalers taken from HiEnd3D Demos (http://www.hiend3d.com)
 * Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <config.h>

#include <string.h>

#include <libspectrum.h>

#include "scaler.h"
#include "scaler_internals.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(x)     ((x)>=0?(x):-(x))
#endif

/* The actual code for the scalers starts here */

#if SCALER_DATA_SIZE == 2

typedef libspectrum_word scaler_data_type;
#define FUNCTION( name ) name##_16

static libspectrum_dword colorMask;
static libspectrum_dword lowPixelMask;
static libspectrum_dword qcolorMask;
static libspectrum_dword qlowpixelMask;
static libspectrum_dword redblueMask;
static libspectrum_dword redblue8_Mask;
static libspectrum_dword redblue16_Mask;
static libspectrum_dword redMask;
static libspectrum_dword greenMask;
static libspectrum_dword green8_Mask;
static libspectrum_dword green16_Mask;
static libspectrum_dword blueMask;
static int green6bit;

static const libspectrum_word dotmatrix_565[16] = {
  0x01E0, 0x0007, 0x3800, 0x0000,
  0x39E7, 0x0000, 0x39E7, 0x0000,
  0x3800, 0x0000, 0x01E0, 0x0007,
  0x39E7, 0x0000, 0x39E7, 0x0000
};
static const libspectrum_word dotmatrix_555[16] = {
  0x00E0, 0x0007, 0x1C00, 0x0000,
  0x1CE7, 0x0000, 0x1CE7, 0x0000,
  0x1C00, 0x0000, 0x00E0, 0x0007,
  0x1CE7, 0x0000, 0x1CE7, 0x0000
};
static const libspectrum_word *dotmatrix;

int 
scaler_select_bitformat( libspectrum_dword BitFormat )
{
  switch( BitFormat ) {

    /* FIXME(?): there is an assumption here that our colour fields
       are (*) xxxx|xyyy|yyyz|zzzz for the 565 mode
       and (*) xxxx|xyyy|yyzz|zzz0 for the 555 mode, where (*) is the
       least significant bit on LSB machines and the most significant
       bit on MSB machines. This is currently (April 2003) OK as the
       only user interface to use this code is SDL, which hides all
       variation in SDL_MapRGB(3), but be very careful (especially
       about endianness) if we ever use the "interpolating" scalers
       from another user interface */

  case 565:
    colorMask = 0x0000F7DE;
    lowPixelMask = 0x00000821;
    qcolorMask = 0x0000E79C;
    qlowpixelMask = 0x00001863;
    redblueMask = 0x0000F81F;
    redblue8_Mask = 0x0007C0F8;
    redblue16_Mask = 0x000F81F0;
    green6bit = 1;
    redMask   = 0x0000001F;
    greenMask = 0x000007E0;
    green8_Mask = 0x00003F00;
    green16_Mask = 0x00007E00;
    blueMask  = 0x0000F800;
    dotmatrix = dotmatrix_565;
    break;

  case 555:
    colorMask = 0x00007BDE;
    lowPixelMask = 0x00000421;
    qcolorMask = 0x0000739C;
    qlowpixelMask = 0x00000C63;
    redblueMask = 0x00007C1F;
    redblue8_Mask = 0x0003E0F8;
    redblue16_Mask = 0x0007C1F0;
    green6bit = 0;
    redMask   = 0x0000001F;
    greenMask = 0x000003E0;
    green8_Mask = 0x00001F00;
    green16_Mask = 0x00003E00;
    blueMask  = 0x00007C00;
    dotmatrix = dotmatrix_555;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown bitformat %d", BitFormat );
    return 1;

  }

  return 0;
}

#elif SCALER_DATA_SIZE == 4	/* #if SCALER_DATA_SIZE == 2 */

typedef libspectrum_dword scaler_data_type;
#define FUNCTION( name ) name##_32

/* The assumption here is that the colour fields are laid out in
   memory as (LSB) red|green|blue|padding (MSB). We wish to access
   these as 32-bit entities, so make sure we get our masks the right
   way round. */

#ifdef WORDS_BIGENDIAN

static const libspectrum_dword colorMask = 0xFEFEFE00;
static const libspectrum_dword lowPixelMask = 0x01010100;
static const libspectrum_dword qcolorMask = 0xFCFCFC00;
static const libspectrum_dword qlowpixelMask = 0x03030300;
static const libspectrum_qword redblueMask = 0xFF00FF00;
static const libspectrum_dword redblue8_Mask = 0xF807F807;
static const libspectrum_dword redblue16_Mask = 0x0FF00FF0;
static const libspectrum_dword redMask =   0xFF000000;
static const libspectrum_dword greenMask = 0x00FF0000;
static const libspectrum_dword blueMask =  0x0000FF00;
static const libspectrum_dword green8_Mask = 0x008F0700;
static const libspectrum_dword green16_Mask = 0x000FF000;

static const libspectrum_dword dotmatrix[16] = {
  0x003F0000, 0x00003F00, 0x3F000000, 0x00000000,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000,
  0x3F000000, 0x00000000, 0x003F0000, 0x00003F00,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000
};

#else				/* #ifdef WORDS_BIGENDIAN */

static const libspectrum_dword colorMask = 0x00FEFEFE;
static const libspectrum_dword lowPixelMask = 0x00010101;
static const libspectrum_dword qcolorMask = 0x00FCFCFC;
static const libspectrum_dword qlowpixelMask = 0x00030303;
static const libspectrum_qword redblueMask = 0x00FF00FF;
static const libspectrum_dword redblue8_Mask = 0x07F807F8;
static const libspectrum_dword redblue16_Mask = 0x0FF00FF0;
static const libspectrum_dword redMask =   0x000000FF;
static const libspectrum_dword greenMask = 0x0000FF00;
static const libspectrum_dword blueMask =  0x00FF0000;
static const libspectrum_dword green8_Mask = 0x0007F800;
static const libspectrum_dword green16_Mask = 0x000FF000;

static const libspectrum_dword dotmatrix[16] = {
  0x00003F00, 0x003F0000, 0x0000003F, 0x00000000,
  0x003F3F3F, 0x00000000, 0x003F3F3F, 0x00000000,
  0x0000003F, 0x00000000, 0x00003F00, 0x003F0000,
  0x003F3F3F, 0x00000000, 0x003F3F3F, 0x00000000
};

#endif				/* #ifdef WORDS_BIGENDIAN */

#else				/* #if SCALER_DATA_SIZE == 2 or 4 */
#error Unknown SCALER_DATA_SIZE
#endif				/* #if SCALER_DATA_SIZE == 2 or 4 */

static inline int 
GetResult( libspectrum_dword A, libspectrum_dword B, libspectrum_dword C,
	   libspectrum_dword D )
{
  const int ac = (A==C);
  const int bc = (B==C);
  const int x1 = ac;
  const int y1 = (bc & !ac);
  const int ad = (A==D);
  const int bd = (B==D);
  const int x2 = ad;
  const int y2 = (bd & !ad);
  const int x = x1+x2;
  const int y = y1+y2;
  static const int rmap[3][3] = {
      {0, 0, -1},
      {0, 0, -1},
      {1, 1,  0}
    };
  return rmap[y][x];
}

static inline libspectrum_dword 
INTERPOLATE( libspectrum_dword A, libspectrum_dword B )
{
  if (A != B) {
    return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask));
  } else
    return A;
}

static inline libspectrum_dword 
Q_INTERPOLATE( libspectrum_dword A, libspectrum_dword B, libspectrum_dword C,
	       libspectrum_dword D )
{
  register libspectrum_dword x = ((A & qcolorMask) >> 2) +
  ((B & qcolorMask) >> 2) + ((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
  register libspectrum_dword y = (A & qlowpixelMask) +
  (B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);

  y = (y >> 2) & qlowpixelMask;
  return x + y;
}

/* HQ scalers */
#define HQ_INTERPOLATE_1(A,B) Q_INTERPOLATE(A,A,A,B)

#define HQ_INTERPOLATE_2(A,B,C) Q_INTERPOLATE(A,A,B,C)

static inline libspectrum_dword
HQ_INTERPOLATE_3( libspectrum_dword A, libspectrum_dword B )
{
  return ( ( ( ( A & greenMask ) * 7 + ( B & greenMask) ) & green8_Mask ) +
         ( ( ( A & redblueMask ) * 7 + ( B & redblueMask ) ) & redblue8_Mask )
         ) >> 3;
}

static inline libspectrum_dword
HQ_INTERPOLATE_4( libspectrum_dword A, libspectrum_dword B,
                  libspectrum_dword C )
{
  return ( ( ( ( A & greenMask ) * 2 + ( ( B & greenMask) + 
		( C & greenMask ) ) * 7 ) & green16_Mask) +
         ( ( ( A & redblueMask ) * 2 + ( ( B & redblueMask ) +
		( C & redblueMask ) ) * 7 ) & redblue16_Mask ) ) >> 4;
}

#define HQ_INTERPOLATE_5(A,B) INTERPOLATE(A,B)

static inline libspectrum_dword
HQ_INTERPOLATE_6( libspectrum_dword A, libspectrum_dword B,
                  libspectrum_dword C )
{
  return ( ( ( ( A & greenMask ) * 5 + ( B & greenMask) * 2 + 
		( C & greenMask ) ) & green8_Mask) +
         ( ( ( A & redblueMask ) * 5 + ( B & redblueMask ) * 2 +
		( C & redblueMask ) ) & redblue8_Mask ) ) >> 3;
}

static inline libspectrum_dword
HQ_INTERPOLATE_7( libspectrum_dword A, libspectrum_dword B,
                  libspectrum_dword C )
{
  return ( ( ( ( A & greenMask ) * 6 + ( B & greenMask) + 
		( C & greenMask ) ) & green8_Mask) +
         ( ( ( A & redblueMask ) * 6 + ( B & redblueMask ) +
		( C & redblueMask ) ) & redblue8_Mask ) ) >> 3;
}

static inline libspectrum_dword
HQ_INTERPOLATE_9( libspectrum_dword A, libspectrum_dword B,
                  libspectrum_dword C )
{
  return ( ( ( ( A & greenMask ) * 2 + ( ( B & greenMask) + 
		( C & greenMask ) ) * 3 ) & green8_Mask) +
         ( ( ( A & redblueMask ) * 2 + ( ( B & redblueMask ) +
		( C & redblueMask ) ) * 3 ) & redblue8_Mask ) ) >> 3;
}

static inline libspectrum_dword
HQ_INTERPOLATE_10( libspectrum_dword A, libspectrum_dword B,
                   libspectrum_dword C )
{
  return ( ( ( ( A & greenMask ) * 14 + ( B & greenMask) + 
		( C & greenMask ) ) & green16_Mask) +
         ( ( ( A & redblueMask ) * 14 + ( B & redblueMask ) +
		( C & redblueMask ) ) & redblue16_Mask ) ) >> 4;
}

/* dstPtr */
#define HQ_PIXEL00_0     w[5]
#define HQ_PIXEL00_10    HQ_INTERPOLATE_1(w[5], w[1])
#define HQ_PIXEL00_11    HQ_INTERPOLATE_1(w[5], w[4])
#define HQ_PIXEL00_12    HQ_INTERPOLATE_1(w[5], w[2])
#define HQ_PIXEL00_20    HQ_INTERPOLATE_2(w[5], w[4], w[2])
#define HQ_PIXEL00_21    HQ_INTERPOLATE_2(w[5], w[1], w[2])
#define HQ_PIXEL00_22    HQ_INTERPOLATE_2(w[5], w[1], w[4])
#define HQ_PIXEL00_60    HQ_INTERPOLATE_6(w[5], w[2], w[4])
#define HQ_PIXEL00_61    HQ_INTERPOLATE_6(w[5], w[4], w[2])
#define HQ_PIXEL00_70    HQ_INTERPOLATE_7(w[5], w[4], w[2])
#define HQ_PIXEL00_90    HQ_INTERPOLATE_9(w[5], w[4], w[2])
#define HQ_PIXEL00_100   HQ_INTERPOLATE_10(w[5], w[4], w[2])
/* +1 */
#define HQ_PIXEL01_0     w[5]
#define HQ_PIXEL01_10    HQ_INTERPOLATE_1(w[5], w[3])
#define HQ_PIXEL01_11    HQ_INTERPOLATE_1(w[5], w[2])
#define HQ_PIXEL01_12    HQ_INTERPOLATE_1(w[5], w[6])
#define HQ_PIXEL01_20    HQ_INTERPOLATE_2(w[5], w[2], w[6])
#define HQ_PIXEL01_21    HQ_INTERPOLATE_2(w[5], w[3], w[6])
#define HQ_PIXEL01_22    HQ_INTERPOLATE_2(w[5], w[3], w[2])
#define HQ_PIXEL01_60    HQ_INTERPOLATE_6(w[5], w[6], w[2])
#define HQ_PIXEL01_61    HQ_INTERPOLATE_6(w[5], w[2], w[6])
#define HQ_PIXEL01_70    HQ_INTERPOLATE_7(w[5], w[2], w[6])
#define HQ_PIXEL01_90    HQ_INTERPOLATE_9(w[5], w[2], w[6])
#define HQ_PIXEL01_100   HQ_INTERPOLATE_10(w[5], w[2], w[6])
/* +dstPitch */
#define HQ_PIXEL10_0     w[5]
#define HQ_PIXEL10_10    HQ_INTERPOLATE_1(w[5], w[7])
#define HQ_PIXEL10_11    HQ_INTERPOLATE_1(w[5], w[8])
#define HQ_PIXEL10_12    HQ_INTERPOLATE_1(w[5], w[4])
#define HQ_PIXEL10_20    HQ_INTERPOLATE_2(w[5], w[8], w[4])
#define HQ_PIXEL10_21    HQ_INTERPOLATE_2(w[5], w[7], w[4])
#define HQ_PIXEL10_22    HQ_INTERPOLATE_2(w[5], w[7], w[8])
#define HQ_PIXEL10_60    HQ_INTERPOLATE_6(w[5], w[4], w[8])
#define HQ_PIXEL10_61    HQ_INTERPOLATE_6(w[5], w[8], w[4])
#define HQ_PIXEL10_70    HQ_INTERPOLATE_7(w[5], w[8], w[4])
#define HQ_PIXEL10_90    HQ_INTERPOLATE_9(w[5], w[8], w[4])
#define HQ_PIXEL10_100   HQ_INTERPOLATE_10(w[5], w[8], w[4])
/* +dstPitch+1*/
#define HQ_PIXEL11_0     w[5]
#define HQ_PIXEL11_10    HQ_INTERPOLATE_1(w[5], w[9])
#define HQ_PIXEL11_11    HQ_INTERPOLATE_1(w[5], w[6])
#define HQ_PIXEL11_12    HQ_INTERPOLATE_1(w[5], w[8])
#define HQ_PIXEL11_20    HQ_INTERPOLATE_2(w[5], w[6], w[8])
#define HQ_PIXEL11_21    HQ_INTERPOLATE_2(w[5], w[9], w[8])
#define HQ_PIXEL11_22    HQ_INTERPOLATE_2(w[5], w[9], w[6])
#define HQ_PIXEL11_60    HQ_INTERPOLATE_6(w[5], w[8], w[6])
#define HQ_PIXEL11_61    HQ_INTERPOLATE_6(w[5], w[6], w[8])
#define HQ_PIXEL11_70    HQ_INTERPOLATE_7(w[5], w[6], w[8])
#define HQ_PIXEL11_90    HQ_INTERPOLATE_9(w[5], w[6], w[8])
#define HQ_PIXEL11_100   HQ_INTERPOLATE_10(w[5], w[6], w[8])

/* dstPtr */
#define HQ_PIXEL00_1M  HQ_INTERPOLATE_1(w[5], w[1])
#define HQ_PIXEL00_1U  HQ_INTERPOLATE_1(w[5], w[2])
#define HQ_PIXEL00_1L  HQ_INTERPOLATE_1(w[5], w[4])
#define HQ_PIXEL00_2   HQ_INTERPOLATE_2(w[5], w[4], w[2])
#define HQ_PIXEL00_4   HQ_INTERPOLATE_4(w[5], w[4], w[2])
#define HQ_PIXEL00_5   HQ_INTERPOLATE_5(w[4], w[2])
#define HQ_PIXEL00_C   w[5]
/* dstPtr+1*/
#define HQ_PIXEL01_1   HQ_INTERPOLATE_1(w[5], w[2])
#define HQ_PIXEL01_3   HQ_INTERPOLATE_3(w[5], w[2])
#define HQ_PIXEL01_6   HQ_INTERPOLATE_1(w[2], w[5])
#define HQ_PIXEL01_C   w[5]
/* dstPtr+2*/
#define HQ_PIXEL02_1M  HQ_INTERPOLATE_1(w[5], w[3])
#define HQ_PIXEL02_1U  HQ_INTERPOLATE_1(w[5], w[2])
#define HQ_PIXEL02_1R  HQ_INTERPOLATE_1(w[5], w[6])
#define HQ_PIXEL02_2   HQ_INTERPOLATE_2(w[5], w[2], w[6])
#define HQ_PIXEL02_4   HQ_INTERPOLATE_4(w[5], w[2], w[6])
#define HQ_PIXEL02_5   HQ_INTERPOLATE_5(w[2], w[6])
#define HQ_PIXEL02_C   w[5]
/* + dstPitch*/
#define HQ_PIXEL10_1   HQ_INTERPOLATE_1(w[5], w[4])
#define HQ_PIXEL10_3   HQ_INTERPOLATE_3(w[5], w[4])
#define HQ_PIXEL10_6   HQ_INTERPOLATE_1(w[4], w[5])
#define HQ_PIXEL10_C   w[5]
/* + dstPitch + 1 */
#define HQ_PIXEL11     w[5]
/* + dstPitch + 2 */
#define HQ_PIXEL12_1   HQ_INTERPOLATE_1(w[5], w[6])
#define HQ_PIXEL12_3   HQ_INTERPOLATE_3(w[5], w[6])
#define HQ_PIXEL12_6   HQ_INTERPOLATE_1(w[6], w[5])
#define HQ_PIXEL12_C   w[5]
/* + 2*dstPitch */
#define HQ_PIXEL20_1M  HQ_INTERPOLATE_1(w[5], w[7])
#define HQ_PIXEL20_1D  HQ_INTERPOLATE_1(w[5], w[8])
#define HQ_PIXEL20_1L  HQ_INTERPOLATE_1(w[5], w[4])
#define HQ_PIXEL20_2   HQ_INTERPOLATE_2(w[5], w[8], w[4])
#define HQ_PIXEL20_4   HQ_INTERPOLATE_4(w[5], w[8], w[4])
#define HQ_PIXEL20_5   HQ_INTERPOLATE_5(w[8], w[4])
#define HQ_PIXEL20_C   w[5]
/* + 2*dstPitch + 1 */
#define HQ_PIXEL21_1   HQ_INTERPOLATE_1(w[5], w[8])
#define HQ_PIXEL21_3   HQ_INTERPOLATE_3(w[5], w[8])
#define HQ_PIXEL21_6   HQ_INTERPOLATE_1(w[8], w[5])
#define HQ_PIXEL21_C   w[5]
/* + 2*dstPitch + 2 */
#define HQ_PIXEL22_1M  HQ_INTERPOLATE_1(w[5], w[9])
#define HQ_PIXEL22_1D  HQ_INTERPOLATE_1(w[5], w[8])
#define HQ_PIXEL22_1R  HQ_INTERPOLATE_1(w[5], w[6])
#define HQ_PIXEL22_2   HQ_INTERPOLATE_2(w[5], w[6], w[8])
#define HQ_PIXEL22_4   HQ_INTERPOLATE_4(w[5], w[6], w[8])
#define HQ_PIXEL22_5   HQ_INTERPOLATE_5(w[6], w[8])
#define HQ_PIXEL22_C   w[5]

#define HQ_trY 0x00000030
#define HQ_trU 0x00000007
#define HQ_trV 0x00000006

#define HQ_YUVDIFF(y1,u1,v1,y2,u2,v2) \
  ( ( ABS( y1 - y2 ) > HQ_trY ) || \
    ( ABS( u1 - u2 ) > HQ_trU ) || \
    ( ABS( v1 - v2 ) > HQ_trV ) )

void 
FUNCTION( scaler_Super2xSaI )( const libspectrum_byte *srcPtr,
			       libspectrum_dword srcPitch,
			       libspectrum_byte *dstPtr,
			       libspectrum_dword dstPitch,
			       int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    const libspectrum_dword nextlineSrc = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );
    size_t i;

    while (height--) {
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( i = 0; i < width; ++i ) {
	libspectrum_dword color4, color5, color6;
	libspectrum_dword color1, color2, color3;
	libspectrum_dword colorA0, colorA1, colorA2, colorA3, colorB0, colorB1, colorB2,
	 colorB3, colorS1, colorS2;
	libspectrum_dword product1a, product1b, product2a, product2b;

/*---------------------------------------    B1 B2
                                           4  5  6 S2
                                           1  2  3 S1
	                                     A1 A2
*/

        colorB0 = *(bP - nextlineSrc - 1);
	colorB1 = *(bP - nextlineSrc);
	colorB2 = *(bP - nextlineSrc + 1);
	colorB3 = *(bP - nextlineSrc + 2);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + nextlineSrc - 1);
	color2 = *(bP + nextlineSrc);
	color3 = *(bP + nextlineSrc + 1);
	colorS1 = *(bP + nextlineSrc + 2);

	colorA0 = *(bP + 2 * nextlineSrc - 1);
	colorA1 = *(bP + 2 * nextlineSrc);
	colorA2 = *(bP + 2 * nextlineSrc + 1);
	colorA3 = *(bP + 2 * nextlineSrc + 2);

/*--------------------------------------*/
	if (color2 == color6 && color5 != color3) {
	  product2b = product1b = color2;
	} else if (color5 == color3 && color2 != color6) {
	  product2b = product1b = color5;
	} else if (color5 == color3 && color2 == color6) {
	  register int r = 0;

	  r += GetResult(color6, color5, color1, colorA1);
	  r += GetResult(color6, color5, color4, colorB1);
	  r += GetResult(color6, color5, colorA2, colorS1);
	  r += GetResult(color6, color5, colorB2, colorS2);

	  if (r > 0)
	    product2b = product1b = color6;
	  else if (r < 0)
	    product2b = product1b = color5;
	  else {
	    product2b = product1b = INTERPOLATE(color5, color6);
	  }
	} else {
	  if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
	    product2b = Q_INTERPOLATE(color3, color3, color3, color2);
	  else if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
	    product2b = Q_INTERPOLATE(color2, color2, color2, color3);
	  else
	    product2b = INTERPOLATE(color2, color3);

	  if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
	    product1b = Q_INTERPOLATE(color6, color6, color6, color5);
	  else if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
	    product1b = Q_INTERPOLATE(color6, color5, color5, color5);
	  else
	    product1b = INTERPOLATE(color5, color6);
	}

	if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
	  product2a = INTERPOLATE(color2, color5);
	else if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
	  product2a = INTERPOLATE(color2, color5);
	else
	  product2a = color2;

	if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
	  product1a = INTERPOLATE(color2, color5);
	else if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
	  product1a = INTERPOLATE(color2, color5);
	else
	  product1a = color5;

	*dP = (scaler_data_type)product1a;
	*(dP+nextDstLine) = (scaler_data_type)product2a;
	dP++;
	*dP = (scaler_data_type)product1b;
	*(dP+nextDstLine) = (scaler_data_type)product2b;
	dP++;

	bP++;
      }

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }
  }
}

void 
FUNCTION( scaler_SuperEagle )( const libspectrum_byte *srcPtr,
			       libspectrum_dword srcPitch,
			       libspectrum_byte *dstPtr,
			       libspectrum_dword dstPitch,
			       int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    size_t i;
    const libspectrum_dword nextlineSrc = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;
      for( i = 0; i < width; ++i ) {
	libspectrum_dword color4, color5, color6;
	libspectrum_dword color1, color2, color3;
	libspectrum_dword colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
	libspectrum_dword product1a, product1b, product2a, product2b;

	colorB1 = *(bP - nextlineSrc);
	colorB2 = *(bP - nextlineSrc + 1);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + nextlineSrc - 1);
	color2 = *(bP + nextlineSrc);
	color3 = *(bP + nextlineSrc + 1);
	colorS1 = *(bP + nextlineSrc + 2);

	colorA1 = *(bP + 2 * nextlineSrc);
	colorA2 = *(bP + 2 * nextlineSrc + 1);

	/* -------------------------------------- */
        if (color5 != color3 )
        {
          if (color2 == color6)
          {
            product1b = product2a = color2;
            if ((color1 == color2) || (color6 == colorB2)) {
              product1a = INTERPOLATE(color2, color5);
              product1a = INTERPOLATE(color2, product1a);
            } else {
              product1a = INTERPOLATE(color5, color6);
            }

            if ((color6 == colorS2) || (color2 == colorA1)) {
              product2b = INTERPOLATE(color2, color3);
              product2b = INTERPOLATE(color2, product2b);
            } else {
              product2b = INTERPOLATE(color2, color3);
            }
          }
          else
          {
            product2b = product1a = INTERPOLATE(color2, color6);
            product2b = Q_INTERPOLATE(color3, color3, color3, product2b);
            product1a = Q_INTERPOLATE(color5, color5, color5, product1a);

            product2a = product1b = INTERPOLATE(color5, color3);
            product2a = Q_INTERPOLATE(color2, color2, color2, product2a);
            product1b = Q_INTERPOLATE(color6, color6, color6, product1b);
          }
        }
        else /*if (color5 == color3) */
        {
          if (color2 != color6)
          {
            product2b = product1a = color5;

            if ((colorB1 == color5) || (color3 == colorS1)) {
              product1b = INTERPOLATE(color5, color6);
              product1b = INTERPOLATE(color5, product1b);
            } else {
              product1b = INTERPOLATE(color5, color6);
            }

            if ((color3 == colorA2) || (color4 == color5)) {
              product2a = INTERPOLATE(color5, color2);
              product2a = INTERPOLATE(color5, product2a);
            } else {
              product2a = INTERPOLATE(color2, color3);
            }

          }
          else /* if (color2 != color6) */
          {
            register int r = 0;

            r += GetResult(color6, color5, color1, colorA1);
            r += GetResult(color6, color5, color4, colorB1);
            r += GetResult(color6, color5, colorA2, colorS1);
            r += GetResult(color6, color5, colorB2, colorS2);

            if (r > 0) {
              product1b = product2a = color2;
              product1a = product2b = INTERPOLATE(color5, color6);
            } else if (r < 0) {
              product2b = product1a = color5;
              product1b = product2a = INTERPOLATE(color5, color6);
            } else {
              product2b = product1a = color5;
              product1b = product2a = color2;
            }
          }
	}

	*dP = (scaler_data_type)product1a;
	*(dP+nextDstLine) = (scaler_data_type)product2a;
	dP++;
	*dP = (scaler_data_type)product1b;
	*(dP+nextDstLine) = (scaler_data_type)product2b;
	dP++;

	bP++;
      }

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }
  }
}

void 
FUNCTION( scaler_2xSaI )( const libspectrum_byte *srcPtr,
			  libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			  libspectrum_dword dstPitch,
			  int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    libspectrum_dword nextlineSrc = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      size_t i;
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( i = 0; i < width; ++i ) {

	register libspectrum_dword colorA, colorB;
	libspectrum_dword colorC, colorD, colorE, colorF, colorG, colorH,
	  colorI, colorJ, colorK, colorL, colorM, colorN, colorO;
	libspectrum_dword product, product1, product2;

/*---------------------------------------
   Map of the pixels:                    I|E F|J
                                         G|A B|K
                                         H|C D|L
                                         M|N O|P

   Note that P does not contribute to the algorithm
*/
	colorI = *(bP - nextlineSrc - 1);
	colorE = *(bP - nextlineSrc);
	colorF = *(bP - nextlineSrc + 1);
	colorJ = *(bP - nextlineSrc + 2);

	colorG = *(bP - 1);
	colorA = *(bP);
	colorB = *(bP + 1);
	colorK = *(bP + 2);

	colorH = *(bP + nextlineSrc - 1);
	colorC = *(bP + nextlineSrc);
	colorD = *(bP + nextlineSrc + 1);
	colorL = *(bP + nextlineSrc + 2);

	colorM = *(bP + 2 * nextlineSrc - 1);
	colorN = *(bP + 2 * nextlineSrc);
	colorO = *(bP + 2 * nextlineSrc + 1);

	if ((colorA == colorD) && (colorB != colorC)) {
	  if (((colorA == colorE) && (colorB == colorL)) ||
              ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))) {
	    product = colorA;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorA == colorG) && (colorC == colorO)) ||
              ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))) {
	    product1 = colorA;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorA;
	} else if ((colorB == colorC) && (colorA != colorD)) {
	  if (((colorB == colorF) && (colorA == colorH)) ||
              ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorC == colorH) && (colorA == colorF)) ||
              ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorB;
	} else if ((colorA == colorD) && (colorB == colorC)) {
	  if (colorA == colorB) {
	    product = colorA;
	    product1 = colorA;
	    product2 = colorA;
	  } else {
	    register int r = 0;

	    product1 = INTERPOLATE(colorA, colorC);
	    product = INTERPOLATE(colorA, colorB);

	    r += GetResult(colorA, colorB, colorG, colorE);
	    r -= GetResult(colorB, colorA, colorK, colorF);
	    r -= GetResult(colorB, colorA, colorH, colorN);
	    r += GetResult(colorA, colorB, colorL, colorO);

	    if (r > 0)
	      product2 = colorA;
	    else if (r < 0)
	      product2 = colorB;
	    else {
	      product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);
	    }
	  }
	} else {
	  product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);

	  if ((colorA == colorC) && (colorA == colorF)
	      && (colorB != colorE) && (colorB == colorJ)) {
	    product = colorA;
	  } else if ((colorB == colorE) && (colorB == colorD)
		     && (colorA != colorF) && (colorA == colorI)) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if ((colorA == colorB) && (colorA == colorH)
	      && (colorG != colorC) && (colorC == colorM)) {
	    product1 = colorA;
	  } else if ((colorC == colorG) && (colorC == colorD)
		     && (colorA != colorH) && (colorA == colorI)) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	}

	*dP = (scaler_data_type)colorA;
	*(dP+nextDstLine) = (scaler_data_type)product1;
	dP++;
	*dP = (scaler_data_type)product;
	*(dP+nextDstLine) = (scaler_data_type)product2;
	dP++;

	bP++;
      }

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }
  }
}

void 
FUNCTION( scaler_AdvMame2x )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*) srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  scaler_data_type /* A, */ B, C;
  scaler_data_type D, E, F;
  scaler_data_type /* G, */ H, I;

  while (height--) {
    int i;

    /* B = *(p - 1 - nextlineSrc); */
    E = *(p - 1);
    /* H = *(p - 1 + nextlineSrc); */
    C = *(p - nextlineSrc);
    F = *(p);
    I = *(p + nextlineSrc);

    for (i = 0; i < width; ++i) {
      p++;
      /* A = B; */ B = C; C = *(p - nextlineSrc);
      D = E; E = F; F = *(p);
      /* G = H; */ H = I; I = *(p + nextlineSrc);

      *(q) = D == B && B != F && D != H ? D : E;
      *(q + 1) = B == F && B != D && F != H ? F : E;
      *(q + nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + nextlineDst + 1) = H == F && D != H && B != F ? F : E;
      q += 2;
    }
    p += nextlineSrc - width;
    q += (nextlineDst - width) << 1;
  }
}

void 
FUNCTION( scaler_AdvMame3x )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*) srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  scaler_data_type /* A, */ B, C;
  scaler_data_type D, E, F;
  scaler_data_type /* G, */ H, I;

  while (height--) {
    int i;

    /* B = *(p - 1 - nextlineSrc); */
    E = *(p - 1);
    /* H = *(p - 1 + nextlineSrc); */
    C = *(p - nextlineSrc);
    F = *(p);
    I = *(p + nextlineSrc);

    for (i = 0; i < width; ++i) {
      p++;
      /* A = B; */ B = C; C = *(p - nextlineSrc);
      D = E; E = F; F = *(p);
      /* G = H; */ H = I; I = *(p + nextlineSrc);

      *(q) = D == B && B != F && D != H ? D : E;
      *(q + 1) = E;
      *(q + 2) = B == F && B != D && F != H ? F : E;
      *(q + nextlineDst) = E;
      *(q + nextlineDst + 1) = E;
      *(q + nextlineDst + 2) = E;
      *(q + 2 * nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + 2 * nextlineDst + 1) = E;
      *(q + 2 * nextlineDst + 2) = H == F && D != H && B != F ? F : E;
      q += 3;
    }
    p += nextlineSrc - width;
    q += (nextlineDst - width) * 3;
  }
}

void 
FUNCTION( scaler_Half )( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			 libspectrum_dword dstPitch,
			 int width, int height )
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        scaler_data_type color1 = *(((scaler_data_type*) srcPtr) + i);
        scaler_data_type color2 = *(((scaler_data_type*) srcPtr) + i + 1);

        *r = INTERPOLATE(color1, color2);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

void 
FUNCTION( scaler_HalfSkip )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        *r = *(((const scaler_data_type*) srcPtr) + i + 1);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

void 
FUNCTION( scaler_Normal1x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  while( height-- ) {
    memcpy( dstPtr, srcPtr, SCALER_DATA_SIZE * width );
    srcPtr += srcPitch;
    dstPtr += dstPitch;
  }
}

void 
FUNCTION( scaler_Normal2x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  const scaler_data_type *s;
  scaler_data_type i, *d, *d2;

  while( height-- ) {

    for( i = 0, s = (const scaler_data_type*)srcPtr,
	   d = (scaler_data_type*)dstPtr,
	   d2 = (scaler_data_type*)(dstPtr + dstPitch);
	 i < width;
	 i++ ) {
      *d++ = *d2++ = *s; *d++ = *d2++ = *s++;
    }

    srcPtr += srcPitch;
    dstPtr += dstPitch << 1;
  }
}

void 
FUNCTION( scaler_Normal3x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  libspectrum_byte *r;
  libspectrum_dword dstPitch2 = dstPitch * 2;
  libspectrum_dword dstPitch3 = dstPitch * 3;

  while (height--) {
    int i;
    r = dstPtr;
    for (i = 0; i < width; ++i, r += 3 * SCALER_DATA_SIZE ) {
      scaler_data_type color = *(((const scaler_data_type*) srcPtr) + i);

      *(scaler_data_type*)( r +                    0             ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE             ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE             ) = color;
      *(scaler_data_type*)( r +                    0 + dstPitch  ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch  ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch  ) = color;
      *(scaler_data_type*)( r +                    0 + dstPitch2 ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch2 ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch2 ) = color;
    }
    srcPtr += srcPitch;
    dstPtr += dstPitch3;
  }
}

void 
FUNCTION( scaler_Timex1_5x )( const libspectrum_byte *srcPtr,
           libspectrum_dword srcPitch,
           libspectrum_byte *dstPtr,
           libspectrum_dword dstPitch,
           int width, int height )
{
  libspectrum_byte *r;
  libspectrum_dword dstPitch2 = dstPitch * 2;
  libspectrum_dword dstPitch3 = dstPitch * 3;

  while (height--) {
    int i;
    r = dstPtr;
    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, r += 3 * SCALER_DATA_SIZE ) {
        /* Interpolate a new pixel inbetween the source pixels */
        scaler_data_type color1 = *(((const scaler_data_type*) srcPtr) + i);
        scaler_data_type color2 = *(((const scaler_data_type*) srcPtr) + i + 1);
        scaler_data_type color3 = INTERPOLATE(color1, color2);

        *(scaler_data_type*)( r +                    0             ) = color1;
        *(scaler_data_type*)( r +     SCALER_DATA_SIZE             ) = color3;
        *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE             ) = color2;
        *(scaler_data_type*)( r +                    0 + dstPitch  ) = color1;
        *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch  ) = color3;
        *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch  ) = color2;
        *(scaler_data_type*)( r +                    0 + dstPitch2 ) = color1;
        *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch2 ) = color3;
        *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch2 ) = color2;
      }
      dstPtr += dstPitch3;
    }
    srcPtr += srcPitch;
  }
}

void
FUNCTION( scaler_TV2x )( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			 libspectrum_dword dstPitch,
			 int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*)dstPtr;

  while(height--) {
    for (i = 0, j = 0; i < width; ++i, j += 2) {
      scaler_data_type p1 = *(p + i);
      scaler_data_type pi;

      pi  = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
      pi |= (((p1 & greenMask  ) * 7) >> 3) & greenMask;

      *(q + j) = p1;
      *(q + j + 1) = p1;
      *(q + j + nextlineDst) = pi;
      *(q + j + nextlineDst + 1) = pi;
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

void
FUNCTION( scaler_TV3x )( const libspectrum_byte *srcPtr,
                         libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
                         libspectrum_dword dstPitch,
                         int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*)dstPtr;

  while(height--) {
    for (i = 0, j = 0; i < width; ++i, j += 3) {
      scaler_data_type p1 = *(p + i);
      scaler_data_type pi;

      pi  = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
      pi |= (((p1 & greenMask  ) * 7) >> 3) & greenMask;

      *(q + j) = p1;
      *(q + j + 1) = p1;
      *(q + j + 2) = p1;

      *(q + j + nextlineDst) = p1;
      *(q + j + nextlineDst + 1) = p1;
      *(q + j + nextlineDst + 2) = p1;

      *(q + j + (nextlineDst << 1)) = pi;
      *(q + j + (nextlineDst << 1) + 1) = pi;
      *(q + j + (nextlineDst << 1) + 2) = pi;
    }
    p += nextlineSrc;
    q += nextlineDst * 3;
  }
}

void
FUNCTION( scaler_TimexTV )( const libspectrum_byte *srcPtr,
			    libspectrum_dword srcPitch,
			    libspectrum_byte *dstPtr,
			    libspectrum_dword dstPitch,
			    int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  while(height--) {
    if( ( height & 1 ) == 0 ) {
      for (i = 0, j = 0; i < width; ++i, j++ ) {
        scaler_data_type p1 = *(p + i);
        scaler_data_type pi;

	pi  = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
	pi |= (((p1 & greenMask  ) * 7) >> 3) & greenMask;

        *(q + j) = p1;
        *(q + j + nextlineDst) = pi;
      }
      q += nextlineDst << 1;
    }
    p += nextlineSrc;
  }
}

static inline scaler_data_type DOT_16(scaler_data_type c, int j, int i) {
  return c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3)));
}

void
FUNCTION( scaler_DotMatrix )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  int i, j, ii, jj;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  for (j = 0, jj = 0; j < height; ++j, jj += 2) {
    for (i = 0, ii = 0; i < width; ++i, ii += 2) {
      scaler_data_type c = *(p + i);
      *(q + ii) = DOT_16(c, jj, ii);
      *(q + ii + 1) = DOT_16(c, jj, ii + 1);
      *(q + ii + nextlineDst) = DOT_16(c, jj + 1, ii);
      *(q + ii + nextlineDst + 1) = DOT_16(c, jj + 1, ii + 1);
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

/*
    Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
    U  = -0.16874 * R - 0.33126 * G + 0.50000 * B  (+ 128)
    V  =  0.50000 * R - 0.41869 * G - 0.08131 * B  (+ 128)
*/

#define RGB_TO_Y(r, g, b) ( ( 2449L * r + 4809L * g + 934L * b + 1024 ) >> 11 )
#define RGB_TO_U(r, g, b) ( ( 4096L * b - 1383L * r - 2713L * g + 1024 ) >> 11 )
#define RGB_TO_V(r, g, b) ( ( 4096L * r - 3430L * g - 666L * b +  1024 ) >> 11 )

/*
    R = Y + 1.402 (V-128)
    G = Y - 0.34414 (U-128) - 0.71414 (V-128)
    B = Y + 1.772 (U-128)
*/

#define YUV_TO_R(y, u, v) ( MIN( ABS( ( 8192L * y              + 11485L * v + 16384 ) >> 15 ), 255 ) )
#define YUV_TO_G(y, u, v) ( MIN( ABS( ( 8192L * y - 2819L  * u -  5850L * v + 16384 ) >> 15 ), 255 ) )
#define YUV_TO_B(y, u, v) ( MIN( ABS( ( 8192L * y + 14516L * u              + 16384 ) >> 15 ), 255 ) )

#define RGB_TO_PIXEL_555(r,g,b) \
        (((r * 125) >> 10) + (((g * 125) >> 5) & greenMask) + \
                ((b * 125) & blueMask))

#define RGB_TO_PIXEL_565(r,g,b) \
        (((r * 125) >> 10) + (((g * 253) >> 5) & greenMask) + \
                ((b * 249) & blueMask))

#define R_TO_R(r) \
        ( ( ( (r) & redMask ) * 8424 ) >> 10 )

#define G_TO_G(g) \
      ( green6bit ? \
        ( ( ( (g) & greenMask ) >> 5 ) * 4145 ) >> 10 : \
        ( ( ( (g) & greenMask ) >> 5 ) * 8424 ) >> 10 )

#define B_TO_B(b) \
      ( green6bit ? \
        ( ( ( (b) & blueMask ) >> 11 ) * 8424 ) >> 10 : \
        ( ( ( (b) & blueMask ) >> 10 ) * 8424 ) >> 10 )

void
FUNCTION( scaler_PalTV )( const libspectrum_byte *srcPtr,
                              libspectrum_dword srcPitch,
                              libspectrum_byte *dstPtr,
                              libspectrum_dword dstPitch,
                              int width, int height )
{
/*
   1.a. RGB => 255,255,255 RGB
   1.b. RGB => YUV
   2. 422 interstricial color subsampling 
   3.a. YUV => RGB
   3.b  255,255,255 RGB => RGB
*/
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p, *p0 = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q, *q0 = (scaler_data_type *)dstPtr;
  
  libspectrum_byte  r0, g0, b0,
                    r1, g1, b1,
                    r2, g2, b2,
                    r3, g3, b3;
  libspectrum_signed_word y1, y2, u1, u2, v1, v2;

/*
 422 cosited
    # + # +             + only Y
                        # Y and Cb Cr
    # + # +
    
    abcd...    =>  1/2a + a + 1/2b / 2; ...; 1/2a + b + 1/2c; ... ; ...
    always 3 sample/proc

*/
  for( j = height; j; j-- ) {
    p = p0 - 1; q = q0;
#if SCALER_DATA_SIZE == 2
    /* 1.a. RGB => RGB */
    r2 = R_TO_R( *p );
    g2 = G_TO_G( *p );
    b2 = B_TO_B( *p );
    p++;
    r0 = R_TO_R( *p );
    g0 = G_TO_G( *p );
    b0 = B_TO_B( *p );
    p++;
    r1 = R_TO_R( *p );
    g1 = G_TO_G( *p );
    b1 = B_TO_B( *p );
    p++;
#else
    r2 = (*p & redMask);
    g2 = (*p & greenMask) >> 8;
    b2 = (*p & blueMask) >> 16;
    p++;
    r0 = (*p & redMask);
    g0 = (*p & greenMask) >> 8;
    b0 = (*p & blueMask) >> 16;
    p++;
    r1 = (*p & redMask);
    g1 = (*p & greenMask) >> 8;
    b1 = (*p & blueMask) >> 16;
    p++;
#endif
    u1 = ( RGB_TO_U( r2, g2, b2 ) + 2 * RGB_TO_U( r0, g0, b0 ) +
            RGB_TO_U( r1, g1, b1 ) ) >> 2;
    v1 = ( RGB_TO_V( r2, g2, b2 ) + 2 * RGB_TO_V( r0, g0, b0 ) +
            RGB_TO_V( r1, g1, b1 ) ) >> 2;
    for( i = width; i; i -= 2 ) {
#if SCALER_DATA_SIZE == 2
      /* 1.a. RGB => RGB */
      r2 = R_TO_R( *p );
      g2 = G_TO_G( *p );
      b2 = B_TO_B( *p );
      p++;
      r3 = R_TO_R( *p );
      g3 = G_TO_G( *p );
      b3 = B_TO_B( *p );
      p++;
#else
      r2 = (*p & redMask);
      g2 = (*p & greenMask) >> 8;
      b2 = (*p & blueMask) >> 16;
      p++;
      r3 = (*p & redMask);
      g3 = (*p & greenMask) >> 8;
      b3 = (*p & blueMask) >> 16;
      p++;
#endif
/* 1.b. RGB => YUV && 2. YUV subsampling */
      y1 = RGB_TO_Y( r0, g0, b0 );
      y2 = RGB_TO_Y( r1, g1, b1 );

      u2 = ( RGB_TO_U( r1, g1, b1 ) + 2 * RGB_TO_U( r2, g2, b2 ) +
            RGB_TO_U( r3, g3, b3 ) ) >> 2;
      v2 = ( RGB_TO_V( r1, g1, b1 ) + 2 * RGB_TO_V( r2, g2, b2 ) +
            RGB_TO_V( r3, g3, b3 ) ) >> 2;
/* 3.a. YUV => RGB  */
      r0 = YUV_TO_R(y1, u1, v1);
      g0 = YUV_TO_G(y1, u1, v1);
      b0 = YUV_TO_B(y1, u1, v1);
      
      u1 = (u1 + u2) >> 1;
      v1 = (v1 + v2) >> 1;

      r1 = YUV_TO_R(y2, u1, v1);
      g1 = YUV_TO_G(y2, u1, v1);
      b1 = YUV_TO_B(y2, u1, v1);
#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q++ = RGB_TO_PIXEL_565( r0, g0, b0 );
        *q++ = RGB_TO_PIXEL_565( r1, g1, b1 );
      } else {
        *q++ = RGB_TO_PIXEL_555( r0, g0, b0 );
        *q++ = RGB_TO_PIXEL_555( r1, g1, b1 );
      }
#else
      *q++ = r0 + (g0 << 8) + (b0 << 16);
      *q++ = r1 + (g1 << 8) + (b1 << 16);
#endif
      u1 = u2; v1 = v2;
      r0 = r2; g0 = g2; b0 = b2;
      r1 = r3; g1 = g3; b1 = b3;
    }
    p0 += nextlineSrc;
    q0 += nextlineDst;
  }
}

void
FUNCTION( scaler_PalTV2x )( const libspectrum_byte *srcPtr,
                              libspectrum_dword srcPitch,
                              libspectrum_byte *dstPtr,
                              libspectrum_dword dstPitch,
                              int width, int height )
{
/*
   1.a. RGB => 255,255,255 RGB
   1.b. RGB => YUV
   2. 4:2:2 cosited color subsampling 
   3.a. YUV => RGB
   3.b  255,255,255 RGB => RGB
*/
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p, *p0 = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q, *q0 = (scaler_data_type *)dstPtr;
  
  libspectrum_byte  r0, g0, b0,
                    r1, g1, b1,
                    rx, gx, bx;
  libspectrum_signed_dword y1, y2, u1, v1, u2, v2;

/*
 422 cosited
    # + # +             + only Y
                        # Y and Cb Cr
    # + # +
    
    abcd...    =>  1/2a + a + 1/2b / 2; ...; 1/2a + b + 1/2c; ... ; ...
    always 3 sample/proc

*/
  for( j = height; j; j-- ) {
    p = p0 - 1; q = q0;
#if SCALER_DATA_SIZE == 2
    r0 = R_TO_R( *p );
    g0 = G_TO_G( *p );
    b0 = B_TO_B( *p );
    p++;
    r1 = R_TO_R( *p );
    g1 = G_TO_G( *p );
    b1 = B_TO_B( *p );
#else
    r0 = *p & redMask;
    g0 = (*p & greenMask) >> 8;
    b0 = (*p & blueMask) >> 16;
    p++;
    r1 = *(p) & redMask;
    g1 = (*(p) & greenMask) >> 8;
    b1 = (*(p) & blueMask) >> 16;
#endif
    y1 = RGB_TO_Y( r1, g1, b1 );
    u1 = ( RGB_TO_U( r0, g0, b0 ) + 3 * RGB_TO_U( r1, g1, b1 ) ) >> 2;
    v1 = ( RGB_TO_V( r0, g0, b0 ) + 3 * RGB_TO_V( r1, g1, b1 ) ) >> 2;
    for( i = width; i; i-- ) {
      p++;      /* next point */
#if SCALER_DATA_SIZE == 2
      /* 1.a. RGB => RGB */
      r0 = R_TO_R( *p );
      g0 = G_TO_G( *p );
      b0 = B_TO_B( *p );
#else
      r0 = (*p & redMask);
      g0 = (*p & greenMask) >> 8;
      b0 = (*p & blueMask) >> 16;
#endif
/* 1.b. RGB => YUV && 2. YUV subsampling */
      y2 = RGB_TO_Y( r0, g0, b0 );
      u2 = ( RGB_TO_U( r1, g1, b1 ) + 3 * RGB_TO_U( r0, g0, b0 ) ) >> 2;
      v2 = ( RGB_TO_V( r1, g1, b1 ) + 3 * RGB_TO_V( r0, g0, b0 ) ) >> 2;

/* 3.a. YUV => RGB  */
      rx = YUV_TO_R( y1, u1, v1 );      /* [x0][  ]*/
      gx = YUV_TO_G( y1, u1, v1 );
      bx = YUV_TO_B( y1, u1, v1 );

      u1 = ( u1 + u2 ) >> 1;
      v1 = ( v1 + v2 ) >> 1;

      r1 = YUV_TO_R( y1, u1, v1 );
      g1 = YUV_TO_G( y1, u1, v1 );
      b1 = YUV_TO_B( y1, u1, v1 );

#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q = RGB_TO_PIXEL_565( rx, gx, bx );
      } else {
        *q = RGB_TO_PIXEL_555( rx, gx, bx );
      }
#else
      *q = rx + ( gx << 8 ) + ( bx << 16 );
#endif

      if( settings_current.pal_tv2x )
        *(q + nextlineDst) =
            ((((*q & redblueMask) * 7) >> 3) & redblueMask) |
                ((((*q & greenMask  ) * 7) >> 3) & greenMask);
      else
        *(q + nextlineDst) = *q;

      q++;
#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q = RGB_TO_PIXEL_565( r1, g1, b1 );
      } else {
        *q = RGB_TO_PIXEL_555( r1, g1, b1 );
      }
#else
      *q = r1 + ( g1 << 8 ) + ( b1 << 16 );
#endif
      if( settings_current.pal_tv2x )
        *(q + nextlineDst) =
            ((((*q & redblueMask) * 7) >> 3) & redblueMask) |
                ((((*q & greenMask  ) * 7) >> 3) & greenMask);
      else
        *(q + nextlineDst) = *q;

      q++;
      y1 = y2; u1 = u2; v1 = v2;        /* save for next point */
      r1 = r0; g1 = g0; b1 = b0;
    }
    p0 += nextlineSrc;
    q0 += nextlineDst << 1;
  }
}

void
FUNCTION( scaler_PalTV3x )( const libspectrum_byte *srcPtr,
                              libspectrum_dword srcPitch,
                              libspectrum_byte *dstPtr,
                              libspectrum_dword dstPitch,
                              int width, int height )
{
/*
   1.a. RGB => 255,255,255 RGB
   1.b. RGB => YUV
   2. 4:2:2 cosited color subsampling 
   3.a. YUV => RGB
   3.b  255,255,255 RGB => RGB
*/
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p, *p0 = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q, *q0 = (scaler_data_type *)dstPtr;
  
  libspectrum_byte  r0, g0, b0,
                    r1, g1, b1,
                    r2, g2, b2,
                    rx, gx, bx;
  libspectrum_signed_dword y1, y2, u1, v1, u2, v2;

/*
 422 cosited
    # + # +             + only Y
                        # Y and Cb Cr
    # + # +
    
    abcd...    =>  1/2a + a + 1/2b / 2; ...; 1/2a + b + 1/2c; ... ; ...
    always 3 sample/proc

*/
  for( j = height; j; j-- ) {
    p = p0 - 1; q = q0;
#if SCALER_DATA_SIZE == 2
    r0 = R_TO_R( *p );
    g0 = G_TO_G( *p );
    b0 = B_TO_B( *p );
    p++;
    r1 = R_TO_R( *p );
    g1 = G_TO_G( *p );
    b1 = B_TO_B( *p );
#else
    r0 = *p & redMask;
    g0 = (*p & greenMask) >> 8;
    b0 = (*p & blueMask) >> 16;
    p++;        /* next point */
    r1 = *(p) & redMask;
    g1 = (*(p) & greenMask) >> 8;
    b1 = (*(p) & blueMask) >> 16;
#endif
    y1 = RGB_TO_Y( r1, g1, b1 );
    u1 = ( RGB_TO_U( r0, g0, b0 ) + 3 * RGB_TO_U( r1, g1, b1 ) ) >> 2;
    v1 = ( RGB_TO_V( r0, g0, b0 ) + 3 * RGB_TO_V( r1, g1, b1 ) ) >> 2;
    for( i = width; i; i-- ) {
      p++;
#if SCALER_DATA_SIZE == 2
      /* 1.a. RGB => RGB */
      r0 = R_TO_R( *p );
      g0 = G_TO_G( *p );
      b0 = B_TO_B( *p );
#else
      r0 = (*p & redMask);
      g0 = (*p & greenMask) >> 8;
      b0 = (*p & blueMask) >> 16;
#endif
/* 1.b. RGB => YUV && 2. YUV subsampling */
      y2 = RGB_TO_Y( r0, g0, b0 );
      u2 = ( RGB_TO_U( r1, g1, b1 ) + 3 * RGB_TO_U( r0, g0, b0 ) ) >> 2;
      v2 = ( RGB_TO_V( r1, g1, b1 ) + 3 * RGB_TO_V( r0, g0, b0 ) ) >> 2;

/* 3.a. YUV => RGB  */
      rx = YUV_TO_R( y1, u1, v1 );      /* [x0][  ]*/
      gx = YUV_TO_G( y1, u1, v1 );
      bx = YUV_TO_B( y1, u1, v1 );

      u1 = ( u1 + u2 ) >> 1;
      v1 = ( v1 + v2 ) >> 1;

      r1 = YUV_TO_R( y1, u1, v1 );
      g1 = YUV_TO_G( y1, u1, v1 );
      b1 = YUV_TO_B( y1, u1, v1 );

/*
    ab  => EFG         
    ab     EFG
           efg
*/
      r2 = ((int)rx + r1) >> 1;                 /* F */
      g2 = ((int)gx + g1) >> 1;
      b2 = ((int)bx + b1) >> 1;

#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q = RGB_TO_PIXEL_565( rx, gx, bx);
      } else {
        *q = RGB_TO_PIXEL_555( rx, gx, bx);
      }
#else
      *q = rx + ( gx << 8 ) + ( bx << 16 );     /* E, E, e */
#endif
      *(q + nextlineDst) = *q;

      if( settings_current.pal_tv2x )
        *(q + (nextlineDst << 1)) =
            ((((*q & redblueMask) * 7) >> 3) & redblueMask) |
                ((((*q & greenMask  ) * 7) >> 3) & greenMask);
      else
        *(q + (nextlineDst << 1)) = *q;

      q++;
#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q = RGB_TO_PIXEL_565( r2, g2, b2 );
      } else {
        *q = RGB_TO_PIXEL_555( r2, g2, b2 );
      }
#else
      *q = r2 + ( g2 << 8 ) + ( b2 << 16 );     /* F, F, f*/
#endif
      *(q + nextlineDst) = *q;

      if( settings_current.pal_tv2x )
        *(q + (nextlineDst << 1)) =
            ((((*q & redblueMask) * 7) >> 3) & redblueMask) |
                ((((*q & greenMask  ) * 7) >> 3) & greenMask);
      else
        *(q + (nextlineDst << 1)) = *q;

      q++;
#if SCALER_DATA_SIZE == 2
/* 3.b. RGB => RGB */
      if( green6bit ) {
        *q = RGB_TO_PIXEL_565( r1, g1, b1 );
      } else {
        *q = RGB_TO_PIXEL_555( r1, g1, b1 );
      }
#else
      *q = r1 + ( g1 << 8 ) + ( b1 << 16 );     /* G, G, g*/
#endif
      *(q + nextlineDst) = *q;
      if( settings_current.pal_tv2x )
        *(q + (nextlineDst << 1)) =
            ((((*q & redblueMask) * 7) >> 3) & redblueMask) |
                ((((*q & greenMask  ) * 7) >> 3) & greenMask);
      else
        *(q + (nextlineDst << 1)) = *q;

      q++;
      y1 = y2; u1 = u2; v1 = v2;        /* save for next point */
      r1 = r0; g1 = g0; b1 = b0;
    }
    p0 += nextlineSrc;
    q0 += (nextlineDst << 1) + nextlineDst;
  }
}

#define prevline (-nextlineSrc)
#define nextline nextlineSrc
#define MOVE_B_TO_A(A,B) \
		w[A] = w[B]; y[A] = y[B]; u[A] = u[B]; v[A] = v[B];
#define MOVE_P_RIGHT \
	MOVE_B_TO_A(1,2) \
	MOVE_B_TO_A(4,5) \
	MOVE_B_TO_A(7,8) \
	MOVE_B_TO_A(2,3) \
	MOVE_B_TO_A(5,6) \
	MOVE_B_TO_A(8,9)

void
FUNCTION( scaler_HQ2x ) ( const libspectrum_byte *srcPtr,
                          libspectrum_dword srcPitch,
                          libspectrum_byte *dstPtr,
                          libspectrum_dword dstPitch,
                          int width, int height )
{
  int i, j, k, pattern;
  int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p, *p0 = (const scaler_data_type *)srcPtr;
  int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q, *q1, *qN, *qN1, *q0 = (scaler_data_type *)dstPtr;
  libspectrum_qword w[10];
  
  libspectrum_byte  r, g, b;
  libspectrum_signed_dword y[10], u[10], v[10];

  /*   +----+----+----+
       |    |    |    |
       | w1 | w2 | w3 |
       +----+----+----+
       |    |    |    |
       | w4 | w5 | w6 |
       +----+----+----+
       |    |    |    |
       | w7 | w8 | w9 |
       +----+----+----+ */
  for( j = 0; j < height; j++ ) {
    p = p0;
    q = q0; q1 = q + 1;
    qN = q + nextlineDst; qN1 = qN + 1;
    w[2] = *(p + prevline);
    w[5] = *p;
    w[8] = *(p + nextline);
    w[1] = *(p + prevline - 1);
    w[4] = *(p - 1);
    w[7] = *(p + nextline - 1);
    w[3] = *(p + prevline + 1);
    w[6] = *(p + 1);
    w[9] = *(p + nextline + 1);
    for( k = 1; k <= 9; k++ ) {
#if SCALER_DATA_SIZE == 2
      r = R_TO_R( w[k] );
      g = G_TO_G( w[k] );
      b = B_TO_B( w[k] );
#else
      r =  w[k] & redMask;
      g = (w[k] & greenMask) >> 8;
      b = (w[k] & blueMask) >> 16;
#endif
      y[k] = RGB_TO_Y( r, g, b );
      u[k] = RGB_TO_U( r, g, b );
      v[k] = RGB_TO_V( r, g, b );
    }

    for( i = 0; i < width; i++ ) {
      pattern = 0;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[1], u[1], v[1] ) ) pattern |= 0x01;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[2], u[2], v[2] ) ) pattern |= 0x02;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[3], u[3], v[3] ) ) pattern |= 0x04;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[4], u[4], v[4] ) ) pattern |= 0x08;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[6], u[6], v[6] ) ) pattern |= 0x10;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[7], u[7], v[7] ) ) pattern |= 0x20;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[8], u[8], v[8] ) ) pattern |= 0x40;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[9], u[9], v[9] ) ) pattern |= 0x80;

#include "scaler_hq2x.c"

      p++;
      q  += 2; q1  += 2;
      qN += 2; qN1 += 2;
      MOVE_P_RIGHT
      w[3] = *(p + prevline + 1);
      w[6] = *(p + 1);
      w[9] = *(p + nextline + 1);
      for( k = 3; k <= 9; k += 3 ) {
#if SCALER_DATA_SIZE == 2
        r = R_TO_R( w[k] );
        g = G_TO_G( w[k] );
        b = B_TO_B( w[k] );
#else
        r =   w[k] & redMask;
        g = ( w[k] & greenMask ) >> 8;
        b = ( w[k] & blueMask  ) >> 16;
#endif
        y[k] = RGB_TO_Y( r, g, b );
        u[k] = RGB_TO_U( r, g, b );
        v[k] = RGB_TO_V( r, g, b );
      }
    }
    p0 += nextlineSrc;
    q0 += nextlineDst << 1;
  }
}

void
FUNCTION( scaler_HQ3x ) ( const libspectrum_byte *srcPtr,
                          libspectrum_dword srcPitch,
                          libspectrum_byte *dstPtr,
                          libspectrum_dword dstPitch,
                          int width, int height )
{
  int i, j, k, pattern;
  int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p, *p0 = (const scaler_data_type *)srcPtr;
  int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q, *qN, *qNN, *q1, *qN1, *qNN1, *q2, *qN2, *qNN2, 
		   *q0 = (scaler_data_type *)dstPtr;
  libspectrum_qword w[10];
  
  libspectrum_byte  r, g, b;
  libspectrum_signed_dword y[10], u[10], v[10];

  /*   +----+----+----+
       |    |    |    |
       | w1 | w2 | w3 |
       +----+----+----+
       |    |    |    |
       | w4 | w5 | w6 |
       +----+----+----+
       |    |    |    |
       | w7 | w8 | w9 |
       +----+----+----+ */
  for( j = 0; j < height; j++ ) {
    if( j == height - 1 ) nextline = 0;

    p = p0;
    q = q0;
    q1 = q + 1; q2 = q + 2;
    qN = q + nextlineDst; qN1 = qN + 1; qN2 = qN + 2;
    qNN = qN + nextlineDst;  qNN1 = qNN + 1; qNN2 = qNN + 2;

    w[2] = *(p + prevline);
    w[5] = *p;
    w[8] = *(p + nextline);
    w[1] = *(p + prevline - 1);
    w[4] = *(p - 1);
    w[7] = *(p + nextline - 1);
    w[3] = *(p + prevline + 1);
    w[6] = *(p + 1);
    w[9] = *(p + nextline + 1);
    for( k = 1; k <= 9; k++ ) {
#if SCALER_DATA_SIZE == 2
      r = R_TO_R( w[k] );
      g = G_TO_G( w[k] );
      b = B_TO_B( w[k] );
#else
      r =  w[k] & redMask;
      g = (w[k] & greenMask) >> 8;
      b = (w[k] & blueMask) >> 16;
#endif
      y[k] = RGB_TO_Y( r, g, b );
      u[k] = RGB_TO_U( r, g, b );
      v[k] = RGB_TO_V( r, g, b );
    }

    for( i = 0; i < width; i++ ) {
      pattern = 0;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[1], u[1], v[1] ) ) pattern |= 0x01;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[2], u[2], v[2] ) ) pattern |= 0x02;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[3], u[3], v[3] ) ) pattern |= 0x04;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[4], u[4], v[4] ) ) pattern |= 0x08;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[6], u[6], v[6] ) ) pattern |= 0x10;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[7], u[7], v[7] ) ) pattern |= 0x20;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[8], u[8], v[8] ) ) pattern |= 0x40;
      if( HQ_YUVDIFF( y[5], u[5], v[5], y[9], u[9], v[9] ) ) pattern |= 0x80;

#include "scaler_hq3x.c"

      p++;
      q   += 3; q1   += 3; q2   += 3;
      qN  += 3; qN1  += 3; qN2  += 3;
      qNN += 3; qNN1 += 3; qNN2 += 3;
      MOVE_P_RIGHT
      w[3] = *(p + prevline + 1);
      w[6] = *(p + 1);
      w[9] = *(p + nextline + 1);
      for( k = 3; k <= 9; k += 3 ) {
#if SCALER_DATA_SIZE == 2
        r = R_TO_R( w[k] );
        g = G_TO_G( w[k] );
        b = B_TO_B( w[k] );
#else
        r =   w[k] & redMask;
        g = ( w[k] & greenMask ) >> 8;
        b = ( w[k] & blueMask  ) >> 16;
#endif
        y[k] = RGB_TO_Y( r, g, b );
        u[k] = RGB_TO_U( r, g, b );
        v[k] = RGB_TO_V( r, g, b );
      }
    }
    p0 += nextlineSrc;
    q0 += ( nextlineDst << 1 ) + nextlineDst;
  }
}
