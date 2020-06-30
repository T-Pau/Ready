/* disassemble.c: Fuse's disassembler
   Copyright (c) 2002-2015 Darren Salt, Philip Kendall
   Copyright (c) 2016 BogDan Vatra

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

   Darren: linux@youmustbejoking.demon.co.uk

   Philip: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <libspectrum.h>

#include "debugger.h"
#include "fuse.h"
#include "memory_pages.h"
#include "ui/ui.h"

/* Used to flag whether we're after a DD or FD prefix */
enum hl_type { USE_HL, USE_IX, USE_IY };

static void disassemble_main( libspectrum_word address, char *buffer,
			      size_t buflen, size_t *length,
			      enum hl_type use_hl );
static void disassemble_00xxxxxx( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_00xxx010( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_00xxx110( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_11xxxxxx( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_11xxx001( libspectrum_byte b, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_11xxx011( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_11xxx101( libspectrum_word address, char *buffer,
				  size_t buflen, size_t *length,
				  enum hl_type use_hl );
static void disassemble_cb( libspectrum_word address, char *buffer,
			    size_t buflen, size_t *length );
static void disassemble_ed( libspectrum_word address, char *buffer,
			    size_t buflen, size_t *length );
static void disassemble_ddfd_cb( libspectrum_word address, char offset,
				 enum hl_type use_hl, char *buffer,
				 size_t buflen, size_t *length );

static void get_byte( char *buffer, size_t buflen, libspectrum_byte b );
static void get_word( char *buffer, size_t buflen, libspectrum_word address );
static void get_offset( char *buffer, size_t buflen, libspectrum_word address,
			libspectrum_byte offset );

static const char *reg_pair( libspectrum_byte b, enum hl_type use_hl );
static const char *hl_ix_iy( enum hl_type use_hl );
static void ix_iy_offset( char *buffer, size_t buflen, enum hl_type use_hl,
			  libspectrum_byte offset );

static int source_reg( libspectrum_word address, enum hl_type use_hl,
		       char *buffer, size_t buflen );
static int dest_reg( libspectrum_word address, enum hl_type use_hl,
		     char *buffer, size_t buflen );
static int single_reg( int i, enum hl_type use_hl, libspectrum_byte offset,
		       char *buffer, size_t buflen );

static const char *addition_op( libspectrum_byte b );
static const char *condition( libspectrum_byte b );
static const char *rotate_op( libspectrum_byte b );
static const char *bit_op( libspectrum_byte b );
static int bit_op_bit( libspectrum_byte b );

/* A very thin wrapper to avoid exposing the USE_HL constant */
void
debugger_disassemble( char *buffer, size_t buflen, size_t *length,
		      libspectrum_word address )
{
  disassemble_main( address, buffer, buflen, length, USE_HL );
}

/* Disassemble one instruction */
static void
disassemble_main( libspectrum_word address, char *buffer, size_t buflen,
		  size_t *length, enum hl_type use_hl )
{
  libspectrum_byte b;
  char buffer2[40], buffer3[40];
  size_t prefix_length = 0;

  b = readbyte_internal( address );

  /* Before we do anything else, strip off any DD or FD prefixes, keeping
     a count of how many we've seen */
  while( b == 0xdd || b == 0xfd ) {
    use_hl = b == 0xdd ? USE_IX : USE_IY;
    address++;
    prefix_length++;
    b = readbyte_internal( address );
  }

  if( b < 0x40 ) {
    disassemble_00xxxxxx( address, buffer, buflen, length, use_hl );
  } else if( b == 0x76 ) {
    snprintf( buffer, buflen, "HALT" ); *length = 1;
  } else if( b < 0x80 ) {

    if( ( b & 0x07 ) == 0x06 ) {		 /* LD something,(HL) */
      dest_reg( address, USE_HL, buffer2, 40 );
      source_reg( address, use_hl, buffer3, 40 );
      *length = ( use_hl == USE_HL ? 1 : 2 );
    } else if( ( ( b >> 3 ) & 0x07 ) == 0x06 ) { /* LD (HL),something */
      dest_reg( address, use_hl, buffer2, 40 );
      source_reg( address, USE_HL, buffer3, 40 );
      *length = ( use_hl == USE_HL ? 1 : 2 );
    } else {				/* Does not involve (HL) at all */
      dest_reg( address, use_hl, buffer2, 40 );
      source_reg( address, use_hl, buffer3, 40 );
      *length = 1;
    }
    /* Note LD (HL),(HL) does not exist */

    snprintf( buffer, buflen, "LD %s,%s", buffer2, buffer3 );

  } else if( b < 0xc0 ) {
    *length = 1 + source_reg( address, use_hl, buffer2, 40 );
    snprintf( buffer, buflen, addition_op( b ), buffer2 );
  } else {
    disassemble_11xxxxxx( address, buffer, buflen, length, use_hl );
  }

  /* Increment the instruction length by the number of prefix bytes */
  *length += prefix_length;
}

/* Disassemble something of the form 00xxxxxx */
static void
disassemble_00xxxxxx( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  const char *opcode_00xxx000[] = {
    "NOP", "EX AF,AF'", "DJNZ ", "JR ", "JR NZ,", "JR Z,", "JR NC,", "JR C,"
  };
  const char *opcode_00xxx111[] = {
    "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF"
  };
  char buffer2[40], buffer3[40];

  libspectrum_byte b = readbyte_internal( address );

  switch( b & 0x0f ) {

  case 0x00: case 0x08:
    if( b <= 0x08 ) {
      snprintf( buffer, buflen, "%s", opcode_00xxx000[ b >> 3 ] ); *length = 1;
    } else {
      get_offset( buffer2, 40, address + 2, readbyte_internal( address + 1 ) );
      snprintf( buffer, buflen, "%s%s", opcode_00xxx000[ b >> 3 ], buffer2 );
      *length = 2;
    }
    break;

  case 0x01:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "LD %s,%s", reg_pair( b, use_hl ), buffer2 );
    *length = 3;
    break;

  case 0x02:
    disassemble_00xxx010( address, buffer, buflen, length, use_hl );
    break;

  case 0x03:
    snprintf( buffer, buflen, "INC %s", reg_pair( b, use_hl ) ); *length = 1;
    break;

  case 0x04: case 0x0c:
    *length = 1 + dest_reg( address, use_hl, buffer2, 40 );
    snprintf( buffer, buflen, "INC %s", buffer2 );
    break;

  case 0x05: case 0x0d:
    *length = 1 + dest_reg( address, use_hl, buffer2, 40 );
    snprintf( buffer, buflen, "DEC %s", buffer2 );
    break;

  case 0x06: case 0x0e:
    *length = 2 + dest_reg( address, use_hl, buffer2, 40 );
    get_byte( buffer3, 40, readbyte_internal( address + *length - 1 ) );
    snprintf( buffer, buflen, "LD %s,%s", buffer2, buffer3 );
    break;

  case 0x07: case 0x0f:
    snprintf( buffer, buflen, "%s", opcode_00xxx111[ b >> 3 ] ); *length = 1;
    break;

  case 0x09:
    snprintf( buffer, buflen, "ADD %s,%s", hl_ix_iy( use_hl ),
	      reg_pair( b, use_hl ) );
    *length = 1;
    break;

  case 0x0a:
    disassemble_00xxx110( address, buffer, buflen, length, use_hl );
    break;

  case 0x0b:
    snprintf( buffer, buflen, "DEC %s", reg_pair( b, use_hl ) );
    *length = 1;
    break;

  }
}

/* Disassemble something of the form 00xxx010 */
static void
disassemble_00xxx010( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  switch( b >> 4 ) {

  case 0: case 1: 
    snprintf( buffer, buflen, "LD (%s),A", reg_pair( b, use_hl ) );
    *length = 1;
    break;

  case 2:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "LD (%s),%s", buffer2, hl_ix_iy( use_hl ) );
    *length = 3;
    break;

  case 3:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "LD (%s),A", buffer2 ); *length = 3;
    break;
  }
}

/* Disassemble something of the form 00xxx110 */
static void
disassemble_00xxx110( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  switch( b >> 4 ) {

  case 0: case 1: 
    snprintf( buffer, buflen, "LD A,(%s)", reg_pair( b, use_hl ) );
    *length = 1;
    break;

  case 2:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "LD %s,(%s)", hl_ix_iy( use_hl ), buffer2 );
    *length = 3;
    break;

  case 3:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "LD A,(%s)", buffer2 ); *length = 3;
    break;
  }
}
  
/* Disassemble something of the form 11xxxxxx */
static void
disassemble_11xxxxxx( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  switch( b & 0x07 ) {

  case 0x00:
    snprintf( buffer, buflen, "RET %s", condition( b ) ); *length = 1;
    break;

  case 0x01:
    disassemble_11xxx001( b, buffer, buflen, length, use_hl );
    break;

  case 0x02:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "JP %s,%s", condition( b ), buffer2 );
    *length = 3;
    break;
      
  case 0x03:
    disassemble_11xxx011( address, buffer, buflen, length, use_hl );
    break;

  case 0x04:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "CALL %s,%s", condition( b ), buffer2 );
    *length = 3;
    break;

  case 0x05:
    disassemble_11xxx101( address, buffer, buflen, length, use_hl );
    break;

  case 0x06:
    get_byte( buffer2, 40, readbyte_internal( address + 1 ) );
    snprintf( buffer, buflen, addition_op( b ), buffer2 );
    *length = 2;
    break;

  case 0x07:
    snprintf( buffer, buflen, "RST %X", 0x08 * ( ( b >> 3 ) - 0x18 ) );
    *length = 1;
    break;
  }
}

