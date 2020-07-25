/* module.h: API for Fuse modules
   Copyright (c) 2007 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#ifndef FUSE_MODULE_H
#define FUSE_MODULE_H

#include <libspectrum.h>

typedef void (*module_reset_fn)( int hard_reset );
typedef void (*module_romcs_fn)( void );
typedef void (*module_snapshot_enabled_fn)( libspectrum_snap *snap );
typedef void (*module_snapshot_from_fn)( libspectrum_snap *snap );
typedef void (*module_snapshot_to_fn)( libspectrum_snap *snap );

typedef struct module_info_t
{

  module_reset_fn reset;
  module_romcs_fn romcs;
  module_snapshot_enabled_fn snapshot_enabled;
  module_snapshot_from_fn snapshot_from;
  module_snapshot_to_fn snapshot_to;

} module_info_t;

int module_register( module_info_t *module );
void module_end( void );

void module_reset( int hard_reset );
void module_romcs( void );
void module_snapshot_enabled( libspectrum_snap *snap );
void module_snapshot_from( libspectrum_snap *snap );
void module_snapshot_to( libspectrum_snap *snap );

#endif			/* #ifndef FUSE_MODULE_H */
