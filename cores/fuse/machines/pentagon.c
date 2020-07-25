/* pentagon.c: Pentagon 128K specific routines, this is intended to be a 1991
               era Pentagon machine with Beta 128 and AY as described in the
               Russian Speccy FAQ and emulated on most Spectrum emulators.
   Copyright (c) 1999-2012 Philip Kendall and Fredrick Meunier
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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "module.h"
#include "pentagon.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/joystick.h"
#include "settings.h"
#include "spec48.h"
#include "spec128.h"

static void pentagon_from_snapshot( libspectrum_snap *snap );

static module_info_t pentagon_module_info = {

  NULL,
  NULL,
  NULL,
  pentagon_from_snapshot,
  NULL,

};

static int pentagon_reset( void );

libspectrum_byte
pentagon_select_1f_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte data;
  libspectrum_byte tmpattached = 0x00;

  /* TODO: fine-grained attachment handling */

  data = beta_sr_read( port, &tmpattached );
  if( !tmpattached && settings_current.joy_kempston )
    data = joystick_kempston_read( port, &tmpattached );

  if( tmpattached ) {
    *attached = 0xff; /* TODO: check this */
    return data;
  }

  return 0xff;
}

libspectrum_byte
pentagon_select_ff_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte data;
  libspectrum_byte tmpattached = 0x00;
  
  /* TODO: fine-grained attachment handling */

  data = beta_sp_read( port, &tmpattached );
  if( !tmpattached )
    data = spectrum_unattached_port();

  *attached = 0xff; /* TODO: check this */
  return data;
}

int
pentagon_port_from_ula( libspectrum_word port GCC_UNUSED )
{
  /* No contended ports */
  return 0;
}

int
pentagon_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PENT;
  machine->id = "pentagon";

  machine->reset = pentagon_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages    = 8;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = spec128_memory_map;

  module_register( &pentagon_module_info );

  return 0;
}

static int
pentagon_reset(void)
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_pentagon_0,
                            settings_default.rom_pentagon_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_pentagon_1,
                            settings_default.rom_pentagon_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( beta_memory_map_romcs, 0,
                                 settings_current.rom_pentagon_2,
                                 settings_default.rom_pentagon_2, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  periph_clear();
  machines_periph_pentagon();

  /* Earlier style Betadisk 128 interface */
  periph_set_present( PERIPH_TYPE_BETA128_PENTAGON, PERIPH_PRESENT_ALWAYS );

  periph_set_present( PERIPH_TYPE_COVOX_FB, PERIPH_PRESENT_OPTIONAL );

  periph_update();

  beta_builtin = 1;
  beta_active = 1;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  spec48_common_display_setup();

  return 0;
}

static void
pentagon_from_snapshot( libspectrum_snap *snap )
{
  /* During init we set beta_active to true unconditionally to bootstrap into
     the TR-DOS ROM, but during snapshot loading we should repect the paging
     setting from the snapshot itself */
  if( periph_is_active( PERIPH_TYPE_BETA128_PENTAGON ) || 
      periph_is_active( PERIPH_TYPE_BETA128_PENTAGON_LATE ) ) {
    if( libspectrum_snap_beta_paged( snap ) ) {
      beta_page();
    } else {
      beta_unpage();
    }
  }
}
