/* scaler_internals.h: functions internal to the scaler code
   Copyright (c) 2003 Fredrick Meunier, Philip Kendall

   Originally taken from ScummVM - Scumm Interpreter
   Copyright (C) 2001  Ludvig Strigeus
   Copyright (C) 2001/2002 The ScummVM project

   HQ2x and HQ3x scalers taken from HiEnd3D Demos (http://www.hiend3d.com)
   Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )

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

#ifndef FUSE_SCALER_INTERNALS_H
#define FUSE_SCALER_INTERNALS_H

#define DECLARE_SCALER( name ) \
         extern void scaler_##name##_16( const libspectrum_byte *srcPtr, \
					 libspectrum_dword srcPitch, \
					 libspectrum_byte *dstPtr, \
					 libspectrum_dword dstPitch, \
					 int width, int height ); \
         extern void scaler_##name##_32( const libspectrum_byte *srcPtr, \
					 libspectrum_dword srcPitch, \
					 libspectrum_byte *dstPtr, \
					 libspectrum_dword dstPitch, \
					 int width, int height );

DECLARE_SCALER(2xSaI);
DECLARE_SCALER(Super2xSaI);
DECLARE_SCALER(SuperEagle);
DECLARE_SCALER(AdvMame2x);
DECLARE_SCALER(AdvMame3x);
DECLARE_SCALER(Half);
DECLARE_SCALER(HalfSkip);
DECLARE_SCALER(Normal1x);
DECLARE_SCALER(Normal2x);
DECLARE_SCALER(Normal3x);
DECLARE_SCALER(Timex1_5x);
DECLARE_SCALER(TV2x);
DECLARE_SCALER(TV3x);
DECLARE_SCALER(TimexTV);
DECLARE_SCALER(DotMatrix);
DECLARE_SCALER(PalTV);
DECLARE_SCALER(PalTV2x);
DECLARE_SCALER(PalTV3x);
DECLARE_SCALER(HQ2x);
DECLARE_SCALER(HQ3x);

#endif				/* #ifndef FUSE_SCALER_INTERNALS_H */
