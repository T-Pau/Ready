/* tc2068.c: Timex TC2068 specific routines
   Copyright (c) 1999-2015 Philip Kendall, Fredrick Meunier, Witold Filipczyk,
                           Darren Salt
   Copyright (c) 2015 Adrien Destugues
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

#include <string.h>

#include <libspectrum.h>

#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "periph.h"
#include "peripherals/dck.h"
#include "peripherals/joystick.h"
#include "peripherals/scld.h"
#include "spec48.h"
#include "settings.h"
#include "tc2068.h"
#include "ui/ui.h"

static int tc2068_reset( void );

memory_page tc2068_empty_mapping[MEMORY_PAGES_IN_8K];
static int empty_mapping_allocated = 0;

libspectrum_byte
tc2068_ay_registerport_read( libspectrum_word port, libspectrum_byte *attached )
{
  if( machine_current->ay.current_register == 14 ) return 0xff;

  return ay_registerport_read( port, attached );
}

libspectrum_byte
tc2068_ay_dataport_read( libspectrum_word port, libspectrum_byte *attached )
{
  if (machine_current->ay.current_register != 14) {
    return ay_registerport_read( port, attached );
  } else {

    libspectrum_byte ret;

    /* In theory, we may need to distinguish cases where some data
       is returned here and were it isn't. In practice, this doesn't
       matter for the TC2068 as it doesn't have a floating bus, so we'll
       get 0xff in both cases anyway */
    *attached = 0xff; /* TODO: check this */

    ret =   machine_current->ay.registers[7] & 0x40
	  ? machine_current->ay.registers[14]
	  : 0xff;

    if( port & 0x0100 ) ret &= ~joystick_timex_read( port, 0 );
    if( port & 0x0200 ) ret &= ~joystick_timex_read( port, 1 );

    return ret;
  }
}

static void
ensure_empty_mapping( void )
{
  int i;
  libspectrum_byte *empty_chunk;

  if( empty_mapping_allocated ) return;

  empty_chunk = memory_pool_allocate_persistent( 0x2000, 1 );
  memset( empty_chunk, 0xff, 0x2000 );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    memory_page *page = &tc2068_empty_mapping[i];
    page->page = empty_chunk + i * MEMORY_PAGE_SIZE;
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 0;
    page->contended = 0;
    page->source = memory_source_none;
  }

  empty_mapping_allocated = 1;
}

int
tc2068_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2068;
  machine->id = "2068";

  machine->reset = tc2068_reset;

  machine->timex = 1;
  machine->ram.port_from_ula	     = tc2048_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;
  machine->ram.valid_pages	     = 3;

  ensure_empty_mapping();

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;
}

static int
tc2068_reset( void )
{
  size_t i, j;
  int error;

  error = machine_load_rom( 0, settings_current.rom_tc2068_0,
                            settings_default.rom_tc2068_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 1, settings_current.rom_tc2068_1,
                            settings_default.rom_tc2068_1, 0x2000 );
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

void
tc2068_tc2048_common_reset( void )
{
  scld_set_exrom_dock_contention();

  memory_current_screen = 5;
  memory_screen_mask = 0xdfff;

  scld_dec_write( 0x00ff, 0x00 );
  scld_hsr_write( 0x00f4, 0x00 );

  tc2068_tc2048_common_display_setup();
}

int
tc2068_memory_map( void )
{
  /* Start by mapping in the default configuration */
  scld_memory_map_home();

  /* Then apply horizontal memory */
  scld_memory_map();

  /* And finally ROMCS overrides */
  memory_romcs_map();

  return 0;
}

void
tc2068_tc2048_common_display_setup( void )
{
  display_dirty = display_dirty_timex;
  display_write_if_dirty = display_write_if_dirty_timex;
  display_dirty_flashing = display_dirty_flashing_timex;

  memory_display_dirty = memory_display_dirty_sinclair;
}
