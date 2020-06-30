/* spec48.c: Spectrum 48K specific routines
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

#include <stdio.h>

#include <libspectrum.h>

#include "machine.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "spectrum.h"

static int spec48_reset( void );

int
spec48_port_from_ula( libspectrum_word port )
{
  /* All even ports supplied by ULA */
  return !( port & 0x0001 );
}

int spec48_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_48;
  machine->id = "48";

  machine->reset = spec48_reset;

  machine->timex = 0;
  machine->ram.port_from_ula         = spec48_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;
  machine->ram.valid_pages	     = 3;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;
}

static int
spec48_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_48,
                            settings_default.rom_48, 0x4000 );
  if( error ) return error;

  periph_clear();
  machines_periph_48();
  periph_update();

  beta_builtin = 0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return spec48_common_reset();
}

void
spec48_common_display_setup( void )
{
  display_dirty = display_dirty_sinclair;
  display_write_if_dirty = display_write_if_dirty_sinclair;
  display_dirty_flashing = display_dirty_flashing_sinclair;

  memory_display_dirty = memory_display_dirty_sinclair;
}

int
spec48_common_reset( void )
{
  /* 0x0000: ROM 0 */
  memory_map_16k( 0x0000, memory_map_rom, 0 );
  /* 0x4000: RAM 5, contended */
  memory_ram_set_16k_contention( 5, 1 );
  memory_map_16k( 0x4000, memory_map_ram, 5 );
  /* 0x8000: RAM 2, not contended */
  memory_ram_set_16k_contention( 2, 0 );
  memory_map_16k( 0x8000, memory_map_ram, 2 );
  /* 0xc000: RAM 0, not contended */
  memory_ram_set_16k_contention( 0, 0 );
  memory_map_16k( 0xc000, memory_map_ram, 0 );

  return 0;
}

int
spec48_memory_map( void )
{
  memory_map_16k( 0x0000, memory_map_rom, 0 );
  memory_romcs_map();
  return 0;
}
