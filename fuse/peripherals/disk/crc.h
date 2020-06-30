/* crc.h: Routines for CRC16/CRC32
   Copyright (c) 2007 Gergely Szasz

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

   Philip: philip-fuse@shadowmagic.org.uk

*/
#ifndef FUSE_CRC_H
#define FUSE_CRC_H

libspectrum_word crc_fdc( libspectrum_word crc, libspectrum_byte data );
libspectrum_signed_dword crc_udi( libspectrum_signed_dword crc, libspectrum_byte data );

#endif /* FUSE_CRC_H */
