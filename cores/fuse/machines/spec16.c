/* spec16.c: Spectrum 16K specific routines
   Copyright (c) 1999-2011 Philip Kendall
   Copyright (c) 2015 Adrien Destugues

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
#include <string.h>

#include <libspectrum.h>

#include "machine.h"
#include "machines.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "periph.h"
#include "settings.h"
#include "spec48.h"

static int spec16_reset( void );

static memory_page empty_mapping[MEMORY_PAGES_IN_16K];
static int empty_mapping_allocated = 0;

int spec16_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_16;
  machine->id = "16";

  machine->reset = spec16_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = spec48_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_65432100;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_65432100;
  machine->ram.valid_pages    = 1;

  machine->unattached_port = spectrum_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;
}

static void
ensure_empty_mapping( void )
{
  int i;
  libspectrum_byte *empty_chunk;

  if( empty_mapping_allocated ) return;

  empty_chunk = memory_pool_allocate_persistent( 0x4000, 1 );
  memset( empty_chunk, 0xff, 0x4000 );

  for( i = 0; i < MEMORY_PAGES_IN_16K; i++ ) {
    memory_page *page = &empty_mapping[i];
    page->page = empty_chunk + i * MEMORY_PAGE_SIZE;
    page->writable = 0;
    page->contended = 0;
    page->source = memory_source_none;
  }

  empty_mapping_allocated = 1;
}

static int
spec16_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_16, 
                            settings_default.rom_16, 0x4000 );
  if( error ) return error;

  ensure_empty_mapping();

  periph_clear();
  machines_periph_48();
  periph_update();

  /* The one RAM page is contended */
  memory_ram_set_16k_contention( 5, 1 );

  memory_map_16k( 0x0000, memory_map_rom, 0 );
  memory_map_16k( 0x4000, memory_map_ram, 5 );
  memory_map_16k( 0x8000, empty_mapping, 0 );
  memory_map_16k( 0xc000, empty_mapping, 0 );

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return 0;
}
