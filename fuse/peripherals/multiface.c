/* multiface.c: Multiface One/128/3 handling routines
   Copyright (c) 2005,2007 Gergely Szasz
   Copyright (c) 2017 Fredrick Meunier

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

   Gergely: szaszg@hu.inter.net

   Many thanks to: Mark Woodmass

*/

#include <config.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "debugger/debugger.h"
#include "event.h"
#include "infrastructure/startup_manager.h"
#include "memory.h"
#include "module.h"
#include "multiface.h"
#include "options.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "unittests/unittests.h"
#include "utils.h"
#include "z80/z80.h"

/* 8KB ROM */
#define MULTIFACE_ROM_SIZE 0x2000
/* 8KB RAM */
#define MULTIFACE_RAM_SIZE 0x2000

#define MF_MASK( a ) ( 1 << a )
#ifdef IS
#undef IS
#endif

#define IS(a, b) (a & MF_MASK( b ) )

#ifdef SET
#undef SET
#endif
#define SET(a, b, c) a = ( a & ~MF_MASK( b ) ) | ( c ? MF_MASK( b ) : 0 )

enum {
  MF_1 = 0,
  MF_128,
  MF_3
};

/* Two 8KB memory chunk accessible by the Z80 when /ROMCS is low */
static memory_page multiface_memory_map_romcs_rom[MEMORY_PAGES_IN_8K];
static memory_page multiface_memory_map_romcs_ram[MEMORY_PAGES_IN_8K];

static int romcs = 0;

typedef struct multiface_t {
  int IC8a_Q;			/* IC8 74LS74 first Flip-flop /Q output*/
  int IC8b_Q;			/* IC8 74LS74 second Flip-flop /Q output */
  int J2;			/* Jumper 2 to disable software paging, or
				   the software on/off state for 128/3 */
  int J1;			/* Jumper 1 to disable joystick (always 0) */
  libspectrum_byte xfdd_reg[4]; /* 74LS670 chip store low 4 bits of
                                   0x1ffd, 0x3ffd, 0x5ffd and 0x7ffd */
  periph_type type;		/* type of multiface: one/128/3 */
  libspectrum_byte ram[8192];	/* 8k RAM */
  int *c_settings;		/* ptr to current_settings.multiface### */
  char **d_rom;
  char **c_rom;
} multiface_t;

static multiface_t mf[3];

int multiface_active = 0;
int multiface_activated = 0;
int multiface_available = 0;

/* Memory source */
static int multiface_rom_memory_source, multiface_ram_memory_source;

/* Debugger events */
static const char * const event_type_string = "multiface";
static int page_event, unpage_event;

static int multiface_init( void *context );

static void multiface_page( int idx );
static void multiface_unpage( int idx );
static void multiface_reset( int hard_reset );
static void multiface_memory_map( void );

static libspectrum_byte multiface_port_in1( libspectrum_word port,
                                            libspectrum_byte *attached );
static libspectrum_byte multiface_port_in128( libspectrum_word port,
                                              libspectrum_byte *attached );
static libspectrum_byte multiface_port_in3( libspectrum_word port,
                                            libspectrum_byte *attached );

static void multiface_port_out1( libspectrum_word port, libspectrum_byte val );
static void multiface_port_out128( libspectrum_word port,
                                   libspectrum_byte val );
static void multiface_port_out3( libspectrum_word port, libspectrum_byte val );

static void multiface_port_xffd_write( libspectrum_word port,
                                       libspectrum_byte val );

static void multiface_enabled_snapshot( libspectrum_snap *snap );
static void multiface_from_snapshot( libspectrum_snap *snap );
static void multiface_to_snapshot( libspectrum_snap *snap );

static module_info_t multiface_module_info = {

  /* .reset = */ multiface_reset,
  /* .romcs = */ multiface_memory_map,
  /* .snapshot_enabled = */ multiface_enabled_snapshot,
  /* .snapshot_from = */ multiface_from_snapshot,
  /* .snapshot_to = */ multiface_to_snapshot,

};

static const periph_port_t multiface_ports_1[] = {
/* ---- ----  x001 --1-  */
  { 0x0072, 0x0012, multiface_port_in1, multiface_port_out1 },
  { 0, 0, NULL, NULL }
};

