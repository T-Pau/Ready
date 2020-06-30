/* tc2048.c: Timex TC2048 specific routines
   Copyright (c) 1999-2015 Philip Kendall, Fredrick Meunier

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
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/scld.h"
#include "settings.h"
#include "spec48.h"
#include "tc2068.h"

static int tc2048_reset( void );

int
tc2048_port_from_ula( libspectrum_word port )
{
  /* Ports F4 (HSR), FE (SCLD) and FF (DEC) supplied by ULA */
  port &= 0xff;

  return( port == 0xf4 || port == 0xfe || port == 0xff );
}

int tc2048_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2048;
  machine->id = "2048";

  machine->reset = tc2048_reset;

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
tc2048_reset( void )
{
  size_t i, j;
  int error;

  error = machine_load_rom( 0, settings_current.rom_tc2048,
                            settings_default.rom_tc2048, 0x4000 );
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
  machines_periph_48();

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* As does the ZX Printer */
  periph_set_present( PERIPH_TYPE_ZXPRINTER, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ZXPRINTER_FULL_DECODE, PERIPH_PRESENT_OPTIONAL );

  /* SCLD always present */
  periph_set_present( PERIPH_TYPE_SCLD, PERIPH_PRESENT_ALWAYS );

  /* TC2048 has a built-in Kempston joystick, which uses the "loose"
     decoding */
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_KEMPSTON_LOOSE, PERIPH_PRESENT_ALWAYS );

  /* SpeccyBoot doesn't seem to work on the TC2048 */
  periph_set_present( PERIPH_TYPE_SPECCYBOOT, PERIPH_PRESENT_NEVER );

  periph_update();

  beta_builtin = 0;

  for( i = 0; i < 8; i++ )
    for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
      memory_page *dock_page, *exrom_page;
      
      dock_page = &timex_dock[i * MEMORY_PAGES_IN_8K + j];
      *dock_page = tc2068_empty_mapping[j];
      dock_page->page_num = i;

      exrom_page = &timex_exrom[i * MEMORY_PAGES_IN_8K + j];
      *exrom_page = tc2068_empty_mapping[j];
      exrom_page->page_num = i;
    }

  tc2068_tc2048_common_reset();

  return 0;
}