/* Disassemble something for the form 11xxx001 */
static void
disassemble_11xxx001( libspectrum_byte b, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  switch( ( b >> 3 ) - 0x18 ) {
    
  case 0x00: case 0x02: case 0x04:
    snprintf( buffer, buflen, "POP %s", reg_pair( b, use_hl ) ); *length = 1;
    break;

  case 0x01: snprintf( buffer, buflen, "RET" ); *length = 1; break;
  case 0x03: snprintf( buffer, buflen, "EXX" ); *length = 1; break;

  case 0x05: 
    snprintf( buffer, buflen, "JP (%s)", hl_ix_iy( use_hl ) ); *length = 1;
    break;

  case 0x06: snprintf( buffer, buflen, "POP AF" ); *length = 1; break;

  case 0x07:
    snprintf( buffer, buflen, "LD SP,%s", hl_ix_iy( use_hl ) ); *length = 1;
    break;
  }
}

/* Disassemble something for the form 11xxx011 */
static void
disassemble_11xxx011( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  switch( ( b >> 3 ) - 0x18 ) {

  case 0x00:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "JP %s", buffer2 ); *length = 3;
    break;

  case 0x01:
    if( use_hl != USE_HL ) {
      char offset = readbyte_internal( address + 1 );
      disassemble_ddfd_cb( address+2, offset, use_hl, buffer, buflen,
			   length );
      (*length) += 2;
    } else {
      disassemble_cb( address+1, buffer, buflen, length ); (*length)++;
    }
    break;

  case 0x02:
    get_byte( buffer2, 40, readbyte_internal( address + 1 ) );
    snprintf( buffer, buflen, "OUT (%s),A", buffer2 ); *length = 2;
    break;

  case 0x03:
    get_byte( buffer2, 40, readbyte_internal( address + 1 ) );
    snprintf( buffer, buflen, "IN A,(%s)", buffer2 ); *length = 2;
    break;

  case 0x04:
    snprintf( buffer, buflen, "EX (SP),%s", hl_ix_iy( use_hl ) ); *length = 1;
    break;

  case 0x05:
    /* Note: does not get modified by DD or FD */
    snprintf( buffer, buflen, "EX DE,HL" ); *length = 1;
    break;

  case 0x06:
    snprintf( buffer, buflen, "DI" ); *length = 1;
    break;

  case 0x07:
    snprintf( buffer, buflen, "EI" ); *length = 1;
    break;
  }
}

