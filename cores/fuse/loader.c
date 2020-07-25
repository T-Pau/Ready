/* loader.c: loader detection
   Copyright (c) 2006 Philip Kendall

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

#include "event.h"
#include "loader.h"
#include "memory_pages.h"
#include "rzx.h"
#include "settings.h"
#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"

static int successive_reads = 0;
static libspectrum_signed_dword last_tstates_read = -100000;
static libspectrum_byte last_b_read = 0x00;
static int length_known1 = 0, length_known2 = 0;
static int length_long1 = 0, length_long2 = 0;

typedef enum acceleration_mode_t {
  ACCELERATION_MODE_NONE = 0,
  ACCELERATION_MODE_INCREASING,
  ACCELERATION_MODE_DECREASING,
} acceleration_mode_t;

static acceleration_mode_t acceleration_mode;
static size_t acceleration_pc;

void
loader_frame( libspectrum_dword frame_length )
{
  if( last_tstates_read > -100000 ) {
    last_tstates_read -= frame_length;
  }
}

void
loader_tape_play( void )
{
  successive_reads = 0;
  acceleration_mode = ACCELERATION_MODE_NONE;
}

void
loader_tape_stop( void )
{
  successive_reads = 0;
  acceleration_mode = ACCELERATION_MODE_NONE;
}

static void
do_acceleration( void )
{
  if( length_known1 ) {
    /* B is used to indicate the length of the pulses */
    int set_b_high = length_long1;
    set_b_high ^= ( acceleration_mode == ACCELERATION_MODE_DECREASING );
    if( set_b_high ) {
      z80.bc.b.h = 0xfe;
    } else {
      z80.bc.b.h = 0x00;
    }

    /* Bit 5 of C is used to indicate the current microphone level */
    z80.bc.b.l = (z80.bc.b.l & ~0x20) | (tape_microphone ? 0x00 : 0x20);

    z80.af.b.l |= 0x01;

    /* Simulate the RET at the end of the edge-finding loop */
    z80.pc.b.l = readbyte_internal( z80.sp.w ); z80.sp.w++;
    z80.pc.b.h = readbyte_internal( z80.sp.w ); z80.sp.w++;

    event_remove_type( tape_edge_event );
    tape_next_edge( tstates, 1 );

    successive_reads = 0;
  }

  length_known1 = length_known2;
  length_long1 = length_long2;
}

