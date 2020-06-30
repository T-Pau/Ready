/* coretest.c: Test program for Fuse's Z80 core
   Copyright (c) 2003-2017 Philip Kendall

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuse.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/if1.h"
#include "peripherals/spectranet.h"
#include "peripherals/ula.h"
#include "peripherals/usource.h"
#include "profile.h"
#include "rzx.h"
#include "slt.h"
#include "tape.h"

#include "event.h"
#include "infrastructure/startup_manager.h"
#include "module.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80.h"
#include "z80_macros.h"

static const char *progname;		/* argv[0] */
static const char *testsfile;		/* argv[1] */

static int init_dummies( void );

libspectrum_dword tstates;
libspectrum_dword event_next_event;

/* 64Kb of RAM */
static libspectrum_byte initial_memory[ 0x10000 ], memory[ 0x10000 ];

libspectrum_byte readbyte( libspectrum_word address );
libspectrum_byte readbyte_internal( libspectrum_word address );

void writebyte( libspectrum_word address, libspectrum_byte b );
void writebyte_internal( libspectrum_word address, libspectrum_byte b );

static int run_test( FILE *f );
static int read_test( FILE *f, libspectrum_dword *end_tstates );

static void dump_z80_state( void );
static void dump_memory_state( void );

int
main( int argc, char **argv )
{
  FILE *f;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "Usage: %s <testsfile>\n", progname );
    return 1;
  }

  testsfile = argv[1];

  if( init_dummies() ) return 1;

  /* Initialise the tables used by the Z80 core */
  z80_init( NULL );

  f = fopen( testsfile, "r" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open tests file `%s': %s\n", progname,
	     testsfile, strerror( errno ) );
    return 1;
  }

  while( run_test( f ) ) {
    /* Do nothing */
  }

  if( fclose( f ) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, testsfile,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}

libspectrum_byte
readbyte( libspectrum_word address )
{
  printf( "%5d MC %04x\n", tstates, address );
  tstates += 3;
  return readbyte_internal( address );
}

libspectrum_byte
readbyte_internal( libspectrum_word address )
{
  printf( "%5d MR %04x %02x\n", tstates, address, memory[ address ] );
  return memory[ address ];
}

void
writebyte( libspectrum_word address, libspectrum_byte b )
{
  printf( "%5d MC %04x\n", tstates, address );
  tstates += 3;
  writebyte_internal( address, b );
}

void
writebyte_internal( libspectrum_word address, libspectrum_byte b )
{
  printf( "%5d MW %04x %02x\n", tstates, address, b );
  memory[ address ] = b;
}

void
contend_read( libspectrum_word address, libspectrum_dword time )
{
  printf( "%5d MC %04x\n", tstates, address );
  tstates += time;
}

void
contend_read_no_mreq( libspectrum_word address, libspectrum_dword time )
{
  contend_read( address, time );
}

void
contend_write_no_mreq( libspectrum_word address, libspectrum_dword time )
{
  printf( "%5d MC %04x\n", tstates, address );
  tstates += time;
}

static void
contend_port_preio( libspectrum_word port )
{
  if( ( port & 0xc000 ) == 0x4000 ) {
    printf( "%5d PC %04x\n", tstates, port );
  }

  tstates++;
}

static void
contend_port_postio( libspectrum_word port )
{
  if( port & 0x0001 ) {
    
    if( ( port & 0xc000 ) == 0x4000 ) {
      printf( "%5d PC %04x\n", tstates, port ); tstates++;
      printf( "%5d PC %04x\n", tstates, port ); tstates++;
      printf( "%5d PC %04x\n", tstates, port ); tstates++;
    } else {
      tstates += 3;
    }

  } else {

    printf( "%5d PC %04x\n", tstates, port ); tstates += 3;

  }
}

libspectrum_byte
readport( libspectrum_word port )
{
  libspectrum_byte r = port >> 8;

  contend_port_preio( port );

  printf( "%5d PR %04x %02x\n", tstates, port, r );

  contend_port_postio( port );

  return r;
}

void
writeport( libspectrum_word port, libspectrum_byte b )
{
  contend_port_preio( port );

  printf( "%5d PW %04x %02x\n", tstates, port, b );

  contend_port_postio( port );
}

static int
run_test( FILE *f )
{
  size_t i;

  /* Get ourselves into a known state */
  z80_reset( 1 ); tstates = 0;
  for( i = 0; i < 0x10000; i += 4 ) {
    memory[ i     ] = 0xde; memory[ i + 1 ] = 0xad;
    memory[ i + 2 ] = 0xbe; memory[ i + 3 ] = 0xef;
  }

  if( read_test( f, &event_next_event ) ) return 0;

  /* Grab a copy of the memory for comparison at the end */
  memcpy( initial_memory, memory, 0x10000 );

  z80_do_opcodes();

  /* And dump our final state */
  dump_z80_state();
  dump_memory_state();

  printf( "\n" );

  return 1;
}

static int
read_test( FILE *f, libspectrum_dword *end_tstates )
{
  unsigned af, bc, de, hl, af_, bc_, de_, hl_, ix, iy, sp, pc, memptr;
  unsigned i, r, iff1, iff2, im;
  unsigned end_tstates2;
  unsigned address;
  char test_name[ 80 ];

  do {

    if( !fgets( test_name, sizeof( test_name ), f ) ) {

      if( feof( f ) ) return 1;

      fprintf( stderr, "%s: error reading test description from `%s': %s\n",
	       progname, testsfile, strerror( errno ) );
      return 1;
    }

  } while( test_name[0] == '\n' );

  /* FIXME: how should we read/write our data types? */
  if( fscanf( f, "%x %x %x %x %x %x %x %x %x %x %x %x %x", &af, &bc,
	      &de, &hl, &af_, &bc_, &de_, &hl_, &ix, &iy, &sp, &pc,
	      &memptr ) != 13 ) {
    fprintf( stderr, "%s: first registers line in `%s' corrupt\n", progname,
	     testsfile );
    return 1;
  }

  AF  = af;  BC  = bc;  DE  = de;  HL  = hl;
  AF_ = af_; BC_ = bc_; DE_ = de_; HL_ = hl_;
  IX  = ix;  IY  = iy;  SP  = sp;  PC  = pc;
  z80.memptr.w = memptr;

  if( fscanf( f, "%x %x %u %u %u %d %d", &i, &r, &iff1, &iff2, &im,
	      &z80.halted, &end_tstates2 ) != 7 ) {
    fprintf( stderr, "%s: second registers line in `%s' corrupt\n", progname,
	     testsfile );
    return 1;
  }

  I = i; R = R7 = r; IFF1 = iff1; IFF2 = iff2; IM = im;
  *end_tstates = end_tstates2;

  while( 1 ) {

    if( fscanf( f, "%x", &address ) != 1 ) {
      fprintf( stderr, "%s: no address found in `%s'\n", progname, testsfile );
      return 1;
    }

    if( address >= 0x10000 ) break;

    while( 1 ) {

      unsigned byte;

      if( fscanf( f, "%x", &byte ) != 1 ) {
	fprintf( stderr, "%s: no data byte found in `%s'\n", progname,
		 testsfile );
	return 1;
      }
    
      if( byte >= 0x100 ) break;

      memory[ address++ ] = byte;

    }
  }

  printf( "%s", test_name );

  return 0;
}