/* Disassemble something for the form 11xxx101 */
static void
disassemble_11xxx101( libspectrum_word address, char *buffer, size_t buflen,
		      size_t *length, enum hl_type use_hl )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  switch( ( b >> 3 ) - 0x18 ) {
	
  case 0x00: case 0x02: case 0x04:
    snprintf( buffer, buflen, "PUSH %s", reg_pair( b, use_hl ) ); *length = 1;
    break;

  case 0x01:
    get_word( buffer2, 40, address + 1 );
    snprintf( buffer, buflen, "CALL %s", buffer2 ); *length = 3;
    break;

  case 0x03:
  case 0x07:
    /* These should never happen as we strip off all DD/FD prefixes before
     * disassembling the instruction itself */
    ui_error( UI_ERROR_ERROR, "disassemble_11xx101: b = 0x%02x", b );
    fuse_abort();
    break;

  case 0x05:
    disassemble_ed( address+1, buffer, buflen, length ); (*length)++;
    break;

  case 0x06:
    snprintf( buffer, buflen, "PUSH AF" ); *length = 1;
    break;
  }
}

/* Disassemble an instruction after a CB prefix */
static void
disassemble_cb( libspectrum_word address, char *buffer, size_t buflen,
		size_t *length )
{
  char buffer2[40];
  libspectrum_byte b = readbyte_internal( address );

  source_reg( address, USE_HL, buffer2, 40 );

  if( b < 0x40 ) {
    snprintf( buffer, buflen, "%s %s", rotate_op( b ), buffer2 );
    *length = 1;
  } else {
    snprintf( buffer, buflen, "%s %d,%s", bit_op( b ), bit_op_bit( b ),
	      buffer2 );
    *length = 1;
  }
}

