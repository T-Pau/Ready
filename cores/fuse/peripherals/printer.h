/* printer.h: Printer support
   Copyright (c) 2001-2016 Ian Collier, Russell Marks, Philip Kendall

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

#ifndef FUSE_PRINTER_H
#define FUSE_PRINTER_H

#include <libspectrum.h>

void printer_frame( void );
void printer_serial_write( libspectrum_byte b );
void printer_parallel_strobe_write( int on );
void printer_parallel_write( libspectrum_word port, libspectrum_byte b );
void printer_register_startup( void );

#endif				/* #ifndef FUSE_PRINTER_H */
