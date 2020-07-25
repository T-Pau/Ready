/* ula.c: ULA routines
   Copyright (c) 1999-2016 Philip Kendall, Darren Salt
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 Fredrick Meunier

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
#include "debugger/debugger.h"
#include "keyboard.h"
#include "infrastructure/startup_manager.h"
#include "loader.h"
#include "machine.h"
#include "machines/spec128.h"
#include "machines/specplus3.h"
#include "module.h"
#include "periph.h"
#include "phantom_typist.h"
#include "settings.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"
#include "ula.h"

static libspectrum_byte last_byte;

libspectrum_byte ula_contention[ ULA_CONTENTION_SIZE ];
libspectrum_byte ula_contention_no_mreq[ ULA_CONTENTION_SIZE ];

/* What to return if no other input pressed; depends on the last byte
   output to the ULA; see CSS FAQ | Technical Information | Port #FE
   for full details */
libspectrum_byte ula_default_value;

static void ula_from_snapshot( libspectrum_snap *snap );
static void ula_to_snapshot( libspectrum_snap *snap );
static libspectrum_byte ula_read( libspectrum_word port, libspectrum_byte *attached );
static void ula_write( libspectrum_word port, libspectrum_byte b );

static module_info_t ula_module_info = {

  /* .reset = */ NULL,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ ula_from_snapshot,
  /* .snapshot_to = */ ula_to_snapshot,

};

static const periph_port_t ula_ports[] = {
  { 0x0001, 0x0000, ula_read, ula_write },
  { 0, 0, NULL, NULL }
};

static const periph_t ula_periph = {
  /* .option = */ NULL,
  /* .ports = */ ula_ports,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static const periph_port_t ula_ports_full_decode[] = {
  { 0x00ff, 0x00fe, ula_read, ula_write },
  { 0, 0, NULL, NULL }
};

static const periph_t ula_periph_full_decode = {
  /* .option = */ NULL,
  /* .ports = */ ula_ports_full_decode,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

/* Debugger system variables */
static const char * const debugger_type_string = "ula";
static const char * const last_byte_detail_string = "last";
static const char * const tstates_detail_string = "tstates";
static const char * const mem7ffd_detail_string = "mem7ffd";
static const char * const mem1ffd_detail_string = "mem1ffd";

/* Adapter just to get the return type to be what the debugger is expecting */
static libspectrum_dword
get_last_byte( void )
{
  return ula_last_byte();
}

static libspectrum_dword
get_tstates( void )
{
  return tstates;
}

static void
set_tstates( libspectrum_dword value )
{
  tstates = value;
}

static libspectrum_dword
get_7ffd( void )
{
  return machine_current->ram.last_byte;
}

static void
set_7ffd( libspectrum_dword value )
{
  spec128_memoryport_write( 0, value );
}

static libspectrum_dword
get_1ffd( void )
{
  return machine_current->ram.last_byte2;
}

static void
set_1ffd( libspectrum_dword value )
{
  specplus3_memoryport2_write_internal( 0, value );
}

static int
ula_init( void *context )
{
  module_register( &ula_module_info );

  periph_register( PERIPH_TYPE_ULA, &ula_periph );
  periph_register( PERIPH_TYPE_ULA_FULL_DECODE, &ula_periph_full_decode );

  debugger_system_variable_register(
    debugger_type_string, last_byte_detail_string, get_last_byte, NULL );
  debugger_system_variable_register(
    debugger_type_string, tstates_detail_string, get_tstates, set_tstates );
  debugger_system_variable_register(
    debugger_type_string, mem7ffd_detail_string, get_7ffd, set_7ffd );
  debugger_system_variable_register(
    debugger_type_string, mem1ffd_detail_string, get_1ffd, set_1ffd );

  ula_default_value = 0xff;

  return 0;
}

void
ula_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_ULA, dependencies,
                            ARRAY_SIZE( dependencies ), ula_init, NULL,
                            NULL );
}

static libspectrum_byte
ula_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte r = ula_default_value;

  *attached = 0xff;

  loader_detect_loader();

  r &= phantom_typist_ula_read( port );

  r &= keyboard_read( port >> 8 );
  if( tape_microphone ) r ^= 0x40;

  return r;
}

/* What happens when we write to the ULA? */
static void
ula_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  last_byte = b;

  display_set_lores_border( b & 0x07 );
  sound_beeper( tstates,
                (!!(b & 0x10) << 1) + ( (!(b & 0x8)) | tape_microphone ) );

  /* FIXME: shouldn't really be using the memory capabilities here */

  if( machine_current->timex ) {

    ula_default_value = 0x5f;

  } else if( machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {

    ula_default_value = 0xbf;

  } else if( machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY || !settings_current.issue2 ) {

    /* 128K always acts like an Issue 3 */
    ula_default_value = b & 0x10 ? 0xff : 0xbf;

  } else {

    /* Issue 2 */
    ula_default_value = b & 0x18 ? 0xff : 0xbf;

  }

}

libspectrum_byte
ula_last_byte( void )
{
  return last_byte;
}

libspectrum_byte
ula_tape_level( void )
{
  return last_byte & 0x8;
}

static void
ula_from_snapshot( libspectrum_snap *snap )
{
  ula_write( 0x00fe, libspectrum_snap_out_ula( snap ) );
  tstates = libspectrum_snap_tstates( snap );
  settings_current.issue2 = libspectrum_snap_issue2( snap );
}

static void
ula_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_out_ula( snap, last_byte );
  libspectrum_snap_set_tstates( snap, tstates );
  libspectrum_snap_set_issue2( snap, settings_current.issue2 );
}  

void
ula_contend_port_early( libspectrum_word port )
{
  if( memory_map_read[ port >> MEMORY_PAGE_SIZE_LOGARITHM ].contended )
    tstates += ula_contention_no_mreq[ tstates ];
   
  tstates++;
}

void
ula_contend_port_late( libspectrum_word port )
{
  if( machine_current->ram.port_from_ula( port ) ) {

    tstates += ula_contention_no_mreq[ tstates ]; tstates += 2;

  } else {

    if( memory_map_read[ port >> MEMORY_PAGE_SIZE_LOGARITHM ].contended ) {
      tstates += ula_contention_no_mreq[ tstates ]; tstates++;
      tstates += ula_contention_no_mreq[ tstates ]; tstates++;
      tstates += ula_contention_no_mreq[ tstates ];
    } else {
      tstates += 2;
    }

  }
}