/* Disassemble an instruction after an ED prefix */
static void
disassemble_ed( libspectrum_word address, char *buffer, size_t buflen,
		size_t *length )
{
  libspectrum_byte b;
  char buffer2[40];

  const char *opcode_01xxx111[] = {
    "LD I,A", "LD R,A", "LD A,I", "LD A,R", "RRD", "RLD", "NOPD", "NOPD"
  };

  /* Note 0xbc to 0xbf removed before this table is used */
  const char *opcode_101xxxxx[] = {
    "LDI",  "CPI",  "INI",  "OUTI", "NOPD", "NOPD", "NOPD", "NOPD",
    "LDD",  "CPD",  "IND",  "OUTD", "NOPD", "NOPD", "NOPD", "NOPD",
    "LDIR", "CPIR", "INIR", "OTIR", "NOPD", "NOPD", "NOPD", "NOPD",
    "LDDR", "CPDR", "INDR", "OTDR"
  };

  /* The order in which the IM x instructions appear */
  const int im_modes[] = { 0, 0, 1, 2 };

  b = readbyte_internal( address );

  if( b < 0x40 || b > 0xbb ) {
    snprintf( buffer, buflen, "NOPD" ); *length = 1;
  } else if( b < 0x80 ) {

    switch( b & 0x0f ) {

    case 0x00: case 0x08:
      if( b == 0x70 ) {
	snprintf( buffer, buflen, "IN F,(C)" ); *length = 1;
      } else {
	dest_reg( address, USE_HL, buffer2, 40 );
	snprintf( buffer, buflen, "IN %s,(C)", buffer2 ); *length = 1;
      }
      break;

    case 0x01: case 0x09:
      if( b == 0x71 ) {
	snprintf( buffer, buflen, "OUT (C),0" ); *length = 1;
      } else {
	dest_reg( address, USE_HL, buffer2, 40 );
	snprintf( buffer, buflen, "OUT (C),%s", buffer2 ); *length = 1;
      }
      break;

    case 0x02:
      snprintf( buffer, buflen, "SBC HL,%s", reg_pair( b, USE_HL ) );
      *length = 1;
      break;

    case 0x03:
      get_word( buffer2, 40, address + 1 );
      snprintf( buffer, buflen, "LD (%s),%s", buffer2, reg_pair( b, USE_HL ) );
      *length = 3;
      break;

    case 0x04: case 0x0c:
      snprintf( buffer, buflen, "NEG" ); *length = 1;
      break;

    case 0x05: case 0x0d:
      if( b == 0x4d ) {
	snprintf( buffer, buflen, "RETI" ); *length = 1;
      } else {
	snprintf( buffer, buflen, "RETN" ); *length = 1;
      }
      break;

    case 0x06: case 0x0e:
      snprintf( buffer, buflen, "IM %d", im_modes[ ( b >> 3 ) & 0x03 ] );
      *length = 1;
      break;

    case 0x07: case 0x0f:
      snprintf( buffer, buflen, "%s", opcode_01xxx111[ ( b >> 3 ) & 0x07 ] );
      *length = 1;
      break;

    case 0x0a:
      snprintf( buffer, buflen, "ADC HL,%s", reg_pair( b, USE_HL ) );
      *length = 1;
      break;

    case 0x0b:
      get_word( buffer2, 40, address + 1 );
      snprintf( buffer, buflen, "LD %s,(%s)", reg_pair( b, USE_HL ), buffer2 );
      *length = 3;
      break;

    }
  } else if( b < 0xa0 ) {
    snprintf( buffer, buflen, "NOPD" ); *length = 1;
  } else {
    /* Note: 0xbc to 0xbf already removed */
    snprintf( buffer, buflen, "%s", opcode_101xxxxx[ b & 0x1f ] ); *length = 1;
  }
}

