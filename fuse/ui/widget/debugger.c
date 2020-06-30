/* debugger.c: The debugger widget
   Copyright (c) 2002-2014 Philip Kendall, Darren Salt
   Copyright (c) 2015 Stuart Brady
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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <libspectrum.h>
  
#include "debugger/debugger.h"
#include "display.h"
#include "keyboard.h"
#include "machine.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/scld.h"
#include "peripherals/ula.h"
#include "ui/uidisplay.h"
#include "widget.h"
#include "widget_internals.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

static enum {
  DB_REGISTERS, DB_BYTES, DB_TEXT, DB_DISASM, DB_BREAKPT
} display = DB_REGISTERS;

struct {
  libspectrum_word value;
  size_t page;
  char x, width, column;
} db_editv = {
  0, 0, 255, 0
};

static libspectrum_word debugger_memaddr;
static int breakpt_no = 0, breakpt_show = 0;

/* Various data displays */
static void display_registers( void );
static void display_bytes( void );
static void display_text( void );
static void display_disasm( void );
static void display_breakpts( void );

/* Scrolling for the data displays */
static void scroll( int step );

#define LC(X) ( (X)*8 - DISPLAY_BORDER_ASPECT_WIDTH )
#define LR(Y) ( (Y)*8 - DISPLAY_BORDER_HEIGHT )

static inline const char *format_8_bit( void )
{
  return debugger_output_base == 10 ? "%-3d" : "%02X";
}

static inline const char *format_16_bit( void )
{
  return debugger_output_base == 10 ? "%-5d" : "%04X";
}

int ui_debugger_activate( void )
{
  return widget_do_debugger();
}

int ui_debugger_deactivate( int interruptible GCC_UNUSED )
{
  /* Refresh the Spectrum's display, including the border */
  display_refresh_all();
  return widget_end_all( WIDGET_FINISHED_OK );
}

int ui_debugger_update( void )
{
  return widget_debugger_draw( NULL );
}

int ui_debugger_disassemble( libspectrum_word addr )
{
  debugger_memaddr = addr;
  return 0;
}

/* Debugger update function. The dialog box is created every time it is
   displayed, so no need to do anything here */
void ui_breakpoints_updated( void )
{
}

int widget_debugger_draw( void *data )
{
  static const char state[][8] = {
    "Running", "Halted", "Stepped", "Breakpt"
  };
  int x;
  char pbuf[8];

  widget_rectangle( LC(0), LR(0), 40 * 8, 17 * 8 + 4, 1 );
  widget_rectangle( LC(0), LR(17) + 2, 320, 1, 7 );

  switch ( display ) {
  case DB_REGISTERS: display_registers(); break;
  case DB_BYTES:     display_bytes();	  break;
  case DB_TEXT:      display_text();	  break;
  case DB_DISASM:    display_disasm();	  break;
  case DB_BREAKPT:   display_breakpts();  break;
  }

  widget_printstring( LC(0), LR(15) - 4, 6, state[debugger_mode] );
  widget_printstring( LC(10), LR(15) - 4, 6,
		      "\022S\021ingle step  \022C\021ontinue  Co\022m\021mand" );

  x = LC(-1);
  if( display != DB_REGISTERS )
    x = widget_printstring( x + 8, LR(16), 7, "\022R\021egs" );
  if( display != DB_BYTES )
    x = widget_printstring( x + 8, LR(16), 7, "\022B\021ytes" );
  if( display != DB_TEXT )
    x = widget_printstring( x + 8, LR(16), 7, "\022T\021ext" );
  if( display != DB_DISASM )
    x = widget_printstring( x + 8, LR(16), 7, "\022D\021isasm" );
  if( display != DB_BREAKPT )
    x = widget_printstring( x + 8, LR(16), 7, "Brea\022k\021pts" );

  widget_printstring_right( LC(25) + 4, LR(16), 5, "PC" );
  sprintf( pbuf, "%04X", PC );
  widget_printstring_fixed( LC(26) / 8, LR(16) / 8, 7, pbuf );

  widget_printstring_right( LR(35) + 4, LR(16), 5, "Bas\022e\021" );
  sprintf( pbuf, "%d", debugger_output_base );
  widget_printstring( LR(36), LR(16), 7, pbuf );

  widget_display_lines( LR(0) / 8, 18 );

  return 0;
}


