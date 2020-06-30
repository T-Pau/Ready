/* machines_periph.c: various machine-specific peripherals
   Copyright (c) 2011-2016 Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Gergely Szasz

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

#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "machines_periph.h"
#include "pentagon.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "spec128.h"
#include "specplus3.h"
#include "tc2068.h"

static void
spec_se_memoryport_write( libspectrum_word port GCC_UNUSED,
			  libspectrum_byte b )
{
  machine_current->ram.last_byte = b;
  machine_current->memory_map();
}

static const periph_port_t spec128_memory_ports[] = {
  { 0x8002, 0x0000, NULL, spec128_memoryport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t spec128_memory = {
  NULL,
  spec128_memory_ports,
  0,
  NULL
};

static const periph_port_t plus3_memory_ports[] = {
  { 0xc002, 0x4000, NULL, spec128_memoryport_write },
  { 0xf002, 0x1000, NULL, specplus3_memoryport2_write },
  { 0, 0, NULL, NULL }
};

static const periph_t plus3_memory = {
  NULL,
  plus3_memory_ports,
  0,
  NULL
};

static const periph_port_t upd765_ports[] = {
  { 0xf002, 0x3000, specplus3_fdc_read, specplus3_fdc_write },
  { 0xf002, 0x2000, specplus3_fdc_status, NULL },
  { 0, 0, NULL, NULL }
};

static const periph_t upd765 = {
  NULL,
  upd765_ports,
  0,
  NULL
};

static const periph_port_t se_memory_ports[] = {
  { 0xffff, 0x7ffd, NULL, spec_se_memoryport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t se_memory = {
  NULL,
  se_memory_ports,
  0,
  NULL
};

static const periph_port_t tc2068_ay_ports[] = {
  { 0x00ff, 0x00f5, tc2068_ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, tc2068_ay_dataport_read, ay_dataport_write },
  { 0, 0, NULL, NULL }
};

static const periph_t tc2068_ay = {
  NULL,
  tc2068_ay_ports,
  0,
  NULL
};

static const periph_port_t beta128_pentagon_ports[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00ff, pentagon_select_ff_read, beta_sp_write },
  { 0, 0, NULL, NULL }
};

static const periph_t beta128_pentagon = {
  NULL,
  beta128_pentagon_ports,
  0,
  NULL
};

static const periph_port_t beta128_pentagon_late_ports[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, beta_cr_write },
  { 0x00ff, 0x003f, beta_tr_read, beta_tr_write },
  { 0x00ff, 0x005f, beta_sec_read, beta_sec_write },
  { 0x00ff, 0x007f, beta_dr_read, beta_dr_write },
  { 0x00ff, 0x00ff, beta_sp_read, beta_sp_write },
  { 0, 0, NULL, NULL }
};

static const periph_t beta128_pentagon_late = {
  NULL,
  beta128_pentagon_late_ports,
  0,
  NULL
};

static const periph_port_t pentagon1024_memory_ports[] = {
  { 0xc002, 0x4000, NULL, pentagon1024_memoryport_write  },
  { 0xf008, 0xe000, NULL, pentagon1024_v22_memoryport_write }, /* v2.2 */
  { 0, 0, NULL, NULL }
};

static const periph_t pentagon1024_memory = {
  NULL,
  pentagon1024_memory_ports,
  0,
  NULL
};

static int
machines_periph_init( void *context )
{
  periph_register( PERIPH_TYPE_128_MEMORY, &spec128_memory );
  periph_register( PERIPH_TYPE_PLUS3_MEMORY, &plus3_memory );
  periph_register( PERIPH_TYPE_UPD765, &upd765 );
  periph_register( PERIPH_TYPE_SE_MEMORY, &se_memory );
  periph_register( PERIPH_TYPE_AY_TIMEX_WITH_JOYSTICK, &tc2068_ay );
  periph_register( PERIPH_TYPE_BETA128_PENTAGON, &beta128_pentagon );
  periph_register( PERIPH_TYPE_BETA128_PENTAGON_LATE, &beta128_pentagon_late );
  periph_register( PERIPH_TYPE_PENTAGON1024_MEMORY, &pentagon1024_memory );

  return 0;
}

void
machines_periph_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_MACHINES_PERIPH,
                            dependencies, ARRAY_SIZE( dependencies ),
                            machines_periph_init, NULL, NULL );
}

/* Peripherals generally available on all machines; the Timex machines and
   Russian clones remove some items from this list */
static void
base_peripherals( void )
{
  periph_set_present( PERIPH_TYPE_DIVIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_DIVMMC, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_KEMPSTON_MOUSE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SIMPLEIDE, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SPECCYBOOT, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SPECTRANET, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_ZXATASP, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXCF, PERIPH_PRESENT_OPTIONAL );
}

/* Peripherals available on the 48K and 128K */
static void
base_peripherals_48_128( void )
{
  base_peripherals();
  periph_set_present( PERIPH_TYPE_BETA128, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE1, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_MULTIFACE_128, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_OPUS, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_PLUSD, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_SPECDRUM, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_USOURCE, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the 48K and similar machines */
void
machines_periph_48( void )
{
  base_peripherals_48_128();
  periph_set_present( PERIPH_TYPE_FULLER, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_MELODIK, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_MULTIFACE_1, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_DIDAKTIK80, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_DISCIPLE, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the 128K and similar machines */
void
machines_periph_128( void )
{
  base_peripherals_48_128();
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_ALWAYS );
}

/* The set of peripherals available on the +3 and similar machines */
void
machines_periph_plus3( void )
{
  base_peripherals();
  periph_set_present( PERIPH_TYPE_AY_PLUS3, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_MULTIFACE_3, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_PARALLEL_PRINTER, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_PLUS3_MEMORY, PERIPH_PRESENT_ALWAYS );
  periph_set_present( PERIPH_TYPE_ZXMMC, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the TC2068 and TS2068 */
void
machines_periph_timex( void )
{
  base_peripherals();

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* SCLD always present */
  periph_set_present( PERIPH_TYPE_SCLD, PERIPH_PRESENT_ALWAYS );

  /* AY chip with joystick always present */
  periph_set_present( PERIPH_TYPE_AY_TIMEX_WITH_JOYSTICK, PERIPH_PRESENT_ALWAYS );

  /* ZX Printer and Interface 2 available */
  periph_set_present( PERIPH_TYPE_INTERFACE2, PERIPH_PRESENT_OPTIONAL );
  periph_set_present( PERIPH_TYPE_ZXPRINTER_FULL_DECODE, PERIPH_PRESENT_OPTIONAL );
}

/* The set of peripherals available on the Pentagon and Scorpion */
void
machines_periph_pentagon( void )
{
  base_peripherals();

  /* 128K-style memory paging available */
  periph_set_present( PERIPH_TYPE_128_MEMORY, PERIPH_PRESENT_ALWAYS );

  /* AY available */
  periph_set_present( PERIPH_TYPE_AY, PERIPH_PRESENT_ALWAYS );

  /* ULA uses full decoding */
  periph_set_present( PERIPH_TYPE_ULA, PERIPH_PRESENT_NEVER );
  periph_set_present( PERIPH_TYPE_ULA_FULL_DECODE, PERIPH_PRESENT_ALWAYS );

  /* All machines have a built-in Betadisk 128 interface, which also
     handles Kempston joystick as they share a port; we don't add the
     actual Betadisk interface here as it differs slightly between the
     (original) Pentagon and the Scorpion/Pentagon 1024 */
  periph_set_present( PERIPH_TYPE_KEMPSTON, PERIPH_PRESENT_NEVER );
}