static const periph_port_t multiface_ports_128[] = {
/* ---- ----  x011 --1-  */
  { 0x0072, 0x0032, multiface_port_in128, multiface_port_out128 },
  { 0, 0, NULL, NULL }
};

static const periph_port_t multiface_ports_3[] = {
/* ---- ----  x011 --1-  */
  { 0x0072, 0x0032, multiface_port_in3, multiface_port_out3 },
  { 0x90ff, 0x10fd, NULL, multiface_port_xffd_write },
  { 0, 0, NULL, NULL }
};

static const periph_t multiface_periph_1 = {
  &settings_current.multiface1,
  multiface_ports_1,
  1,
  NULL
};

static const periph_t multiface_periph_128 = {
  &settings_current.multiface128,
  multiface_ports_128,
  1,
  NULL
};

static const periph_t multiface_periph_3 = {
  &settings_current.multiface3,
  multiface_ports_3,
  1,
  NULL
};

void
multiface_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_MULTIFACE, dependencies,
                            ARRAY_SIZE( dependencies ), multiface_init, NULL,
                            NULL );
}

static int
multiface_init( void *context GCC_UNUSED )
{
  int i;

  multiface_active = 0;
  multiface_activated = 0;
  multiface_available = 0;

  module_register( &multiface_module_info );
  multiface_rom_memory_source = memory_source_register( "Multiface ROM" );
  multiface_ram_memory_source = memory_source_register( "Multiface RAM" );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    multiface_memory_map_romcs_rom[i].source = multiface_rom_memory_source;
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    multiface_memory_map_romcs_ram[i].source = multiface_ram_memory_source;

  mf[MF_1].type = PERIPH_TYPE_MULTIFACE_1;
  mf[MF_128].type = PERIPH_TYPE_MULTIFACE_128;
  mf[MF_3].type = PERIPH_TYPE_MULTIFACE_3;
  mf[MF_1].c_settings = &settings_current.multiface1;
  mf[MF_128].c_settings = &settings_current.multiface128;
  mf[MF_3].c_settings = &settings_current.multiface3;
  mf[MF_1].d_rom = &settings_default.rom_multiface1;
  mf[MF_128].d_rom = &settings_default.rom_multiface128;
  mf[MF_3].d_rom = &settings_default.rom_multiface3;
  mf[MF_1].c_rom = &settings_current.rom_multiface1;
  mf[MF_128].c_rom = &settings_current.rom_multiface128;
  mf[MF_3].c_rom = &settings_current.rom_multiface3;

  periph_register( PERIPH_TYPE_MULTIFACE_1, &multiface_periph_1 );
  periph_register( PERIPH_TYPE_MULTIFACE_128, &multiface_periph_128 );
  periph_register( PERIPH_TYPE_MULTIFACE_3, &multiface_periph_3 );
  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
multiface_reset_real( int idx, int hard_reset )
{
  int i;

  multiface_unpage( idx );

  SET( multiface_activated, idx, 0 );
  SET( multiface_available, idx, 0 );

  if( hard_reset ) memset( mf[idx].ram, 0, 8192 );
  if( !periph_is_active( mf[idx].type ) ) return;

  mf[idx].IC8a_Q = 1;
  mf[idx].IC8b_Q = 1;
  mf[idx].J1 = 0;		/* Joystick always disabled :-( */

  if( mf[idx].type == PERIPH_TYPE_MULTIFACE_1 )
    mf[idx].J2 = settings_current.multiface1_stealth ? 0 : 1;
  else
    mf[idx].J2 = 0;

  memset( mf[idx].xfdd_reg, 0x00, 4 );

  *mf[idx].c_settings = 0;
  periph_activate_type( mf[idx].type, 0 );

  if( machine_load_rom_bank( multiface_memory_map_romcs_rom, 0,
                             *mf[idx].c_rom, *mf[idx].d_rom, 0x2000 ) )
    return;

  machine_current->ram.romcs = 0;

/* Now, the last (if all enabled: MF_3) is win (ROM/RAM)
   BTW: not too critical, because if only one selected
        than have to works this stuff properly...
*/
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    struct memory_page *page = &multiface_memory_map_romcs_ram[ i ];
    page->page = &mf[idx].ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }

  *mf[idx].c_settings = 1;
  SET( multiface_available, idx, 1 );
  periph_activate_type( mf[idx].type, 1 );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_MULTIFACE, 1 );
}

