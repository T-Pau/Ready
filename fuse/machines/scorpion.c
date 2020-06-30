/* scorpion.c: Scorpion 256K specific routines
   Copyright (c) 1999-2011 Philip Kendall, Fredrick Meunier and Stuart Brady

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
#include <unistd.h>

#include <libspectrum.h>

#include "compat.h"
#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "pentagon.h"
#include "peripherals/disk/beta.h"
#include "settings.h"
#include "scorpion.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus3.h"
#include "spectrum.h"

static int scorpion_reset( void );
static int scorpion_memory_map( void );

int
scorpion_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_SCORP;
  machine->id = "scorpion";

  machine->reset = scorpion_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages    = 16;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = scorpion_memory_map;

  return 0;
}

int
scorpion_reset(void)
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_scorpion_0,
                            settings_default.rom_scorpion_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_scorpion_1,
                            settings_default.rom_scorpion_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_scorpion_2,
                            settings_default.rom_scorpion_2, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( beta_memory_map_romcs, 0,
                                 settings_current.rom_scorpion_3,
                                 settings_default.rom_scorpion_3, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  periph_clear();
  machines_periph_pentagon();

  /* +3-style memory paging */
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_PLUS3_MEMORY, PERIPH_PRESENT_ALWAYS );

  /* Later style Betadisk 128 interface */
  periph_set_present( PERIPH_TYPE_BETA128_PENTAGON_LATE, PERIPH_PRESENT_ALWAYS );

  periph_set_present( PERIPH_TYPE_COVOX_DD, PERIPH_PRESENT_OPTIONAL );

  periph_update();

  beta_builtin = 1;
  beta_active = 0;

  spec48_common_display_setup();

  return 0;
}

static int
scorpion_memory_map( void )
{
  int rom, page, screen;

  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  if( machine_current->ram.last_byte2 & 0x02 ) {
    rom = 2;
  } else {
    rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;
  }
  machine_current->ram.current_rom = rom;

  if( machine_current->ram.last_byte2 & 0x01 ) {
    memory_map_16k( 0x0000, memory_map_ram, 0 );
    machine_current->ram.special = 1;
  } else {
    spec128_select_rom( rom );
  }

  page = ( ( machine_current->ram.last_byte2 & 0x10 ) >> 1 ) |
           ( machine_current->ram.last_byte  & 0x07 );
  
  spec128_select_page( page );
  machine_current->ram.current_page = page;

  memory_romcs_map();

  return 0;
}