/* Disassemble an instruction after DD/FD CB prefixes */
static void
disassemble_ddfd_cb( libspectrum_word address, char offset,
		     enum hl_type use_hl, char *buffer, size_t buflen,
		     size_t *length )
{
  libspectrum_byte b = readbyte_internal( address );
  char buffer2[40], buffer3[40];

  if( b < 0x40 ) {
    if( ( b & 0x07 ) == 0x06 ) {
      ix_iy_offset( buffer2, 40, use_hl, offset );
      snprintf( buffer, buflen, "%s %s", rotate_op( b ), buffer2 );
      *length = 1;
    } else {
      source_reg( address, USE_HL, buffer2, 40 );
      ix_iy_offset( buffer3, 40, use_hl, offset );
      snprintf( buffer, buflen, "LD %s,%s %s", buffer2,
		rotate_op( b ), buffer3 );
      *length = 1;
    }
  } else if( b < 0x80 ) {
    ix_iy_offset( buffer2, 40, use_hl, offset );
    snprintf( buffer, buflen, "BIT %d,%s", ( b >> 3 ) & 0x07, buffer2 );
    *length = 1;
  } else {
    if( ( b & 0x07 ) == 0x06 ) {
      ix_iy_offset( buffer2, 40, use_hl, offset );
      snprintf( buffer, buflen, "%s %d,%s", bit_op( b ), bit_op_bit( b ),
		buffer2 );
      *length = 1;
    } else {
      source_reg( address, USE_HL, buffer2, 40 );
      ix_iy_offset( buffer3, 40, use_hl, offset );
      snprintf( buffer, buflen, "LD %s,%s %s", buffer2, bit_op( b ), buffer3 );
      *length = 1;
    }
  }
}

/* Get a text representation of a one-byte number */
static void
get_byte( char *buffer, size_t buflen, libspectrum_byte b )
{
  snprintf( buffer, buflen, debugger_output_base == 10 ? "%d" : "%02X", b );
}

/* Get a text representation of an (LSB) two-byte number */
static void
get_word( char *buffer, size_t buflen, libspectrum_word address )
{
  libspectrum_word w;

  w  = readbyte_internal( address + 1 ); w <<= 8;
  w += readbyte_internal( address     );

  snprintf( buffer, buflen, debugger_output_base == 10 ? "%d" : "%04X", w );
}

/* Get a text representation of ( 'address' + 'offset' ) */
static void
get_offset( char *buffer, size_t buflen, libspectrum_word address,
	    libspectrum_byte offset )
{
  address += ( offset >= 0x80 ? offset-0x100 : offset );
  snprintf( buffer, buflen, debugger_output_base == 10 ? "%d" : "%04X",
	    address );
}

/* Select the appropriate register pair from BC, DE, HL (or IX, IY) or
   SP, depending on bits 4 and 5 of the opcode */
static const char *
reg_pair( libspectrum_byte b, enum hl_type use_hl )
{
  switch( ( b >> 4 ) & 0x03 ) {
  case 0: return "BC";
  case 1: return "DE";
  case 2: return hl_ix_iy( use_hl );
  case 3: return "SP";
  }
  return "* INTERNAL ERROR *";	/* Should never happen */
}

/* Get whichever of HL, IX or IY is in use here */
static const char *
hl_ix_iy( enum hl_type use_hl )
{
  switch( use_hl ) {
  case USE_HL: return "HL";
  case USE_IX: return "IX";
  case USE_IY: return "IY";
  }
  return "* INTERNAL ERROR *";	/* Should never happen */
}

