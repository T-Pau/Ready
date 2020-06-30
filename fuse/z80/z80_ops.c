/* z80_ops.c: Process the next opcode
   Copyright (c) 1999-2005 Philip Kendall, Witold Filipczyk
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Gergely Szasz
   Copyright (c) 2015 Sergio Baldoví

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

#include "debugger/debugger.h"
#include "event.h"
#include "machine.h"
#include "memory_pages.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/if1.h"
#include "peripherals/multiface.h"
#include "peripherals/spectranet.h"
#include "peripherals/ula.h"
#include "peripherals/usource.h"
#include "profile.h"
#include "rzx.h"
#include "settings.h"
#include "slt.h"
#include "svg.h"
#include "tape.h"
#include "z80.h"

#include "z80_macros.h"

#ifndef HAVE_ENOUGH_MEMORY
static int z80_cbxx( libspectrum_byte opcode2 );
static int z80_ddxx( libspectrum_byte opcode2 );
static int z80_edxx( libspectrum_byte opcode2 );
static int z80_fdxx( libspectrum_byte opcode2 );
static void z80_ddfdcbxx( libspectrum_byte opcode3 );
#endif				/* #ifndef HAVE_ENOUGH_MEMORY */

/* Certain features (eg RZX playback trigged interrupts, the debugger,
   TR-DOS ROM paging) can't be handled within the normal 'events'
   framework as they don't happen at a specified tstate. In order to
   support these, we basically need to check every opcode as to
   whether they have occurred or not.

   There are (fairly common) circumstances under which we know that
   the features will never occur (eg we will never get an interrupt
   from RZX playback unless we're doing RZX playback), and we would
   quite like to skip the check in this state. We can do this if we
   use gcc's computed goto feature[1]. What follows is some
   preprocessor hackery to moderately transparently do this while
   still retaining the "normal" behaviour for non-gcc compilers.

   Ensure that the same arguments are given to respective
   SETUP_CHECK() and CHECK() macros or everything will break.

   [1] see 'C Extensions', 'Labels as Values' in the gcc info page.
*/

#ifdef __GNUC__

#define SETUP_CHECK( label, condition ) \
  pos_##label,
#define SETUP_NEXT( label )

enum {
#include "z80_checks.h"
  numchecks
};

#define CHECK( label, condition ) goto *cgoto[ pos_##label ]; label:
#define END_CHECK

#else				/* #ifdef __GNUC__ */

#define CHECK( label, condition ) if( condition ) {
#define END_CHECK }

#endif				/* #ifdef __GNUC__ */

#ifndef HAVE_ENOUGH_MEMORY
static libspectrum_byte opcode = 0x00;
#endif

