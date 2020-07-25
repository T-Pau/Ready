/* specdrum.c: Routines for handling the Specdrum Drum Kit
   Copyright (c) 2011-2016 Jon Mitchell, Philip Kendall
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

   Philip: philip-fuse@shadowmagic.org.uk

   Jon: ooblick@gmail.com

*/

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "sound.h"
#include "specdrum.h"
#include "spectrum.h"

static void specdrum_reset( int hard_reset );
static void specdrum_enabled_snapshot( libspectrum_snap *snap );
static void specdrum_from_snapshot( libspectrum_snap *snap );
static void specdrum_to_snapshot( libspectrum_snap *snap );

static module_info_t specdrum_module_info = {

  /* .reset = */ specdrum_reset,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ specdrum_enabled_snapshot,
  /* .snapshot_from = */ specdrum_from_snapshot,
  /* .snapshot_to = */ specdrum_to_snapshot,

};

static const periph_port_t specdrum_ports[] = {
  { 0x00ff, 0x00df, NULL, sound_specdrum_write },
  { 0, 0, NULL, NULL }
};

static const periph_t specdrum_periph = {
  /* .option = */ &settings_current.specdrum,
  /* .ports = */ specdrum_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
specdrum_init( void *context )
{
  module_register( &specdrum_module_info );
  periph_register( PERIPH_TYPE_SPECDRUM, &specdrum_periph );

  return 0;
}

void
specdrum_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_SPECDRUM, dependencies,
                            ARRAY_SIZE( dependencies ), specdrum_init, NULL,
                            NULL );
}

static void
specdrum_reset( int hard_reset GCC_UNUSED )
{
  machine_current->specdrum.specdrum_dac = 0;
}

static void 
specdrum_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.specdrum = libspectrum_snap_specdrum_active( snap );
}

static void
specdrum_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_specdrum_active( snap ) ) return;

  /* We just set the internal machine status to the last read specdrum_dac
   * instead of trying to write to the sound routines, as at this stage 
   * sound isn't initialised so there is no synth to write to
   */

  machine_current->specdrum.specdrum_dac = libspectrum_snap_specdrum_dac( snap );
}

static void specdrum_to_snapshot( libspectrum_snap *snap )
{
  if( !periph_is_active( PERIPH_TYPE_SPECDRUM ) ) return;

  libspectrum_snap_set_specdrum_active( snap, 1 );
  libspectrum_snap_set_specdrum_dac( snap, machine_current->specdrum.specdrum_dac );
}