static void
dump_z80_state( void )
{
  printf( "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
	  AF, BC, DE, HL, AF_, BC_, DE_, HL_, IX, IY, SP, PC, z80.memptr.w );
  printf( "%02x %02x %d %d %d %d %d\n", I, ( R7 & 0x80 ) | ( R & 0x7f ),
	  IFF1, IFF2, IM, z80.halted, tstates );
}

static void
dump_memory_state( void )
{
  size_t i;

  for( i = 0; i < 0x10000; i++ ) {

    if( memory[ i ] == initial_memory[ i ] ) continue;

    printf( "%04x ", (unsigned)i );

    while( i < 0x10000 && memory[ i ] != initial_memory[ i ] )
      printf( "%02x ", memory[ i++ ] );

    printf( "-1\n" );
  }
}

/* Error 'handing': dump core as these should never be called */

void
fuse_abort( void )
{
  abort();
}

int
ui_error( ui_error_level severity GCC_UNUSED, const char *format, ... )
{
  va_list ap;

  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );

  abort();
}

/*
 * Stuff below here not interesting: dummy functions and variables to replace
 * things used by Fuse, but not by the core test code
 */

#include "debugger/debugger.h"
#include "machine.h"
#include "peripherals/scld.h"
#include "settings.h"

libspectrum_byte *slt[256];
size_t slt_length[256];

int
tape_load_trap( void )
{
  /* Should never be called */
  abort();
}

int
tape_save_trap( void )
{
  /* Should never be called */
  abort();
}

scld scld_last_dec;

size_t rzx_instruction_count;
int rzx_playback;
int rzx_instructions_offset;

enum debugger_mode_t debugger_mode;