static void
multiface_reset( int hard_reset )
{
  multiface_reset_real( MF_1, hard_reset );
  multiface_reset_real( MF_128, hard_reset );
  multiface_reset_real( MF_3, hard_reset );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_MULTIFACE,
                                  ( multiface_available ? 1 : 0 ) );
}

void
multiface_status_update( void )
{
  int i;

  ui_menu_activate( UI_MENU_ITEM_MACHINE_MULTIFACE, 0 );

  for( i = MF_1; i <= MF_3; i++ )
    SET( multiface_available, i, periph_is_active( mf[i].type ) );

  if( !multiface_available ) return;

  ui_menu_activate( UI_MENU_ITEM_MACHINE_MULTIFACE, 1 );
  if( IS( multiface_available, MF_1 ) &&
      mf[MF_1].J2 == settings_current.multiface1_stealth ) {
    mf[MF_1].J2 = settings_current.multiface1_stealth ? 0 : 1;
  }
}

static void
multiface_page( int idx )
{
  if( IS( multiface_active, idx ) ) return;
  SET( multiface_active, idx, 1 );
  romcs = machine_current->ram.romcs;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
  if( mf[idx].type != PERIPH_TYPE_MULTIFACE_1 )
    mf[idx].J2 = 1;
}

static void
multiface_unpage( int idx )
{
  if( !IS( multiface_active, idx ) ) return;
  SET( multiface_active, idx, 0 );
  machine_current->ram.romcs = romcs;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
multiface_memory_map( void )
{
  if( !multiface_active )
    return;

  memory_map_romcs_8k( 0x0000, multiface_memory_map_romcs_rom );
  memory_map_romcs_8k( 0x2000, multiface_memory_map_romcs_ram );
}

static libspectrum_byte
multiface_port_in1( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte ret = 0xff;
  int a7;

  if( !IS( multiface_available, MF_1 ) ) return ret;

  /* TODO: check if this value should be set to 0xff */
  *attached = 0xff;

  /* in () */
  /* Multiface one */
  /* xxxxxxxx 1001xx1x page in  IN A, (159) 10011111*/
  /* xxxxxxxx 0001xx1x page out IN A, (31)  00011111*/

  a7 = port & 0x0080;

  /* TODO: read joystick */
  /*
  if( mf[MF_1].J1 ) {
  }
  */

  if( a7 ) {
    if( mf[MF_1].J2 ) {
      multiface_page( MF_1 );
      mf[MF_1].IC8a_Q = 0;
    }
  }
  else {
    multiface_unpage( MF_1 );		/* a7 == 0 */
    mf[MF_1].IC8a_Q = 1;
  }

  return ret;
}

static libspectrum_byte
multiface_port_in128( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte ret = 0xff;
  int a7;

  if( !IS( multiface_available, MF_128 ) ) return ret;

  /* TODO: check if this value should be set to 0xff */
  *attached = 0xff;

  /* Multiface 128 */
  /* I have only the MF128 user guide which say: */
  /*  IN A, (191) -> page in, and IN A, (63) page out */
  /*  let see: */
  /* xxxxxxxx 10111111 ok, may a0 don't care, so */
  /* xxxxxxxx 1011111x and may have some other don't care bit, but how know? */
  /**/
  /* xxxxxxxx 00111111 ok, may a0 don't care, so */
  /* xxxxxxxx 0011111x and may have some other don't care bit, but how know? */

  a7 = port & 0x0080;
  if( a7 ) {
    if( mf[MF_128].J2 ) {
      multiface_page( MF_128 );
      ret = machine_current->ram.last_byte & 0x08 ? 0xff : 0x7f;
      mf[MF_128].IC8a_Q = 0;
    }
  } else {
    multiface_unpage( MF_128 );		/* a7 == 0 */
    mf[MF_128].IC8a_Q = 1;
  }
  return ret;
}

static libspectrum_byte
multiface_port_in3( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte ret = 0xff;
  int a7;

  if( !IS( multiface_available, MF_3 ) ) return ret;

  /* TODO: check if this value should be set to 0xff */
  *attached = 0xff;

  /* Multiface 3 */
  /* The MF3 user guide say nothing about paging memory :-( */
  /* The following links say */
  /*  http://x128.speccy.cz/multiface/multiface.htm */
  /*  https://www.worldofspectrum.org/forums/discussion/comment/303231/#Comment_303231 */
  /*  IN A, (63) -> page in, and IN A, (191) page out */

  a7 = port & 0x0080;
  if( a7 ) {			/* a7 == 1 */
    multiface_unpage( MF_3 );
    mf[MF_3].IC8a_Q = 0;
  } else if( mf[MF_3].J2 ) {
    multiface_page( MF_3 );	/* a7 == 0 */
    mf[MF_3].IC8a_Q = 1;
  }

  /* Return last data written to port 0x1ffd, 0x3ffd, 0x5ffd or 0x7ffd */
  if( mf[MF_3].J2 )
    ret = mf[MF_3].xfdd_reg[ ( port & 0x6000 ) >> 13 ] | 0xf0;

  return ret;
}

static void
multiface_port_out1( libspectrum_word port GCC_UNUSED,
                     libspectrum_byte val GCC_UNUSED )
{
  if( !IS( multiface_available, MF_1 ) ) return;

  /* MF one: out () */
  /*         xxxxxxxx x001xx1x page out */
  mf[MF_1].IC8b_Q = 1;
}

static void
multiface_port_out128_3( int idx, libspectrum_word port )
{
  if( !IS( multiface_available, idx ) ) return;

  if( IS( multiface_active, idx ) ) {
    mf[idx].J2 = port & 0x0080 ? 1 : 0; /* A7 == 1 */
  }
  mf[idx].IC8b_Q = 1;
}

static void
multiface_port_out128( libspectrum_word port, libspectrum_byte val GCC_UNUSED )
{
  multiface_port_out128_3( MF_128, port );
}

static void
multiface_port_out3( libspectrum_word port, libspectrum_byte val GCC_UNUSED )
{
  multiface_port_out128_3( MF_3, port );
}

static void
multiface_port_xffd_write( libspectrum_word port, libspectrum_byte val )
{
  mf[MF_3].xfdd_reg[ ( port & 0x6000 ) >> 13 ] = val & 0x0f;
}

void
multiface_red_button( void )
{
  int i;
/*
  One RED button for all ;-)
*/
  for( i = MF_3; i >= MF_1; i-- ) {
    if( !IS( multiface_available, i ) || mf[i].IC8b_Q == 0 ) continue;

    /* Note: AFAIK the Multiface One schematics show that the physical switch
       (J2) disables paging in OFF state and has no effect on NMI generation.
       But the manual states that the interface is unusable while switched OFF.
       Until better understanding, NMI generation is disabled to avoid a freeze
       in the spectrum machine.
    */
    if( i == MF_1 && !mf[MF_1].J2 ) continue;

    mf[i].IC8b_Q = 0;
    SET( multiface_activated, i, 1 );
    event_add( 0, z80_nmi_event );	/* pull /NMI */
    break;
  }
}

void
multiface_setic8( void )
{
  int i;
/*
  activate all at once...
*/
  for( i = MF_3; i >= MF_1; i-- ) {
    if( !IS( multiface_available, i ) || mf[i].IC8b_Q == 1 ) continue;

    mf[i].IC8a_Q = 0;
    SET( multiface_activated, i, 0 );
    multiface_page( i );
    break;
  }
}

int
multiface_unittest( void )
{
  int r = 0;

  multiface_page( MF_1 );

  r += unittests_assert_8k_page( 0x0000, multiface_rom_memory_source, 0 );
  r += unittests_assert_8k_page( 0x2000, multiface_ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  multiface_unpage( MF_1 );

  r += unittests_paging_test_48( 2 );

  return r;
}

static void
multiface_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.multiface1 = 0;
  settings_current.multiface128 = 0;
  settings_current.multiface3 = 0;

  if( !libspectrum_snap_multiface_active( snap ) ) return;

  if( libspectrum_snap_multiface_model_one( snap ) )
    settings_current.multiface1 = 1;
  else if( libspectrum_snap_multiface_model_128( snap ) )
    settings_current.multiface128 = 1;
  else if( libspectrum_snap_multiface_model_3( snap ) )
    settings_current.multiface3 = 1;
}

static void
multiface_from_snapshot( libspectrum_snap *snap )
{
  int idx;

  if( !libspectrum_snap_multiface_active( snap ) ) return;

  if( libspectrum_snap_multiface_model_one( snap ) )
    idx = MF_1;
  else if( libspectrum_snap_multiface_model_128( snap ) )
    idx = MF_128;
  else if( libspectrum_snap_multiface_model_3( snap ) )
    idx = MF_3;
  else
    return;

  if( !IS( multiface_available, idx ) ) return;

  /* Multiface with 16 Kb RAM not supported */
  if( libspectrum_snap_multiface_ram_length( snap, 0 ) != MULTIFACE_RAM_SIZE ) {
    ui_error( UI_ERROR_ERROR, "Only supported Multiface with 8 Kb RAM" );
    return;
  }

  if( libspectrum_snap_multiface_ram( snap, 0 ) ) {
    memcpy( mf[idx].ram,
            libspectrum_snap_multiface_ram( snap, 0 ), MULTIFACE_RAM_SIZE );
  }

  if( libspectrum_snap_multiface_paged( snap ) ) {
    multiface_page( idx );
    mf[idx].IC8a_Q = ( idx == MF_3 )? 1 : 0;
  } else {
    multiface_unpage( idx );
  }

  /* Restore status of software lockout (128/3) or physical switch (One) */
  switch( idx ) {
  case MF_1:
    mf[MF_1].J2 = !libspectrum_snap_multiface_disabled( snap );
    settings_current.multiface1_stealth = !mf[MF_1].J2;
    break;
  case MF_128:
  case MF_3:
    mf[idx].J2 = !libspectrum_snap_multiface_software_lockout( snap );
    break;
  }

  /* Red button status */
  if( libspectrum_snap_multiface_red_button_disabled( snap ) ) {
    mf[idx].IC8b_Q = 0;
  }

  /* Multiface 3 - 4x4 bit register */
  if( idx == MF_3 ) {
    mf[MF_3].xfdd_reg[ 0 ] =
      libspectrum_snap_out_plus3_memoryport( snap ) & 0x0f;
    mf[MF_3].xfdd_reg[ 3 ] =
      libspectrum_snap_out_128_memoryport( snap ) & 0x0f;
  }
}

static void
multiface_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;
  int idx, i;

  if( periph_is_active( PERIPH_TYPE_MULTIFACE_1 ) ) {
    libspectrum_snap_set_multiface_model_one( snap, 1 );
    idx = MF_1;
  } else if( periph_is_active( PERIPH_TYPE_MULTIFACE_128 ) ) {
    libspectrum_snap_set_multiface_model_128( snap, 1 );
    idx = MF_128;
  } else if( periph_is_active( PERIPH_TYPE_MULTIFACE_3 ) ) {
    libspectrum_snap_set_multiface_model_3( snap, 1 );
    idx = MF_3;
  } else {
    return;
  }

  libspectrum_snap_set_multiface_active( snap, 1 );
  libspectrum_snap_set_multiface_paged( snap, IS( multiface_active, idx ) );

  /* Store status of software lockout (128/3) or physical switch (One) */
  switch( idx ) {
  case MF_1:
    libspectrum_snap_set_multiface_disabled( snap, !mf[MF_1].J2 );
    break;
  case MF_128:
  case MF_3:
    libspectrum_snap_set_multiface_software_lockout( snap, !mf[idx].J2 );
    break;
  }

  /* Store red button status */
  if( !mf[idx].IC8b_Q ) {
    libspectrum_snap_set_multiface_red_button_disabled( snap, 1 );
  }

  buffer = libspectrum_new( libspectrum_byte, MULTIFACE_RAM_SIZE );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    memcpy( buffer + i * MEMORY_PAGE_SIZE,
            multiface_memory_map_romcs_ram[i].page, MEMORY_PAGE_SIZE );

  libspectrum_snap_set_multiface_ram( snap, 0, buffer );
  libspectrum_snap_set_multiface_ram_length( snap, 0, MULTIFACE_RAM_SIZE );
}
