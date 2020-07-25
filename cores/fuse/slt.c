/* slt.c: SLT data handling routines
   Copyright (c) 2004-2016 Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Fredrick Meunier

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

#include <string.h>

#include <libspectrum.h>

#include "infrastructure/startup_manager.h"
#include "module.h"
#include "settings.h"
#include "slt.h"
#include "spectrum.h"
#include "ui/ui.h"

/* .slt level data */

static libspectrum_byte *slt[256];
static size_t slt_length[256];

static libspectrum_byte *slt_screen;	/* The screenshot from the .slt file */
static int slt_screen_level;		/* The level of the screenshot.
					   Not used for anything AFAIK */

static void slt_from_snapshot( libspectrum_snap *snap );
static void slt_to_snapshot( libspectrum_snap *snap );

static module_info_t slt_module_info = {

  NULL,
  NULL,
  NULL,
  slt_from_snapshot,
  slt_to_snapshot,

};

static int
slt_init( void *context )
{
  module_register( &slt_module_info );

  return 0;
}

void
slt_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_SLT, dependencies,
                            ARRAY_SIZE( dependencies ), slt_init, NULL, NULL );
}

int
slt_trap( libspectrum_word address, libspectrum_byte level )
{
  size_t length;
  libspectrum_byte *data;

  if( !settings_current.slt_traps ) return 0;

  if( slt_length[ level ] ) {
    
    length = slt_length[ level ];
    data = slt[ level ];

    while( length-- ) writebyte( address++, *data++ );

  }

  return 0;
}

static void
slt_from_snapshot( libspectrum_snap *snap )
{
  size_t i;

  for( i=0; i<256; i++ ) {

    slt_length[i] = libspectrum_snap_slt_length( snap, i );

    if( slt_length[i] ) {

      slt[i] = memory_pool_allocate( slt_length[i] *
				     sizeof( libspectrum_byte ) );

      memcpy( slt[i], libspectrum_snap_slt( snap, i ),
	      libspectrum_snap_slt_length( snap, i ) );
    }
  }

  if( libspectrum_snap_slt_screen( snap ) ) {

    slt_screen = memory_pool_allocate( 6912 * sizeof( libspectrum_byte ) );

    memcpy( slt_screen, libspectrum_snap_slt_screen( snap ), 6912 );
    slt_screen_level = libspectrum_snap_slt_screen_level( snap );
  }
}

static void
slt_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  for( i=0; i<256; i++ ) {

    libspectrum_snap_set_slt_length( snap, i, slt_length[i] );

    if( slt_length[i] ) {

      buffer = libspectrum_new( libspectrum_byte, slt_length[i] );

      memcpy( buffer, slt[i], slt_length[i] );
      libspectrum_snap_set_slt( snap, i, buffer );
    }
  }

  if( slt_screen ) {
 
    buffer = libspectrum_new( libspectrum_byte, 6912 );

    memcpy( buffer, slt_screen, 6912 );
    libspectrum_snap_set_slt_screen( snap, buffer );
    libspectrum_snap_set_slt_screen_level( snap, slt_screen_level );
  }
}