void widget_debugger_keyhandler( input_key key )
{
  /* Display mode */
  switch ( key ) {
  case INPUT_KEY_Escape:	/* Close widget */
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    debugger_run();
    break;

  case INPUT_KEY_c:
  case INPUT_KEY_Return:	/* Close widget */
  case INPUT_KEY_KP_Enter:
    widget_end_all( WIDGET_FINISHED_OK );
    debugger_run();
    break;

  case INPUT_KEY_s:		/* Single step & reopen widget */
    debugger_mode = DEBUGGER_MODE_HALTED;
    widget_end_all( WIDGET_FINISHED_OK );
    break;

  case INPUT_KEY_r:		/* Display the registers */
    display = DB_REGISTERS;
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_b:		/* Display a memory dump (bytes) */
    display = DB_BYTES;
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_t:		/* Display a memory dump (text) */
    display = DB_TEXT;
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_d:		/* Display a disassembly */
    display = DB_DISASM;
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_k:		/* Display the breakpoints */
    display = DB_BREAKPT;
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_e:		/* Switch base */
    debugger_output_base = 26 - debugger_output_base;	/* 10 or 16 */
    widget_debugger_draw( NULL );
    break;

  case INPUT_KEY_m:		/* Enter a command */
    {
      widget_text_t text_data;

      text_data.title = "Debugger command";
      text_data.allow = WIDGET_INPUT_ASCII;
      text_data.max_length = 63;
      text_data.text[0] = 0;
      if( !widget_do_text( &text_data ) )
	debugger_command_evaluate( widget_text_text );
    }
    break;

  case INPUT_KEY_Up:		/* Back one line */
    scroll( -1 );
    break;

  case INPUT_KEY_Down:		/* Back one instruction or four lines */
    scroll( 1 );
    break;

  case INPUT_KEY_Page_Up:	/* Back eight lines */
    scroll( -8 );
    break;

  case INPUT_KEY_Page_Down:	/* Forward eight lines */
    scroll( 8 );
    break;

  case INPUT_KEY_Home:		/* To start of memory */
    debugger_memaddr = 0;
    scroll( 0 );
    break;

  case INPUT_KEY_End:		/* To end of RAM */
    debugger_memaddr = 0;
    scroll( -8 );
    break;

  default:;
  }
}

static void show_register0( int x, int y, const char *label, int value )
{
  char pbuf[8];

  sprintf( pbuf, "%d", value );
  widget_printstring_right( x - 4, y, 5, label );
  widget_printstring_fixed( x / 8, y / 8, 7, pbuf );
}

static void show_register1( int x, int y, const char *label, int value )
{
  char pbuf[8];

  sprintf( pbuf, format_8_bit(), value );
  widget_printstring_right( x - 4, y, 5, label );
  widget_printstring_fixed( x / 8, y / 8, 7, pbuf );
}

static void show_register2( int x, int y, const char *label, int value )
{
  char pbuf[8];

  sprintf( pbuf, format_16_bit(), value );
  widget_printstring_right( x - 4, y, 5, label );
  widget_printstring_fixed( x / 8, y / 8, 7, pbuf );
}