libspectrum_byte **ROM = NULL;
memory_page memory_map[8];
memory_page *memory_map_home[MEMORY_PAGES_IN_64K];
memory_page memory_map_rom[SPECTRUM_ROM_PAGES * MEMORY_PAGES_IN_16K];
int memory_contended[8] = { 1 };
libspectrum_byte spectrum_contention[ 80000 ] = { 0 };
int profile_active = 0;

void
profile_map( libspectrum_word pc GCC_UNUSED )
{
  abort();
}

int
debugger_check( debugger_breakpoint_type type GCC_UNUSED, libspectrum_dword value GCC_UNUSED )
{
  abort();
}

void debugger_system_variable_register(
  const char *type, const char *detail,
  debugger_get_system_variable_fn_t get,
  debugger_set_system_variable_fn_t set )
{
}

int
debugger_trap( void )
{
  abort();
}

int
slt_trap( libspectrum_word address GCC_UNUSED, libspectrum_byte level GCC_UNUSED )
{
  return 0;
}

int beta_available = 0;
int beta_active = 0;
int if1_available = 0;

void
beta_page( void )
{
  abort();
}

void
beta_unpage( void )
{
  abort();
}

int spectrum_frame_event = 0;

int
event_register( event_fn_t fn GCC_UNUSED, const char *string GCC_UNUSED )
{
  return 0;
}

int opus_available = 0;
int opus_active = 0;

void
opus_page( void )
{
  abort();
}

void
opus_unpage( void )
{
  abort();
}

int plusd_available = 0;
int plusd_active = 0;

void
plusd_page( void )
{
  abort();
}

int disciple_available = 0;
int disciple_active = 0;

void
disciple_page( void )
{
  abort();
}

int didaktik80_available = 0;
int didaktik80_active = 0;
int didaktik80_snap = 0;

void
didaktik80_page( void )
{
  abort();
}

void
didaktik80_unpage( void )
{
  abort();
}

int usource_available = 0;
int usource_active = 0;

void
usource_toggle( void )
{
  abort();
}

void
if1_page( void )
{
  abort();
}

void
if1_unpage( void )
{
  abort();
}

int multiface_activated = 0;

void
multiface_setic8( void )
{
  abort();
}

void
divide_set_automap( int state GCC_UNUSED )
{
  abort();
}

void
divmmc_set_automap( int state GCC_UNUSED )
{
  abort();
}

int spectranet_available = 0;

void
spectranet_page( int via_io GCC_UNUSED )
{
  abort();
}

void
spectranet_nmi( void )
{
  abort();
}

void
spectranet_unpage( void )
{
  abort();
}

void
spectranet_retn( void )
{
}

int
spectranet_nmi_flipflop( void )
{
  return 0;
}

void
startup_manager_register( startup_manager_module module,
  startup_manager_module *dependencies, size_t dependency_count,
  startup_manager_init_fn init_fn, void *init_context,
  startup_manager_end_fn end_fn )
{
}

int svg_capture_active = 0;     /* SVG capture enabled? */

void
svg_capture( void )
{
  abort();
}

int
rzx_frame( void )
{
  abort();
}

void
writeport_internal( libspectrum_word port GCC_UNUSED, libspectrum_byte b GCC_UNUSED )
{
  abort();
}

void
event_add_with_data( libspectrum_dword event_time GCC_UNUSED,
		     int type GCC_UNUSED, void *user_data GCC_UNUSED )
{
  /* Do nothing */
}

int
module_register( module_info_t *module GCC_UNUSED )
{
  return 0;
}

void
z80_debugger_variables_init( void )
{
}

fuse_machine_info *machine_current;
static fuse_machine_info dummy_machine;

settings_info settings_current;

libspectrum_word beta_pc_mask;
libspectrum_word beta_pc_value;

int spectranet_programmable_trap_active;
libspectrum_word spectranet_programmable_trap;

/* Initialise the dummy variables such that we're running on a clean a
   machine as possible */
static int
init_dummies( void )
{
  size_t i;

  for( i = 0; i < 8; i++ ) {
    memory_map[i].page = &memory[ i * MEMORY_PAGE_SIZE ];
  }

  debugger_mode = DEBUGGER_MODE_INACTIVE;
  dummy_machine.capabilities = 0;
  dummy_machine.ram.current_rom = 0;
  machine_current = &dummy_machine;
  rzx_playback = 0;
  scld_last_dec.name.intdisable = 0;
  settings_current.slt_traps = 0;
  settings_current.divide_enabled = 0;
  settings_current.divmmc_enabled = 0;
  settings_current.z80_is_cmos = 0;
  beta_pc_mask = 0xfe00;
  beta_pc_value = 0x3c00;
  spectranet_programmable_trap_active = 0;
  spectranet_programmable_trap = 0x0000;

  return 0;
}