/* Execute Z80 opcodes until the next event */
void
z80_do_opcodes( void )
{
#ifdef HAVE_ENOUGH_MEMORY
  libspectrum_byte opcode = 0x00;
#endif
  libspectrum_byte last_Q;

  int even_m1 =
    machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_EVEN_M1; 

#ifdef __GNUC__

#undef SETUP_CHECK
#define SETUP_CHECK( label, condition ) \
  if( condition ) { cgoto[ next ] = &&label; next = pos_##label + 1; } \
  check++;

#undef SETUP_NEXT
#define SETUP_NEXT( label ) \
  if( next != check ) { cgoto[ next ] = &&label; } \
  next = check;

  void *cgoto[ numchecks ]; size_t next = 0; size_t check = 0;

#include "z80_checks.h"

#endif				/* #ifdef __GNUC__ */

  while( tstates < event_next_event ) {

    /* Profiler */
    CHECK( profile, profile_active )

    profile_map( PC );

    END_CHECK

    /* If we're due an end of frame from RZX playback, generate one */
    CHECK( rzx, rzx_playback )

    if( R + rzx_instructions_offset >= rzx_instruction_count ) {
      event_add( tstates, spectrum_frame_event );
      break;		/* And break out of the execution loop to let
			   the interrupt happen */
    }

    END_CHECK

    /* Check if the debugger should become active at this point */
    CHECK( debugger, debugger_mode != DEBUGGER_MODE_INACTIVE )

    if( debugger_check( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC ) )
      debugger_trap();

    END_CHECK

    CHECK( beta, beta_available )

#define NOT_128_TYPE_OR_IS_48_TYPE ( !( machine_current->capabilities & \
            LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) || \
            machine_current->ram.current_rom )

    if( beta_active ) {
      if( NOT_128_TYPE_OR_IS_48_TYPE && PC >= 16384 ) {
	beta_unpage();
      }
    } else if( ( PC & beta_pc_mask ) == beta_pc_value &&
               NOT_128_TYPE_OR_IS_48_TYPE ) {
      beta_page();
    }

    END_CHECK

    CHECK( plusd, plusd_available )

    if( PC == 0x0008 || PC == 0x003a || PC == 0x0066 || PC == 0x028e ) {
      plusd_page();
    }

    END_CHECK

    CHECK( didaktik80, didaktik80_available )

    if( PC == 0x0000 || PC == 0x0008 ) {
      didaktik80_page();
    } else if( PC == 0x1700 ) {
      didaktik80_unpage();
    }

    END_CHECK

    CHECK( disciple, disciple_available )

    if( PC == 0x0001 || PC == 0x0008 || PC == 0x0066 || PC == 0x028e ) {
      disciple_page();
    }

    END_CHECK

    CHECK( usource, usource_available )

    if( PC == 0x2bae ) {
      usource_toggle();
    }

    END_CHECK

    CHECK( multiface, multiface_activated )

    if( PC == 0x0066 ) {
      multiface_setic8();
    }

    END_CHECK

    CHECK( if1p, if1_available )

    if( PC == 0x0008 || PC == 0x1708 ) {
      if1_page();
    }

    END_CHECK

    CHECK( divide_early, settings_current.divide_enabled )
    
    if( ( PC & 0xff00 ) == 0x3d00 ) {
      divide_set_automap( 1 );
    }
    
    END_CHECK

    CHECK( divmmc_early, settings_current.divmmc_enabled )
    
    if( ( PC & 0xff00 ) == 0x3d00 ) {
      divmmc_set_automap( 1 );
    }
    
    END_CHECK

    CHECK( spectranet_page, spectranet_available && !settings_current.spectranet_disable )

    if( PC == 0x0008 || ((PC & 0xfff8) == 0x3ff8) )
      spectranet_page( 0 );

    if( PC == spectranet_programmable_trap &&
      spectranet_programmable_trap_active )
      event_add( 0, z80_nmi_event );

    END_CHECK

  opcode_delay:

    contend_read( PC, 4 );

    /* Check to see if M1 cycles happen on even tstates */
    CHECK( evenm1, even_m1 )

    if( tstates & 1 ) {
      if( ++tstates == event_next_event ) {
	break;
      }
    }

    END_CHECK

  run_opcode:
    /* Do the instruction fetch; readbyte_internal used here to avoid
       triggering read breakpoints */
    opcode = readbyte_internal( PC );

    CHECK( if1u, if1_available )

    if( PC == 0x0700 ) {
      if1_unpage();
    }

    END_CHECK

    CHECK( divide_late, settings_current.divide_enabled )

    if( ( PC & 0xfff8 ) == 0x1ff8 ) {
      divide_set_automap( 0 );
    } else if( (PC == 0x0000) || (PC == 0x0008) || (PC == 0x0038)
      || (PC == 0x0066) || (PC == 0x04c6) || (PC == 0x0562) ) {
      divide_set_automap( 1 );
    }
    
    END_CHECK

    CHECK( divmmc_late, settings_current.divmmc_enabled )

    if( ( PC & 0xfff8 ) == 0x1ff8 ) {
      divmmc_set_automap( 0 );
    } else if( (PC == 0x0000) || (PC == 0x0008) || (PC == 0x0038)
      || (PC == 0x0066) || (PC == 0x04c6) || (PC == 0x0562) ) {
      divmmc_set_automap( 1 );
    }
    
    END_CHECK

    CHECK( opus, opus_available )

    if( opus_active ) {
      if( PC == 0x1748 ) {
        opus_unpage();
      }
    } else if( PC == 0x0008 || PC == 0x0048 || PC == 0x1708 ) {
      opus_page();
    }

    END_CHECK

    CHECK( spectranet_unpage, spectranet_available )

    if( PC == 0x007c )
      spectranet_unpage();

    END_CHECK

    CHECK( z80_iff2_read, z80.iff2_read )

    z80.iff2_read = 0;
    /* Execute *one* instruction before reevaluating the checks */
    event_add( tstates, z80_nmos_iff2_event );

    END_CHECK

    CHECK( didaktik80snap, didaktik80_snap )

    if( PC == 0x0066 && !didaktik80_active ) {
      opcode = 0xc7;	/* RST 00 */
      didaktik80_snap = 0; /* FIXME: this should be a time-based reset */
    }

    END_CHECK

    CHECK( svg_capture, svg_capture_active )

    svg_capture();

    END_CHECK

  end_opcode:
    PC++; R++;
    last_Q = Q; /* keep Q value from previous opcode for SCF and CCF */
    Q = 0;      /* preempt Q value assuming next opcode doesn't set flags */

    switch(opcode) {
#include "z80/opcodes_base.c"
    }

  }

}

#ifndef HAVE_ENOUGH_MEMORY

static int
z80_cbxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80/z80_cb.c"
  }
  return 0;
}

static int
z80_ddxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80/z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
  return 0;
}

static int
z80_edxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80/z80_ed.c"
  }
  return 0;
}

static int
z80_fdxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80/z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
  return 0;
}

static void
z80_ddfdcbxx( libspectrum_byte opcode3 )
{
  switch(opcode3) {
#include "z80/z80_ddfdcb.c"
  }
}

#endif			/* #ifndef HAVE_ENOUGH_MEMORY */
