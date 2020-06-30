/* fuller.c: Routines for handling the Fuller Box
   Copyright (c) 2007-2016 Stuart Brady, Fredrick Meunier, Philip Kendall

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

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "fuller.h"
#include "infrastructure/startup_manager.h"
#include "joystick.h"
#include "module.h"
#include "periph.h"
#include "settings.h"

static void fuller_enabled_snapshot( libspectrum_snap *snap );
static void fuller_from_snapshot( libspectrum_snap *snap );
static void fuller_to_snapshot( libspectrum_snap *snap );

static module_info_t fuller_module_info = {

  /* .reset = */ NULL,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ fuller_enabled_snapshot,
  /* .snapshot_from = */ fuller_from_snapshot,
  /* .snapshot_to = */ fuller_to_snapshot,

};

static const periph_port_t fuller_ports[] = {
  { 0x00ff, 0x003f, ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x005f, NULL, ay_dataport_write },
  { 0x00ff, 0x007f, joystick_fuller_read, NULL },
  { 0, 0, NULL, NULL },
};

static const periph_t fuller_periph = {
  /* .option = */ &settings_current.fuller,
  /* .ports = */ fuller_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static void
fuller_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.fuller = libspectrum_snap_fuller_box_active( snap );
}

static void
fuller_from_snapshot( libspectrum_snap *snap )
{
  if( periph_is_active( PERIPH_TYPE_FULLER ) )
    ay_state_from_snapshot( snap );
}

static void
fuller_to_snapshot( libspectrum_snap *snap )
{
  int active = periph_is_active( PERIPH_TYPE_FULLER );
  libspectrum_snap_set_fuller_box_active( snap, active );
}

static int
fuller_init( void *context )
{
  module_register( &fuller_module_info );
  periph_register( PERIPH_TYPE_FULLER, &fuller_periph );

  return 0;
}

void
fuller_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_FULLER, dependencies,
                            ARRAY_SIZE( dependencies ), fuller_init, NULL, 
                            NULL );
}