static acceleration_mode_t
acceleration_detector( libspectrum_word pc )
{
  int state = 0, count = 0;
  while( 1 ) {
    libspectrum_byte b = readbyte_internal( pc ); pc++; count++;
    switch( state ) {
    case 0:
      switch( b ) {
      case 0x03: state = 28; break;     /* Data byte of JR NZ, ... - Alkatraz */
      case 0x04: state = 1; break;	/* INC B - Many loaders */
      default: state = 13; break;	/* Possible Digital Integration */
      }
      break;
    case 1:
      switch( b ) {
      case 0x20: state = 40; break;     /* JR NZ - variant Alkatraz */
      case 0xc8: state = 2; break;	/* RET Z */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 2:
      switch( b ) {
      case 0x3e: state = 3; break;	/* LD A,nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 3:
      switch( b ) {
      case 0x00:			/* Search Loader */
      case 0x7f:			/* ROM loader and variants */
      case 0xff:                        /* Dinaload */
	state = 4; break;		/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 4:
      switch( b ) {
      case 0xdb: state = 5; break;	/* IN A,(nn) */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 5:
      switch( b ) {
      case 0xfe: state = 6; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 6:
      switch( b ) {
      case 0x1f: state = 7; break;	/* RRA */
      case 0xa9: state = 24; break;	/* XOR C - Search Loader */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 7:
      switch( b ) {
      case 0x00:			/* NOP - Bleepload */
      case 0xa7:			/* AND A - Microsphere */
      case 0xc8:			/* RET Z - Paul Owens */
      case 0xd0:			/* RET NC - ROM loader */
	state = 8; break;
      case 0xa9: state = 9; break;	/* XOR C - Speedlock */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 8:
      switch( b ) {
      case 0xa9: state = 9; break;	/* XOR C */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 9:
      switch( b ) {
      case 0xe6: state = 10; break;	/* AND nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 10:
      switch( b ) {
      case 0x20: state = 11; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 11:
      switch( b ) {
      case 0x28: state = 12; break;	/* JR nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 12:
      if( b == 0x100 - count ) {
	return ACCELERATION_MODE_INCREASING;
      } else {
	return ACCELERATION_MODE_NONE;
      }
      break;

      /* Digital Integration loader */

    case 13:
      state = 14; break;		/* Possible Digital Integration */
    case 14:
      switch( b ) {
      case 0x05: state = 15; break;	/* DEC B - Digital Integration */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 15:
      switch( b ) {
      case 0xc8: state = 16; break;	/* RET Z */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 16:
      switch( b ) {
      case 0xdb: state = 17; break;	/* IN A,(nn) */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 17:
      switch( b ) {
      case 0xfe: state = 18; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 18:
      switch( b ) {
      case 0xa9: state = 19; break;	/* XOR C */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 19:
      switch( b ) {
      case 0xe6: state = 20; break;	/* AND nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 20:
      switch( b ) {
      case 0x40: state = 21; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 21:
      switch( b ) {
      case 0xca: state = 22; break;	/* JP Z,nnnn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 22:				/* LSB of jump target */
      if( b == ( z80.pc.w - 4 ) % 0x100 ) {
	state = 23;
      } else {
	return ACCELERATION_MODE_NONE;
      }
      break;
    case 23:				/* MSB of jump target */
      if( b == ( z80.pc.w - 4 ) / 0x100 ) {
	return ACCELERATION_MODE_DECREASING;
      } else {
	return ACCELERATION_MODE_NONE;
      }

      /* Search loader */

    case 24:
      switch( b ) {
      case 0xe6: state = 25; break;	/* AND nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 25:
      switch( b ) {
      case 0x40: state = 26; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 26:
      switch( b ) {
      case 0x28: state = 12; break;     /* JR Z - Space Crusade */
      case 0xd8: state = 27; break;	/* RET C */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 27:
      switch( b ) {
      case 0x00: state = 11; break;	/* NOP */
      default: return ACCELERATION_MODE_NONE;
      }
      break;

    /* Alkatraz */

    case 28:
      switch( b ) {
      case 0xc3: state = 29; break;     /* JP nnnn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 29:
      state = 30; break;                /* First data byte of JP */
    case 30:
      state = 31; break;                /* Second data byte of JP */
    case 31:
      switch( b ) {
      case 0xdb: state = 32; break;	/* IN A,(nn) */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 32:
      switch( b ) {
      case 0xfe: state = 33; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 33:
      switch( b ) {
      case 0x1f: state = 34; break;	/* RRA */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 34:
      switch( b ) {
      case 0xc8: state = 35; break;	/* RET Z */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 35:
      switch( b ) {
      case 0xa9: state = 36; break;	/* XOR C */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 36:
      switch( b ) {
      case 0xe6: state = 37; break;	/* AND nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 37:
      switch( b ) {
      case 0x20: state = 38; break;	/* Data byte */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 38:
      switch( b ) {
      case 0x28: state = 39; break;	/* JR Z,nn */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 39:
      switch( b ) {
      case 0xf1:                        /* Normal data byte */
      case 0xf3:                        /* Variant data byte */
        return ACCELERATION_MODE_INCREASING;
      default: return ACCELERATION_MODE_NONE;
      }
      break;

    /* "Variant" Alkatraz */

    case 40:
      switch( b ) {
      case 0x01: state = 41; break;     /* Data byte of JR NZ */
      default: return ACCELERATION_MODE_NONE;
      }
      break;
    case 41:
      switch( b ) {
      case 0xc9: state = 31; break;     /* RET */
      default: return ACCELERATION_MODE_NONE;
      }
      break;

    default:
      /* Can't happen */
      break;
    }
  }

}      

static void
check_for_acceleration( void )
{
  /* If the IN occured at a different location to the one we're
     accelerating, stop acceleration */
  if( acceleration_mode && z80.pc.w != acceleration_pc )
    acceleration_mode = ACCELERATION_MODE_NONE;

  /* If we're not accelerating, check if this is a loader */
  if( !acceleration_mode ) {
    acceleration_mode = acceleration_detector( z80.pc.w - 6 );
    acceleration_pc = z80.pc.w;
  }

  if( acceleration_mode ) do_acceleration();
}

void
loader_detect_loader( void )
{
  libspectrum_dword tstates_diff = tstates - last_tstates_read;
  libspectrum_byte b_diff = z80.bc.b.h - last_b_read;

  last_tstates_read = tstates;
  last_b_read = z80.bc.b.h;

  if( settings_current.detect_loader ) {

    if( tape_is_playing() ) {
      if( tstates_diff > 1000 || ( b_diff != 1 && b_diff != 0 &&
				   b_diff != 0xff ) ) {
	successive_reads++;
	if( successive_reads >= 2 ) {
	  tape_stop();
	}
      } else {
	successive_reads = 0;
      }
    } else {
      if( tstates_diff <= 500 && ( b_diff == 1 || b_diff == 0xff ) ) {
	successive_reads++;
	if( successive_reads >= 10 ) {
	  tape_do_play( 1 );
	}
      } else {
	successive_reads = 0;
      }
    }

  } else {

    successive_reads = 0;

  }

  if( settings_current.accelerate_loader && tape_is_playing() &&
      !rzx_recording )
    check_for_acceleration();

}

void
loader_set_acceleration_flags( int flags, int from_acceleration )
{
  if( flags & LIBSPECTRUM_TAPE_FLAGS_LENGTH_SHORT ) {
    length_known2 = 1;
    length_long2 = 0;
  } else if( flags & LIBSPECTRUM_TAPE_FLAGS_LENGTH_LONG ) {
    length_known2 = 1;
    length_long2 = 1;
  } else {
    length_known2 = 0;
  }

  /* If this tape edge occurred due to normal timings rather than
     our tape acceleration, turn off acceleration for the next edge
     or we miss an edge. See [bugs:#387] for more details */
  if( !from_acceleration ) {
    length_known1 = 0;
  }
}
