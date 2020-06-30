/* spec48_ntsc.c: NTSC Spectrum 48K specific routines
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
#include "machines.h"
#include "machines_periph.h"
#include "memory_pages.h"
#include "periph.h"
#include "settings.h"
#include "spec48.h"
#include "spectrum.h"

static int spec48_ntsc_reset( void );

int spec48_ntsc_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_48_NTSC;
  machine->id = "48_ntsc";

  machine->reset = spec48_ntsc_reset;

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
spec48_ntsc_reset( void )
{
  int error;

  error = machine_load_rom( 0, settings_current.rom_48,
                            settings_default.rom_48, 0x4000 );
  if( error ) return error;

  periph_clear();
  machines_periph_48();
  periph_update();

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  spec48_common_display_setup();

  return spec48_common_reset();
}