/* Get a text representation of '(IX+03)' or similar things */
static void
ix_iy_offset( char *buffer, size_t buflen, enum hl_type use_hl,
	      libspectrum_byte offset )
{
  if( offset < 0x80 ) {
    snprintf( buffer, buflen,
	      debugger_output_base == 10 ? "(%s+%d)" : "(%s+%02X)",
	      hl_ix_iy( use_hl ), offset );
  } else {
    snprintf( buffer, buflen,
	      debugger_output_base == 10 ? "(%s-%d)" : "(%s-%02X)",
	      hl_ix_iy( use_hl ), 0x100 - offset );
  }
}

/* Get an 8-bit register, based on bits 0-2 of the opcode at 'address' */
static int
source_reg( libspectrum_word address, enum hl_type use_hl, char *buffer,
	    size_t buflen )
{
  return single_reg( readbyte_internal( address ) & 0x07, use_hl,
		     readbyte_internal( address + 1 ), buffer, buflen );
}

/* Get an 8-bit register, based on bits 3-5 of the opcode at 'address' */
static int
dest_reg( libspectrum_word address, enum hl_type use_hl, char *buffer,
	  size_t buflen )
{
  return single_reg( ( readbyte_internal( address ) >> 3 ) & 0x07, use_hl,
		     readbyte_internal( address + 1 ), buffer, buflen );
}

/* Get an 8-bit register name, including (HL). Also substitutes
   IXh, IXl and (IX+nn) and the IY versions if appropriate */
static int
single_reg( int i, enum hl_type use_hl, libspectrum_byte offset,
	    char *buffer, size_t buflen )
{
  char buffer2[40];

  if( i == 0x04 && use_hl != USE_HL ) {
    snprintf( buffer, buflen, "%sh", hl_ix_iy( use_hl ) );
    return 0;
  } else if( i == 0x05 && use_hl != USE_HL ) {
    snprintf( buffer, buflen, "%sl", hl_ix_iy( use_hl ) );
    return 0;
  } else if( i == 0x06 && use_hl != USE_HL ) {
    ix_iy_offset( buffer2, 40, use_hl, offset );
    snprintf( buffer, buflen, "%s", buffer2 );
    return 1;
  } else {
    const char *regs[] = { "B", "C", "D", "E", "H", "L", "(HL)", "A" };
    snprintf( buffer, buflen, "%s", regs[i] );
    return 0;
  }
}

/* Various lookup tables for opcodes */

/* Addition/subtraction opcodes:
   10xxxrrr: 'xxx' selects the opcode and 'rrr' the register to be added
   11xxx110: 'xxx' selects the opcode and add a constant
*/
static const char *
addition_op( libspectrum_byte b )
{
  const char *ops[] = { "ADD A,%s", "ADC A,%s", "SUB %s", "SBC A,%s",
			"AND %s",   "XOR %s",   "OR %s",  "CP %s"     };
  return ops[ ( b >> 3 ) & 0x07 ];
}

/* Conditions for jumps, etc:
   11xxx000: RET condition
   11xxx010: JP condition,nnnn
   11xxx100: CALL condition,nnnn
*/
static const char *
condition( libspectrum_byte b )
{
  const char *conds[] = { "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" };
  return conds[ ( b >> 3 ) & 0x07 ];
}

/* Shift/rotate operations:
   CB 00xxxrrr: 'xxx' selects the opcode and 'rrr' the register
   DD/FD CB <offset> 00xxxrrr: the documented rotate/shifts on (IX+nn) etc
                               and the undocumented rotate-and-store opcodes
*/
static const char *
rotate_op( libspectrum_byte b )
{
  const char *ops[] = { "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL" };
  return ops[ b >> 3 ];
}

/* Bit operations:
   CB oobbbrrr: 'oo' (not 00) selects operation
                'bbb' selects bit
                'rrr' selects register
   DD/FD CB <offset> oobbbrrr: the documented bit ops on (IX+nn) etc and the
                               undocumented bit-op-and store
*/
static const char *
bit_op( libspectrum_byte b )
{
  const char *ops[] = { "BIT", "RES", "SET" };
  return ops[ ( b >> 6 ) - 1 ];
}