static void display_registers( void )
{
  int source, page_num, writable, contended;
  libspectrum_word offset;
  size_t block;
  char pbuf[16];
  int i, capabilities;

  show_register2( LC(3),  LR(0), "AF",   AF );
  show_register2( LC(12), LR(0), "AF'",  AF_ );
  show_register2( LC(20), LR(0), "SP",   SP );
  show_register2( LC(29), LR(0), "PC",   PC );
  show_register1( LC(36), LR(0), "R",    ( R & 0x7F ) | ( R7 & 0x80 ) );

  show_register2( LC(3),  LR(1), "BC",   BC );
  show_register2( LC(12), LR(1), "BC'",  BC_ );
  show_register2( LC(20), LR(1), "IX",   IX );
  show_register2( LC(29), LR(1), "IY",   IY );
  show_register1( LC(36), LR(1), "I",    I );

  show_register2( LC(3),  LR(2), "DE",   DE );
  show_register2( LC(12), LR(2), "DE'",  DE_ );
  show_register0( LC(20), LR(2), "IM",   IM );
  show_register0( LC(29), LR(2), "IFF1", IFF1 );
  show_register0( LC(36), LR(2), "IFF2", IFF2 );

  show_register0( LC(36), LR(2), "IFF2", IFF2 );
  show_register2( LC(3),  LR(3), "HL",   HL );
  show_register2( LC(12), LR(3), "HL'",  HL_ );
  widget_printstring_fixed( LC(20) / 8, LR(3) / 8, 5, "SZ5H3PNC" );
  show_register0( LC(36), LR(3), "HALTED", z80.halted );
  show_register1( LC(36), LR(4), "ULA",  ula_last_byte() );

  sprintf( pbuf, "%d", tstates );
  widget_printstring_right( LC(12) - 4, LR(4), 5, "Tstates" );
  widget_printstring_fixed( LC(12) / 8, LR(4) / 8, 7, pbuf );
  for( i = 0; i < 8; ++i )
    pbuf[i] = ( F & ( 0x80 >> i ) ) ? '1' : '0';
  pbuf[8] = 0;
  widget_printstring_fixed( LC(20) / 8, LR(4) / 8, 7, pbuf );

  capabilities = libspectrum_machine_capabilities( machine_current->machine );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY )
    show_register1( LC(37), LR(4), "AY",
		    machine_current->ay.current_register );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
    show_register1( LC(6), LR(5), "128Mem",
		    machine_current->ram.last_byte );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
    show_register1( LC(15), LR(5), "+3Mem",
		    machine_current->ram.last_byte2 );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY )
    show_register1( LC(24), LR(5), "TmxDec", scld_last_dec.byte );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY )
    show_register1( LC(33), LR(5), "TmxHSR", scld_last_hsr );

  if( settings_current.zxcf_active )
    show_register1( LC(6), LR(5), "ZXCF", zxcf_last_memctl() );

  source = page_num = writable = contended = -1;
  offset = 0;
  i = 0;

  for( block = 0; block < MEMORY_PAGES_IN_64K; block++ ) {
    memory_page *page = &memory_map_read[block];

    if( page->source != source ||
      page->page_num != page_num ||
      page->offset != offset ||
      page->writable != writable ||
      page->contended != contended ) {

      int x = LC(5 + 20 * ( i & 1 ) ), y = LR(6 + ( i / 2 ) );

      sprintf( pbuf, format_16_bit(), (unsigned)block * MEMORY_PAGE_SIZE );
      widget_printstring_right( x, y, 5, pbuf );
      snprintf( pbuf, sizeof( pbuf ), "%s %d",
                memory_source_description( memory_map_read[block].source ),
                memory_map_read[block].page_num );
      x = widget_printstring( x + 4, y, 7, pbuf ) + 4;
      if( memory_map_read[block].writable )
        x = widget_printstring( x, y, 4, "w" );
      if( memory_map_read[block].contended )
        x = widget_printstring( x, y, 4, "c" );

      i++;

      source = page->source;
      page_num = page->page_num;
      writable = page->writable;
      contended = page->contended;
      offset = page->offset;
    }

    /* We expect the next page to have an increased offset */
    offset += MEMORY_PAGE_SIZE;
  }
}


static void display_bytes( void )
{
  int x, y;
  char pbuf[36];

  for( y = 0; y < 8; ++y ) {
    libspectrum_word addr = debugger_memaddr + y * 8;

    sprintf( pbuf, format_16_bit(), addr );
    widget_printstring_fixed( LC(1) / 8, LR(y) / 8, 7, pbuf );
    widget_printstring( LC(6), LR(y), 5, ":" );

    for( x = 0; x < 8; ++x ) {
      sprintf( pbuf + x * 4, format_8_bit(),
	       readbyte_internal( addr + x ) );
      if( x < 7 )
	strcat( pbuf, "  " );
    }
    widget_printstring_fixed( LC(7) / 8, LR(y) / 8, 7, pbuf );
  }
}


static void display_text( void )
{
  int x, y;
  char pbuf[8];

  for( y = 0; y < 8; ++y ) {
    libspectrum_word addr = debugger_memaddr + y * 32;

    sprintf( pbuf, format_16_bit(), addr );
    widget_printstring_fixed( LC(1) / 8, LR(y) / 8, 7, pbuf );
    widget_printstring( LC(6), LR(y), 5, ":" );

    for( x = 0; x < 32; ++x )
      widget_printchar_fixed( LC(x + 8) / 8, LR(y) / 8, 7,
			      readbyte_internal( addr + x ) );
  }
}


