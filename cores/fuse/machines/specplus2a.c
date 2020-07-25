/* specplus2a.c: Spectrum +2A specific routines
   Copyright (c) 1999-2011 Philip Kendall

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

#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus3.h"

static int specplus2a_reset( void );

int
specplus2a_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PLUS2A;
  machine->id = "plus2a";

  machine->reset = specplus2a_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = specplus3_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_76543210;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages	     = 8;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = specplus3_memory_map;

  return 0;
}

static int
specplus2a_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_plus2a_0,
                            settings_default.rom_plus2a_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_plus2a_1,
                            settings_default.rom_plus2a_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_plus2a_2,
                            settings_default.rom_plus2a_2, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 3, settings_current.rom_plus2a_3,
                            settings_default.rom_plus2a_3, 0x4000 );
  if( error ) return error;

  error = specplus3_plus2a_common_reset();
  if( error ) return error;

  periph_clear();
  machines_periph_plus3();
  periph_update();

  spec48_common_display_setup();

  return 0;
}

