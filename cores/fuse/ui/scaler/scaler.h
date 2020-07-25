/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project, Fredrick Meunier and
 *			   Philip Kendall
 *
 * HQ2x and HQ3x scalers taken from HiEnd3D Demos (http://www.hiend3d.com)
 * Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Header$
 */

#ifndef SCALER_H
#define SCALER_H

#include <libspectrum.h>

typedef enum scaler_type {
  SCALER_HALF = 0,
  SCALER_HALFSKIP,
  SCALER_NORMAL,
  SCALER_DOUBLESIZE,
  SCALER_TRIPLESIZE,
  SCALER_2XSAI,
  SCALER_SUPER2XSAI,
  SCALER_SUPEREAGLE,
  SCALER_ADVMAME2X,
  SCALER_ADVMAME3X,
  SCALER_TV2X,
  SCALER_TV3X,
  SCALER_TIMEXTV,
  SCALER_DOTMATRIX,
  SCALER_TIMEX1_5X,
  SCALER_PALTV,
  SCALER_PALTV2X,
  SCALER_PALTV3X,
  SCALER_HQ2X,
  SCALER_HQ3X,

  SCALER_NUM		/* End marker; do not remove */
} scaler_type;

typedef enum scaler_flags_t {
  SCALER_FLAGS_NONE        = 0,
  SCALER_FLAGS_EXPAND      = 1 << 0,
} scaler_flags_t;

typedef void ScalerProc( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch,
			 libspectrum_byte *dstPtr, libspectrum_dword dstPitch,
			 int width, int height );

/* The type of function used to expand the area dirtied by a scaler */
typedef void scaler_expand_fn( int *x, int *y, int *w, int *h,
			       int image_width, int image_height );

extern scaler_type current_scaler;
extern ScalerProc *scaler_proc16, *scaler_proc32;
extern scaler_flags_t scaler_flags;
extern scaler_expand_fn *scaler_expander;
extern int scalers_registered;

typedef int (*scaler_available_fn)( scaler_type scaler );

int scaler_select_id( const char *scaler_mode );
void scaler_register_clear( void );
int scaler_select_scaler( scaler_type scaler );
void scaler_register( scaler_type scaler );
int scaler_is_supported( scaler_type scaler );
const char *scaler_name( scaler_type scaler );
ScalerProc *scaler_get_proc16( scaler_type scaler );
ScalerProc *scaler_get_proc32( scaler_type scaler );
scaler_flags_t scaler_get_flags( scaler_type scaler );
float scaler_get_scaling_factor( scaler_type scaler );
scaler_expand_fn* scaler_get_expander( scaler_type scaler );

int scaler_select_bitformat( libspectrum_dword BitFormat );

#endif
