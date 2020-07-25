/* melodik.c: Routines for handling the Melodik interface
   Copyright (c) 2009-2016 Fredrick Meunier, Philip Kendall
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

*/

#include <config.h>

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "infrastructure/startup_manager.h"
#include "melodik.h"
#include "module.h"
#include "periph.h"
#include "settings.h"

static void melodik_enabled_snapshot( libspectrum_snap *snap );
static void melodik_from_snapshot( libspectrum_snap *snap );
static void melodik_to_snapshot( libspectrum_snap *snap );

static module_info_t melodik_module_info = {

  /* .reset = */ NULL,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ melodik_enabled_snapshot,
  /* .snapshot_from = */ melodik_from_snapshot,
  /* .snapshot_to = */ melodik_to_snapshot,

};

static const periph_port_t melodik_ports[] = {
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, NULL, ay_dataport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t melodik_periph = {
  /* .option = */ &settings_current.melodik,
  /* .ports = */ melodik_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static void
melodik_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.melodik = libspectrum_snap_melodik_active( snap );
}

static void
melodik_from_snapshot( libspectrum_snap *snap )
{
  if( periph_is_active( PERIPH_TYPE_MELODIK ) )
    ay_state_from_snapshot( snap );
}

static void
melodik_to_snapshot( libspectrum_snap *snap )
{
  int active = periph_is_active( PERIPH_TYPE_MELODIK );
  libspectrum_snap_set_melodik_active( snap, active );
}

static int
melodik_init( void *context )
{
  module_register( &melodik_module_info );
  periph_register( PERIPH_TYPE_MELODIK, &melodik_periph );

  return 0;
}

void
melodik_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_MELODIK, dependencies,
                            ARRAY_SIZE( dependencies ), melodik_init, NULL,
                            NULL );
}
