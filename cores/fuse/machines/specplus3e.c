/* specplus3e.c: Spectrum +3e specific routines
   Copyright (c) 1999-2011 Philip Kendall, Darren Salt

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

#include "machines.h"
#include "machines_periph.h"
#include "periph.h"
#include "peripherals/disk/upd_fdc.h"
#include "settings.h"
#include "spec48.h"
#include "specplus3.h"
#include "ui/ui.h"

static int specplus3e_reset( void );
extern upd_fdc *specplus3_fdc;

int
specplus3e_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PLUS3E;
  machine->id = "plus3e";

  machine->reset = specplus3e_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = specplus3_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_76543210;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages	     = 8;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = specplus3_shutdown;

  machine->memory_map = specplus3_memory_map;

  return 0;
}

static int
specplus3e_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_plus3e_0,
                            settings_default.rom_plus3e_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_plus3e_1,
                            settings_default.rom_plus3e_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_plus3e_2,
                            settings_default.rom_plus3e_2, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 3, settings_current.rom_plus3e_3,
                            settings_default.rom_plus3e_3, 0x4000 );
  if( error ) return error;

  error = specplus3_plus2a_common_reset();
  if( error ) return error;

  periph_clear();
  machines_periph_plus3();

  periph_set_present( PERIPH_TYPE_UPD765, PERIPH_PRESENT_ALWAYS );

  periph_update();

  specplus3_765_reset();
  specplus3_menu_items();

  spec48_common_display_setup();

  return 0;
}
