/* ts2068.c: Timex TS2068 specific routines
   Copyright (c) 1999-2015 Philip Kendall, Fredrick Meunier, Witold Filipczyk,
                           Darren Salt

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

#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "periph.h"
#include "peripherals/dck.h"
#include "peripherals/scld.h"
#include "spec48.h"
#include "settings.h"
#include "tc2068.h"
#include "ui/ui.h"

static int ts2068_reset( void );

int
ts2068_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TS2068;
  machine->id = "ts2068";

  machine->reset = ts2068_reset;

  machine->timex = 1;
  machine->ram.port_from_ula	     = tc2048_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;
  machine->ram.valid_pages	     = 3;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;
}

static int
ts2068_reset( void )
{
  size_t i, j;
  int error;

  error = machine_load_rom( 0, settings_current.rom_ts2068_0,
                            settings_default.rom_ts2068_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_ts2068_1,
                            settings_default.rom_ts2068_1, 0x2000 );
  if( error ) return error;

  /* 0x0000: ROM 0 */
  scld_home_map_16k( 0x0000, memory_map_rom, 0 );
  /* 0x4000: RAM 5, contended */
  memory_ram_set_16k_contention( 5, 1 );
  scld_home_map_16k( 0x4000, memory_map_ram, 5 );
  /* 0x8000: RAM 2, not contended */
  memory_ram_set_16k_contention( 2, 0 );
  scld_home_map_16k( 0x8000, memory_map_ram, 2 );
  /* 0xc000: RAM 0, not contended */
  memory_ram_set_16k_contention( 0, 0 );
  scld_home_map_16k( 0xc000, memory_map_ram, 0 );

  periph_clear();
  machines_periph_timex();

  /* TS2068 has its own joysticks */
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );

  periph_update();

  for( i = 0; i < 8; i++ )
    for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
      memory_page *dock_page, *exrom_page;
      
      dock_page = &timex_dock[i * MEMORY_PAGES_IN_8K + j];
      *dock_page = tc2068_empty_mapping[j];
      dock_page->page_num = i;

      exrom_page = &timex_exrom[i * MEMORY_PAGES_IN_8K + j];
      *exrom_page = memory_map_rom[MEMORY_PAGES_IN_16K + j];
      exrom_page->source = memory_source_exrom;
      exrom_page->page_num = i;
    }

  tc2068_tc2048_common_reset();

  error = dck_reset();
  if( error ) {
    ui_error( UI_ERROR_INFO, "Ignoring Timex dock file '%s'",
            settings_current.dck_file );
  }

  return 0;
}