/* Which bit is used by a BIT, RES or SET with this opcode (bits 3-5) */
static int
bit_op_bit( libspectrum_byte b )
{
  return ( b >> 3 ) & 0x07;
}

/* Get an instruction relative to a specific address */
libspectrum_word
debugger_search_instruction( libspectrum_word address, int delta )
{
  size_t j, length, longest;
  int i;

  if( !delta ) return address;

  if( delta > 0 ) {

    for( i = 0; i < delta; i++ ) {
      debugger_disassemble( NULL, 0, &length, address );
      address += length;
    }

  } else {

    for( i = 0; i > delta; i-- ) {
      /* Look for _longest_ opcode which produces the current top in second
         place */
      for( longest = 1, j = 1; j <= 8; j++ ) {
        debugger_disassemble( NULL, 0, &length, address - j );
        if( length == j ) longest = j;
      }
      address -= longest;
    }

  }

  return address;
}

/* Unit tests */

/* Disassembly test data */
libspectrum_byte test1_data[] = { 0x00 };

libspectrum_byte test2_data[] = { 0xdd, 0x00 };
libspectrum_byte test3_data[] = { 0xdd, 0x09 };
libspectrum_byte test4_data[] = { 0xdd, 0xdd, 0x00 };
libspectrum_byte test5_data[] = { 0xdd, 0xcb, 0x55, 0x06 };

libspectrum_byte test6_data[] = { 0xfd, 0x00 };
libspectrum_byte test7_data[] = { 0xfd, 0x09 };
libspectrum_byte test8_data[] = { 0xfd, 0xfd, 0x00 };
libspectrum_byte test9_data[] = { 0xfd, 0xcb, 0x55, 0x06 };

libspectrum_byte test10_data[] = { 0xdd, 0xfd, 0x09 };
libspectrum_byte test11_data[] = { 0xfd, 0xdd, 0x09 };

libspectrum_byte test12_data[] = { 0xdd, 0xfd, 0xdd, 0xfd, 0xdd, 0xfd, 0xdd,
                                   0xfd, 0xdd, 0xfd, 0xdd, 0xfd, 0x09 };
libspectrum_byte test13_data[] = { 0xfd, 0xdd, 0xfd, 0xdd, 0xfd, 0xdd, 0xfd,
                                   0xdd, 0xfd, 0xdd, 0xfd, 0xdd, 0x09 };

libspectrum_byte test14_data[] = { 0x7e };
libspectrum_byte test15_data[] = { 0xdd, 0x7e, 0x55 };

static int
run_test( libspectrum_byte *data, size_t data_length, const char *expected )
{
  char disassembly[16];
  size_t length;

  memcpy( memory_map_read[8].page, data, data_length );
  
  debugger_disassemble( disassembly, sizeof( disassembly ), &length, 0x4000 );

  if( strcmp( disassembly, expected ) ) return 1;
  if( length != data_length ) return 1;

  return 0;
}

int
debugger_disassemble_unittest( void )
{
  int r = 0;

  r += run_test( test1_data, sizeof( test1_data ), "NOP" );

  r += run_test( test2_data, sizeof( test2_data ), "NOP" );
  r += run_test( test3_data, sizeof( test3_data ), "ADD IX,BC" );
  r += run_test( test4_data, sizeof( test4_data ), "NOP" );
  r += run_test( test5_data, sizeof( test5_data ), "RLC (IX+55)" );

  r += run_test( test6_data, sizeof( test6_data ), "NOP" );
  r += run_test( test7_data, sizeof( test7_data ), "ADD IY,BC" );
  r += run_test( test8_data, sizeof( test8_data ), "NOP" );
  r += run_test( test9_data, sizeof( test9_data ), "RLC (IY+55)" );

  r += run_test( test10_data, sizeof( test10_data ), "ADD IY,BC" );
  r += run_test( test11_data, sizeof( test11_data ), "ADD IX,BC" );

  r += run_test( test12_data, sizeof( test12_data ), "ADD IY,BC" );
  r += run_test( test13_data, sizeof( test13_data ), "ADD IX,BC" );

  r += run_test( test14_data, sizeof( test14_data ), "LD A,(HL)" );
  r += run_test( test15_data, sizeof( test15_data ), "LD A,(IX+55)" );

  return r;
}
