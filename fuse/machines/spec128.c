/* spec128.c: Spectrum 128K specific routines
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

#include "compat.h"
#include "machine.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus3.h"

static int spec128_reset( void );

int spec128_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_128;
  machine->id = "128";

  machine->reset = spec128_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = spec48_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;
  machine->ram.valid_pages	     = 8;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec128_memory_map;

  return 0;
}

static int
spec128_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_128_0,
                            settings_default.rom_128_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_128_1,
                            settings_default.rom_128_1, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 1 );
  if( error ) return error;

  periph_clear();
  machines_periph_128();
  periph_update();

  beta_builtin = 0;

  spec48_common_display_setup();

  return 0;
}

int
spec128_common_reset( int contention )
{
  size_t i;

  machine_current->ram.locked=0;
  machine_current->ram.last_byte = 0;

  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  /* Odd pages contended on the 128K/+2; the loop is up to 16 to
     ensure all of the Scorpion's 256Kb RAM is not contended */
  for( i = 0; i < 16; i++ )
    memory_ram_set_16k_contention( i, i & 1 ? contention : 0 );

  /* 0x0000: ROM 0 */
  memory_map_16k( 0x0000, memory_map_rom, 0 );
  /* 0x4000: RAM 5 */
  memory_map_16k( 0x4000, memory_map_ram, 5 );
  /* 0x8000: RAM 2 */
  memory_map_16k( 0x8000, memory_map_ram, 2 );
  /* 0xc000: RAM 0 */
  memory_map_16k( 0xc000, memory_map_ram, 0 );

  return 0;
}

void
spec128_memoryport_write( libspectrum_word port GCC_UNUSED,
			  libspectrum_byte b )
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte = b;

  machine_current->memory_map();

  machine_current->ram.locked = b & 0x20;
}

void
spec128_select_rom( int rom )
{
  memory_map_16k( 0x0000, memory_map_rom, rom );
  machine_current->ram.current_rom = rom;
}

void
spec128_select_page( int page )
{
  memory_map_16k( 0xc000, memory_map_ram, page );
  machine_current->ram.current_page = page;
}

int
spec128_memory_map( void )
{
  int page, screen, rom;

  page = machine_current->ram.last_byte & 0x07;
  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  spec128_select_rom( rom );
  spec128_select_page( page );

  memory_romcs_map();

  return 0;
}
