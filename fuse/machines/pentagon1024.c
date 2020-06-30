/* pentagon1024.c: Pentagon 1024 specific routines This machine is expected to
                  be a post-1996 Pentagon (a 1024k v2.2 1024SL?).
   Copyright (c) 1999-2011 Philip Kendall and Fredrick Meunier

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
#include "pentagon.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"

static int pentagon1024_reset( void );
static int pentagon1024_memory_map( void );

int
pentagon1024_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PENT1024;
  machine->id = "pentagon1024";

  machine->reset = pentagon1024_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;
  machine->ram.valid_pages    = 64;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = pentagon1024_memory_map;

  return 0;
}

static int
pentagon1024_reset(void)
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_pentagon1024_0,
                            settings_default.rom_pentagon1024_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_pentagon1024_1,
                            settings_default.rom_pentagon1024_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, settings_current.rom_pentagon1024_3,
                            settings_default.rom_pentagon1024_3, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( beta_memory_map_romcs, 0,
                                 settings_current.rom_pentagon1024_2,
                                 settings_default.rom_pentagon1024_2, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  beta_builtin = 1;
  beta_active = 1;

  periph_clear();
  machines_periph_pentagon();

  /* Pentagon 1024 memory paging */
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_PENTAGON1024_MEMORY, PERIPH_PRESENT_ALWAYS );

  /* Later style Betadisk 128 interface */
  periph_set_present( PERIPH_TYPE_BETA128_PENTAGON_LATE, PERIPH_PRESENT_ALWAYS );

  periph_set_present( PERIPH_TYPE_COVOX_FB, PERIPH_PRESENT_OPTIONAL );

  periph_update();

  spec48_common_display_setup();

  return 0;
}

void
pentagon1024_memoryport_write( libspectrum_word port GCC_UNUSED,
			       libspectrum_byte b )
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte = b;
  machine_current->memory_map();

  if( machine_current->ram.last_byte2 & 0x04 ) /* v2.2 */
    machine_current->ram.locked = b & 0x20;
}

void
pentagon1024_v22_memoryport_write( libspectrum_word port GCC_UNUSED,
				   libspectrum_byte b)
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte2 = b;
  if( b & 0x01 ) {
    display_dirty = display_dirty_pentagon_16_col;
    display_write_if_dirty = display_write_if_dirty_pentagon_16_col;
    display_dirty_flashing = display_dirty_flashing_pentagon_16_col;
    memory_display_dirty = memory_display_dirty_pentagon_16_col;
  } else {
    spec48_common_display_setup();
  }
  machine_current->memory_map();
}

static int pentagon1024_memory_map( void )
{
  int rom, page, screen;

  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  if( beta_active && !( machine_current->ram.last_byte & 0x10 ) ) {
    rom = 2;
  } else {
    rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;
  }

  machine_current->ram.current_rom = rom;

  if( machine_current->ram.last_byte2 & 0x08 ) {
    memory_map_16k( 0x0000, memory_map_ram, 0 );  /* v2.2 */
    machine_current->ram.special = 1;             /* v2.2 */
  } else {
    spec128_select_rom( rom );
  }

  page = ( machine_current->ram.last_byte & 0x07 );

  if( !( machine_current->ram.last_byte2 & 0x04 ) ) {
    page += ( ( machine_current->ram.last_byte & 0xC0 ) >> 3 ) +
	    ( machine_current->ram.last_byte & 0x20 );
  }

  spec128_select_page( page );
  machine_current->ram.current_page = page;

  memory_romcs_map();

  return 0;
}
