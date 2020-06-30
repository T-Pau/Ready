/* covox.c: Routines for handling the Covox
   Copyright (c) 2011-2016 Jon Mitchell, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2017 Fredrick Meunier

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
#include "covox.h"
#include "spectrum.h"

static void covox_reset( int hard_reset );
static void covox_enabled_snapshot( libspectrum_snap *snap );
static void covox_from_snapshot( libspectrum_snap *snap );
static void covox_to_snapshot( libspectrum_snap *snap );

static module_info_t covox_module_info = {

  /* .reset = */ covox_reset,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ covox_enabled_snapshot,
  /* .snapshot_from = */ covox_from_snapshot,
  /* .snapshot_to = */ covox_to_snapshot,

};

static const periph_port_t covox_ports_fb[] = {
  { 0x00ff, 0x00fb, NULL, sound_covox_write },
  { 0, 0, NULL, NULL }
};

static const periph_t covox_periph_fb = {
  /* .option = */ &settings_current.covox,
  /* .ports = */ covox_ports_fb,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static const periph_port_t covox_ports_dd[] = {
  { 0x00ff, 0x00dd, NULL, sound_covox_write },
  { 0, 0, NULL, NULL }
};

static const periph_t covox_periph_dd = {
  /* .option = */ &settings_current.covox,
  /* .ports = */ covox_ports_dd,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
covox_init( void *context )
{
  module_register( &covox_module_info );
  periph_register( PERIPH_TYPE_COVOX_FB, &covox_periph_fb );
  periph_register( PERIPH_TYPE_COVOX_DD, &covox_periph_dd );

  return 0;
}

void
covox_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_COVOX, dependencies,
                            ARRAY_SIZE( dependencies ), covox_init, NULL,
                            NULL );
}

static void
covox_reset( int hard_reset GCC_UNUSED )
{
  machine_current->covox.covox_dac = 0;
}

static void
covox_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.covox = libspectrum_snap_covox_active( snap );
}

static void
covox_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_covox_active( snap ) ) return;

  /* We just set the internal machine status to the last read covox_dac
   * instead of trying to write to the sound routines, as at this stage
   * sound isn't initialised so there is no synth to write to
   */

  machine_current->covox.covox_dac = libspectrum_snap_covox_dac( snap );
}

static void
covox_to_snapshot( libspectrum_snap *snap )
{
  if( !(periph_is_active( PERIPH_TYPE_COVOX_FB ) ||
        periph_is_active( PERIPH_TYPE_COVOX_DD ) ) )
    return;

  libspectrum_snap_set_covox_active( snap, 1 );
  libspectrum_snap_set_covox_dac( snap, machine_current->covox.covox_dac );
}