static void display_disasm( void )
{
  int y;
  char pbuf[40];
  libspectrum_word addr = debugger_memaddr;

  for( y = 0; y < 8; ++y ) {
    size_t length;
    char *spc;

    sprintf( pbuf, format_16_bit(), addr );
    widget_printstring_fixed( LC(1) / 8, LR(y) / 8, 7, pbuf );
    widget_printstring( LC(6), LR(y), 5, ":" );

    debugger_disassemble( pbuf, sizeof( pbuf ), &length, addr );
    addr += length;
    spc = strchr( pbuf, ' ' );
    if( spc )
      *spc = 0;
    widget_printstring( LC(8), LR(y), 7, pbuf );
    if( spc ) {
      spc += 1 + strspn( spc + 1, " " );
      widget_printstring( LC(12) + 4, LR(y), 7, spc );
    }
  }
}


static void display_breakpts( void )
{
  GSList *ptr;
  int i = -breakpt_show;
  char pbuf[80], fmt[20];

  if( i )
    widget_up_arrow( LC(0), LR(0), 7 );

  for( ptr = debugger_breakpoints; i < 8 && ptr; ptr = ptr->next, ++i ) {
    const debugger_breakpoint *bp = ptr->data;

    if( i < 0 )
      continue;

    sprintf( pbuf, "%lu", ( unsigned long )bp->id );
    widget_printstring( LC(1), LR(i), 5, pbuf );
    widget_printstring( LC(6), LR(i), 7,
			debugger_breakpoint_type_abbr[bp->type] );

    switch ( bp->type ) {
    case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
    case DEBUGGER_BREAKPOINT_TYPE_READ:
    case DEBUGGER_BREAKPOINT_TYPE_WRITE:
      if( bp->value.address.source == memory_source_any )
	sprintf( pbuf, format_16_bit(), bp->value.address.offset );
      else {
        snprintf( fmt, sizeof( fmt ), "%%s:%s:%s", format_16_bit(),
                  format_16_bit() );
        snprintf( pbuf, sizeof( pbuf ), fmt,
                  memory_source_description( bp->value.address.source ),
                  bp->value.address.page, bp->value.address.offset );
      }
      break;

    case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
    case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
      sprintf( fmt, "%s:%s", format_16_bit(), format_16_bit() );
      sprintf( pbuf, fmt, bp->value.port.mask, bp->value.port.port );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_TIME:
      sprintf( pbuf, "%5d", bp->value.time.tstates );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_EVENT:
      sprintf( pbuf, "%s:%s", bp->value.event.type, bp->value.event.detail );
      break;
    }
    widget_printstring( LC(10), LR(i), 6, pbuf );

    sprintf( pbuf, "%lu", ( unsigned long )bp->ignore );
    widget_printstring( LC(18) + 4, LR(i), 7, pbuf );

    sprintf( pbuf, "%s", debugger_breakpoint_life_abbr[bp->life] );
    widget_printstring( LC(23), LR(i), 7, pbuf );

    if( bp->condition ) {
      debugger_expression_deparse( pbuf, sizeof( pbuf ), bp->condition );
      widget_printstring( LC(28) + 4, LR(i), 6, pbuf );
    }
  }

  if( !i )
    widget_printstring( LC(1), LR(0), 5, "(No breakpoints)" );
  else if( ptr )
    widget_down_arrow( LC(0), LC(7), 7 );
}


static void scroll( int step )
{
  switch ( display ) {
  case DB_BYTES:
    debugger_memaddr += 8 * step;
    break;

  case DB_TEXT:
    debugger_memaddr += 32 * step;
    break;

  case DB_DISASM:
    if( step > 0 )
      for( ; step; --step ) {
	size_t length;

	debugger_disassemble( NULL, 0, &length, debugger_memaddr );
	debugger_memaddr += length;
    } else
      for( ; step; ++step ) {
	/* For details, see ui/gtk/debugger.c:move_disassembly() */
	size_t i, longest = 1;

	for( i = 1; i <= 8; ++i ) {
	  size_t length;

	  debugger_disassemble( NULL, 0, &length, debugger_memaddr );
	  if( length == i )
	    longest = i;
	}
	debugger_memaddr -= longest;
      }
    break;

  case DB_BREAKPT:
    {
      int length = g_slist_length( debugger_breakpoints );

      breakpt_no += step;
      if( breakpt_no >= length )
	breakpt_no = length - 1;
      if( breakpt_no < 0 )
	breakpt_no = 0;
      if( breakpt_no < breakpt_show )
	breakpt_show = breakpt_no;
      else if( breakpt_no > breakpt_show + 7 )
	breakpt_show = breakpt_no - 7;
    }
    break;

  default:
    return;
  }

  widget_debugger_draw( NULL );
}
