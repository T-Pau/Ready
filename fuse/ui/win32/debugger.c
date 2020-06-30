/* debugger.c: the Win32 debugger
   Copyright (c) 2004-2012 Philip Kendall, Marek Januszewski
   Copyright (c) 2013-2015 Sergio Baldoví
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

/* FIXME: is that needed?
#include <stdio.h>
#include <string.h>
*/

#include <libspectrum.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
 
#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "settings.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/scld.h"
#include "peripherals/ula.h"
#include "ui/ui.h"
#include "win32internals.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#include "debugger.h"

/* FIXME: delete whatever is not needed

#include "machine.h"
#include "memory_pages.h"

 */

/* The various debugger panes */
typedef enum debugger_pane {

  DEBUGGER_PANE_BEGIN = 1,	/* Start marker */

  DEBUGGER_PANE_REGISTERS = DEBUGGER_PANE_BEGIN,
  DEBUGGER_PANE_MEMORYMAP,
  DEBUGGER_PANE_BREAKPOINTS,
  DEBUGGER_PANE_DISASSEMBLY,
  DEBUGGER_PANE_STACK,
  DEBUGGER_PANE_EVENTS,

  DEBUGGER_PANE_END		/* End marker */
} debugger_pane;

static int create_dialog( void );
static int hide_hidden_panes( void );
static UINT get_pane_menu_item( debugger_pane pane );
static BOOL show_hide_pane( debugger_pane pane, int show );
/* static int create_menu_bar( void ); this function is handled by rc */
static void toggle_display( debugger_pane pane, UINT menu_item_id );
static int create_register_display( HFONT font );
/* int create_memory_map( void ); this function is handled by rc */
static int create_breakpoints( void );
static int create_disassembly( HFONT font );
static int create_stack_display( HFONT font );
static void stack_click( LPNMITEMACTIVATE lpnmitem );
static int create_events( void );
static void events_click( LPNMITEMACTIVATE lpnmitem );
/* int create_command_entry( void ); this function is handled by rc */
/* int create_buttons( void ); this function is handled by rc */

static int activate_debugger( void );
static void update_memory_map( void );
static void update_breakpoints( void );
static void update_disassembly( void );
static void update_events( void );
static void add_event( gpointer data, gpointer user_data GCC_UNUSED );
static int deactivate_debugger( void );

static int move_disassembly( WPARAM scroll_command );

static void evaluate_command( void );
static void win32ui_debugger_done_step( void );
static void win32ui_debugger_done_continue( void );
static void win32ui_debugger_break( void );
static void delete_dialog( void );
static void win32ui_debugger_done_close( void );
static INT_PTR CALLBACK win32ui_debugger_proc( HWND hWnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam );

static LRESULT CALLBACK
disassembly_listview_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

/* The top line of the current disassembly */
static libspectrum_word disassembly_top;

/* The next line below the current disassembly */
static libspectrum_word disassembly_bottom;

/* helper constants for disassembly listview's scrollbar */
static const int disassembly_min = 0x0000;
static const int disassembly_max = 0xffff;
/* Visual styles could change visible rows */
static unsigned int disassembly_page = 20;

/* Have we created the above yet? */
static int dialog_created = 0;

/* Is the debugger window active (as opposed to the debugger itself)? */
static int debugger_active;

#define STUB do { printf("STUB: %s()\n", __func__); fflush(stdout); } while(0)

static const TCHAR*
format_8_bit( void )
{
  return debugger_output_base == 10 ? TEXT( "%3d" ) : TEXT( "0x%02X" );
}

static const TCHAR*
format_16_bit( void )
{
  return debugger_output_base == 10 ? TEXT( "%5d" ) : TEXT( "0x%04X" );
}

int
ui_debugger_activate( void )
{
  int error;

  fuse_emulation_pause();

  /* create_dialog will create the dialog or activate if it exists */
  if( !dialog_created ) if( create_dialog() ) return 1;
        
  ShowWindow( fuse_hDBGWnd, SW_SHOW );
  error = hide_hidden_panes(); if( error ) return error;
  
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ), TRUE);
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), FALSE );
  if( !debugger_active ) activate_debugger();

  return 0;
}

void
ui_breakpoints_updated( void )
{
  /* TODO: Refresh debugger list here */
}

static int
hide_hidden_panes( void )
{
  debugger_pane i;
  UINT checkitem;
  MENUITEMINFO mii;

  for( i = DEBUGGER_PANE_BEGIN; i < DEBUGGER_PANE_END; i++ ) {

    checkitem = get_pane_menu_item( i ); if( !checkitem ) return 1;

    mii.fMask = MIIM_STATE;
    mii.cbSize = sizeof( MENUITEMINFO );
    if( ! GetMenuItemInfo( GetMenu( fuse_hDBGWnd ), checkitem, FALSE, &mii ) )
      return 1;
    
    if( mii.fState && MFS_CHECKED ) continue;

    if( ! show_hide_pane( i, SW_HIDE ) ) return 1;
  }

  return 0;
}

static UINT
get_pane_menu_item( debugger_pane pane )
{
  UINT menu_item_id;

  menu_item_id = 0;

  switch( pane ) {
  case DEBUGGER_PANE_REGISTERS: menu_item_id = IDM_DBG_REG; break;
  case DEBUGGER_PANE_MEMORYMAP: menu_item_id = IDM_DBG_MEMMAP; break;
  case DEBUGGER_PANE_BREAKPOINTS: menu_item_id = IDM_DBG_BPS; break;
  case DEBUGGER_PANE_DISASSEMBLY: menu_item_id = IDM_DBG_DIS; break;
  case DEBUGGER_PANE_STACK: menu_item_id = IDM_DBG_STACK; break;
  case DEBUGGER_PANE_EVENTS: menu_item_id = IDM_DBG_EVENTS; break;

  case DEBUGGER_PANE_END: break;
  }

  if( !menu_item_id ) {
    ui_error( UI_ERROR_ERROR, "unknown debugger pane %u", pane );
    return 0;
  }

  return menu_item_id;
}

static BOOL
show_hide_pane( debugger_pane pane, int show )
{
/* Instead of get_pane() and then hiding or showing the widget as it is done
   in gtk ui, we need to show/hide multiple widgets, hence this combined
   function.

   show parameter needs to be SW_SHOW to reveal the pane,
   or SW_HIDE to hide it.
*/
  /* FIXME: window needs to resize/collapse as panel are being hidden */
  int i;
        
  switch( pane ) {
    case DEBUGGER_PANE_REGISTERS:
      for( i = IDC_DBG_REG_PC; i <= IDC_DBG_REG_IM; i++ ) {
        ShowWindow( GetDlgItem( fuse_hDBGWnd, i ), show );
      }
      return TRUE;
  
    case DEBUGGER_PANE_MEMORYMAP:
      for( i = IDC_DBG_MAP11; i <= IDC_DBG_TEXT_CONTENDED; i++ ) {
        ShowWindow( GetDlgItem( fuse_hDBGWnd, i ), show );
      }
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_GRP_MEMMAP ), show );
      return TRUE;
  
    case DEBUGGER_PANE_BREAKPOINTS:
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_BPS ), show );
      return TRUE;
  
    case DEBUGGER_PANE_DISASSEMBLY:
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_PC ), show );
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ), show );
      return TRUE;
  
    case DEBUGGER_PANE_STACK:
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_STACK ), show );
      return TRUE;
  
    case DEBUGGER_PANE_EVENTS:
      ShowWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_EVENTS ), show );
      return TRUE;
  
    case DEBUGGER_PANE_END: break;
  }

  ui_error( UI_ERROR_ERROR, "unknown debugger pane %u", pane );
  return FALSE;
}

int
ui_debugger_deactivate( int interruptable )
{
  if( debugger_active ) deactivate_debugger();

  if( dialog_created ) {
    EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ),
                  !interruptable ? TRUE : FALSE );
    EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), 
                  interruptable ? TRUE : FALSE );
  }

  return 0;
}

static int
create_dialog( void )
{
  int error;
  debugger_pane i;
  MENUITEMINFO mii;  

  HFONT font;

  error = win32ui_get_monospaced_font( &font ); if( error ) return error;

  fuse_hDBGWnd = CreateDialog( fuse_hInstance, MAKEINTRESOURCE( IDD_DBG ),
                               fuse_hWnd, win32ui_debugger_proc );

  /* The main display areas */
  error = create_register_display( font );
  if( error ) return error;

  error = create_breakpoints(); if( error ) return error;

  error = create_disassembly( font ); if( error ) return error;

  error = create_stack_display( font ); if( error ) return error;

  error = create_events(); if( error ) return error;

  /* Initially, have all the panes visible */
  for( i = DEBUGGER_PANE_BEGIN; i < DEBUGGER_PANE_END; i++ ) {
  
    UINT check_item;

    check_item = get_pane_menu_item( i ); if( !check_item ) break;

    mii.fMask = MIIM_STATE;
    mii.fState = MFS_CHECKED;
    mii.cbSize = sizeof( MENUITEMINFO );
    SetMenuItemInfo( GetMenu( fuse_hDBGWnd ), check_item, FALSE, &mii );
  }

  dialog_created = 1;

  return 0;
}

static void
toggle_display( debugger_pane pane, UINT menu_item_id )
{
  MENUITEMINFO mii;
        
  mii.fMask = MIIM_STATE;
  mii.cbSize = sizeof( MENUITEMINFO );
  GetMenuItemInfo( GetMenu( fuse_hDBGWnd ), menu_item_id, FALSE, &mii );
    
  /* Windows doesn't automatically checks/unchecks
     the menus when they're clicked */
  if( mii.fState && MFS_CHECKED ) {
    show_hide_pane( pane, SW_HIDE );
    mii.fState = MFS_UNCHECKED;
    SetMenuItemInfo( GetMenu( fuse_hDBGWnd ), menu_item_id, FALSE, &mii );
  } else {
    show_hide_pane( pane, SW_SHOW );
    mii.fState = MFS_CHECKED;
    SetMenuItemInfo( GetMenu( fuse_hDBGWnd ), menu_item_id, FALSE, &mii );
  }
}

static int
create_register_display( HFONT font )
{
  /* this display is created in rc, just set the monospaced font */
  size_t i;

  for( i = 0; i < NUM_DBG_REGS; i++ ) {
    win32ui_set_font( fuse_hDBGWnd, IDC_DBG_REG_PC + i, font );
  }

  return 0;
}

static int
create_breakpoints( void )
{
  size_t i;

  LPCTSTR breakpoint_titles[] = { _T( "ID" ), _T( "Type" ), _T( "Value" ),
                                  _T( "Ignore" ), _T( "Life" ), 
                                  _T( "Condition" ) };
  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 6; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = _tcslen( breakpoint_titles[i] ) * 8 + 10;
    lvc.pszText = (LPTSTR)breakpoint_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  return 0;
}

static int
create_disassembly( HFONT font )
{
  size_t i;

  LPCTSTR disassembly_titles[] = { TEXT( "Address" ), TEXT( "Instruction" ) };

  /* The disassembly listview itself */

  /* subclass listview to catch keydown and mousewheel messages */
  HWND hwnd_list = GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_PC );
  WNDPROC orig_proc = (WNDPROC) GetWindowLongPtr( hwnd_list, GWLP_WNDPROC );
  SetProp( hwnd_list, "original_proc", (HANDLE) orig_proc );
  SetWindowLongPtr( hwnd_list, GWLP_WNDPROC, 
                    (LONG_PTR) (WNDPROC) disassembly_listview_proc );

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  lv_ext_style |= LVS_EX_DOUBLEBUFFER;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  win32ui_set_font( fuse_hDBGWnd, IDC_DBG_LV_PC, font );

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_TEXT;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 ) lvc.mask |= LVCF_SUBITEM;
    lvc.pszText = (LPTSTR)disassembly_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }

  /* Set columns width */
  ListView_SetColumnWidth( hwnd_list, 0, LVSCW_AUTOSIZE_USEHEADER );
  ListView_SetColumnWidth( hwnd_list, 1, LVSCW_AUTOSIZE_USEHEADER );

  /* Recalculate visible rows, Visual Styles could change rows height */
  disassembly_page = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                                         LVM_GETCOUNTPERPAGE, 0, 0 );

  /* The disassembly scrollbar */
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE; 
  si.nPos = 0;
  si.nMin = disassembly_min;
  si.nMax = disassembly_max;
  si.nPage = disassembly_page;
  SetScrollInfo( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ),
                 SB_CTL, &si, TRUE );
  
  return 0;
}

static int
create_stack_display( HFONT font )
{
  size_t i;

  LPCTSTR stack_titles[] = { _T( "Address" ), _T( "Value" ) };

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  win32ui_set_font( fuse_hDBGWnd, IDC_DBG_LV_STACK, font );

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_TEXT;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 ) lvc.mask |= LVCF_SUBITEM;
    lvc.pszText = (LPTSTR)stack_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }

  /* Set columns width */
  HWND hwnd_list = GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_STACK );
  ListView_SetColumnWidth( hwnd_list, 0, LVSCW_AUTOSIZE_USEHEADER );
  ListView_SetColumnWidth( hwnd_list, 1, LVSCW_AUTOSIZE_USEHEADER );

  return 0;
}

static void
stack_click( LPNMITEMACTIVATE lpnmitem )
{
  libspectrum_word destination;
  int retval, error, row;
  char *stopstring;
  TCHAR buffer[255];
  LVITEM li;

  row = lpnmitem->iItem;
  if( row < 0 ) return;

  li.iSubItem = 1;
  li.pszText = buffer;
  li.cchTextMax = 255;
  retval = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                                 LVM_GETITEMTEXT, row, ( LPARAM ) &li );
  if( !retval ) {
    ui_error( UI_ERROR_ERROR, "couldn't get text for row %d", row );
    return;
  }

  destination = strtoul( buffer, &stopstring, 16 );

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_EXECUTE, memory_source_any, 0, destination, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

static int
create_events( void )
{
  size_t i;
  LPCTSTR titles[] = { _T( "Time" ), _T( "Type" ) };

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_TEXT;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 ) lvc.mask |= LVCF_SUBITEM;
    lvc.pszText = (LPTSTR)titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }

  /* Set columns width */
  HWND hwnd_list = GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_EVENTS );
  ListView_SetColumnWidth( hwnd_list, 0, LVSCW_AUTOSIZE_USEHEADER );
  ListView_SetColumnWidth( hwnd_list, 1, LVSCW_AUTOSIZE_USEHEADER );

  return 0;
}

static void
events_click( LPNMITEMACTIVATE lpnmitem )
{
  int got_text, error, row;
  TCHAR buffer[255];
  LVITEM li;
  libspectrum_dword tstates;

  row = lpnmitem->iItem;
  if( row < 0 ) return;
        
  li.iSubItem = 0;
  li.pszText = buffer;
  li.cchTextMax = 255;
  got_text = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                 LVM_GETITEMTEXT, row, ( LPARAM ) &li );
  if( !got_text ) {
    ui_error( UI_ERROR_ERROR, "couldn't get text for row %d", row );
    return;
  }

  tstates = _ttoi( buffer );
  error = debugger_breakpoint_add_time(
    DEBUGGER_BREAKPOINT_TYPE_TIME, tstates, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

static int
activate_debugger( void )
{
  debugger_active = 1;

  ui_debugger_disassemble( PC );
  ui_debugger_update();

  win32ui_process_messages( 0 );
  return 0;
}

/* Update the debugger's display */
int
ui_debugger_update( void )
{
  size_t i;
  TCHAR buffer[1024], format_string[1024];
  TCHAR *disassembly_text[2] = { &buffer[0], &buffer[40] };
  libspectrum_word address;
  int capabilities; size_t length;

  const char *register_name[] = { TEXT( "PC" ), TEXT( "SP" ),
				  TEXT( "AF" ), TEXT( "AF'" ),
				  TEXT( "BC" ), TEXT( "BC'" ),
				  TEXT( "DE" ), TEXT( "DE'" ),
				  TEXT( "HL" ), TEXT( "HL'" ),
				  TEXT( "IX" ), TEXT( "IY" ),
                                };

  libspectrum_word *value_ptr[] = { &PC, &SP,  &AF, &AF_,
				    &BC, &BC_, &DE, &DE_,
				    &HL, &HL_, &IX, &IY,
				  };

  if( !dialog_created ) return 0;

  /* FIXME: verify all functions below are unicode compliant */
  for( i = 0; i < 12; i++ ) {
    _sntprintf( buffer, 5, "%3s ", register_name[i] );
    _sntprintf( &buffer[4], 76, format_16_bit(), *value_ptr[i] );
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_PC + i, 
                        WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  }

  _tcscpy( buffer, TEXT( "  I   " ) );
  _sntprintf( &buffer[6], 76, format_8_bit(), I );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_I,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  _tcscpy( buffer, TEXT( "  R   " ) );
  _sntprintf( &buffer[6], 80, format_8_bit(), ( R & 0x7f ) | ( R7 & 0x80 ) );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_R,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  _sntprintf( buffer, 80, TEXT( "Halted %d" ), z80.halted );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_HALTED,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  _sntprintf( buffer, 80, TEXT( "T-states %5d" ), tstates );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_T_STATES,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  _sntprintf( buffer, 80, TEXT( "  IM %d\r\nIFF1 %d\r\nIFF2 %d" ),
              IM, IFF1, IFF2 );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_IM,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  _tcscpy( buffer, TEXT( "SZ5H3PNC\r\n" ) );
  for( i = 0; i < 8; i++ ) buffer[i+10] = ( F & ( 0x80 >> i ) ) ? '1' : '0';
  buffer[18] = '\0';
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_FLAGS,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  capabilities = libspectrum_machine_capabilities( machine_current->machine );

  _stprintf( format_string, "   ULA %s", format_8_bit() );
  _sntprintf( buffer, 1024, format_string, ula_last_byte() );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    _stprintf( format_string, "\r\n    AY %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ay.current_register );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    _stprintf( format_string, "\r\n128Mem %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ram.last_byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {
    _stprintf( format_string, "\r\n+3 Mem %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ram.last_byte2 );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO  ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    _stprintf( format_string, "\r\nTmxDec %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        scld_last_dec.byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    _stprintf( format_string, "\r\nTmxHsr %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string, scld_last_hsr );
  }

  if( settings_current.zxcf_active ) {
    _stprintf( format_string, "\r\n  ZXCF %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        zxcf_last_memctl() );
  }

  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_ULA,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  update_memory_map();
  update_breakpoints();
  update_disassembly();

  /* And the stack display */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                      LVM_DELETEALLITEMS, 0, 0 );
  
  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0, address = SP + 38; i < 20; i++, address -= 2 ) {
    
    libspectrum_word contents = readbyte_internal( address ) +
				0x100 * readbyte_internal( address + 1 );

    _sntprintf( disassembly_text[0], 40, format_16_bit(), address );
    _sntprintf( disassembly_text[1], 40, format_16_bit(), contents );

    /* add the item */
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.pszText = disassembly_text[0];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = disassembly_text[1];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }

  /* And the events display */
  update_events();

  return 0;
}

static void
update_memory_map( void )
{
  TCHAR buffer[ 40 ];
  int source, page_num, writable, contended;
  libspectrum_word offset;
  size_t i, j, block, row;

  source = page_num = writable = contended = -1;
  offset = 0;
  row = 0;

  for( block = 0; block < MEMORY_PAGES_IN_64K && row < 8; block++ ) {
    memory_page *page = &memory_map_read[block];

    if( page->source != source ||
      page->page_num != page_num ||
      page->offset != offset ||
      page->writable != writable ||
      page->contended != contended ) {

      _sntprintf( buffer, 40, format_16_bit(),
                  (unsigned)block * MEMORY_PAGE_SIZE );
      SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( row * 4 ), 
                          WM_SETTEXT, ( WPARAM ) 0, ( LPARAM ) buffer );

      /* FIXME: memory_source_description is not unicode */
      _snprintf( buffer, 40, TEXT( "%s %d" ),
                 memory_source_description( page->source ), page->page_num );

      SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( row * 4 ) + 1,
                          WM_SETTEXT, ( WPARAM ) 0, ( LPARAM ) buffer );

      SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( row * 4 ) + 2,
                          WM_SETTEXT, (WPARAM) 0,
                          ( LPARAM ) ( page->writable
                          ? TEXT( "Y" ) : TEXT( "N" ) ) );

      SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( row * 4 ) + 3,
                          WM_SETTEXT, (WPARAM) 0,
                          ( LPARAM ) ( page->contended
                          ? TEXT( "Y" ) : TEXT( "N" ) ) );
      row++;

      source = page->source;
      page_num = page->page_num;
      writable = page->writable;
      contended = page->contended;
      offset = page->offset;
    }

    /* We expect the next page to have an increased offset */
    offset += MEMORY_PAGE_SIZE;
  }

  /* Hide unused rows */
  for( i = row; i < 8; i++ ) {
    for( j = 0; j < 4; j++ ) {
      SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( i * 4 ) + j,
                          WM_SETTEXT, (WPARAM) 0, (LPARAM) NULL );
    }
  }

}

static void
update_breakpoints( void )
{
  /* FIXME: review this function for unicode compatibility */
  TCHAR buffer[ 1024 ],
    *breakpoint_text[6] = { &buffer[  0], &buffer[ 40], &buffer[80],
			    &buffer[120], &buffer[160], &buffer[200] };
  GSList *ptr;
  TCHAR format_string[ 1024 ];

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;
  int i;

  /* Create the breakpoint list */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                      LVM_DELETEALLITEMS, 0, 0 );

  for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {

    debugger_breakpoint *bp = ptr->data;

    _sntprintf( breakpoint_text[0], 40, "%lu", (unsigned long)bp->id );
    _sntprintf( breakpoint_text[1], 40, "%s",
	       debugger_breakpoint_type_text[ bp->type ] );

    switch( bp->type ) {

    case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
    case DEBUGGER_BREAKPOINT_TYPE_READ:
    case DEBUGGER_BREAKPOINT_TYPE_WRITE:
      if( bp->value.address.source == memory_source_any ) {
        _sntprintf( breakpoint_text[2], 40, format_16_bit(),
        bp->value.address.offset );
      } else {
        snprintf( format_string, 1024, "%%s:%s:%s",
                  format_16_bit(), format_16_bit() );
        snprintf( breakpoint_text[2], 40, format_string,
                  memory_source_description( bp->value.address.source ),
                  bp->value.address.page, bp->value.address.offset );
      }
      break;

    case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
    case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
      _stprintf( format_string, "%s:%s", format_16_bit(), format_16_bit() );
      _sntprintf( breakpoint_text[2], 40, format_string,
		  bp->value.port.mask, bp->value.port.port );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_TIME:
      _sntprintf( breakpoint_text[2], 40, "%5d", bp->value.time.tstates );
      break;
      
    case DEBUGGER_BREAKPOINT_TYPE_EVENT:
      _sntprintf( breakpoint_text[2], 40, "%s:%s", bp->value.event.type,
                  bp->value.event.detail );

    }

    _sntprintf( breakpoint_text[3], 40, "%lu", (unsigned long)bp->ignore );
    _sntprintf( breakpoint_text[4], 40, "%s",
	        debugger_breakpoint_life_text[ bp->life ] );
    if( bp->condition ) {
      debugger_expression_deparse( breakpoint_text[5], 80, bp->condition );
    } else {
      _tcscpy( breakpoint_text[5], "" );
    }

    /* get the count of items to insert as last element */
    lvi.iItem = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                                    LVM_GETITEMCOUNT, 0, 0 );

    /* append the breakpoint items */
    for( i = 0; i < 6; i++ ) {
      lvi.iSubItem = i;
      lvi.pszText = breakpoint_text[i];
      if( i == 0 )
        SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_INSERTITEM, 0,
                            ( LPARAM ) &lvi );
      else
        SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_SETITEM, 0,
                            ( LPARAM ) &lvi );
    }
  }
}

static void
update_disassembly( void )
{
  size_t i, length; libspectrum_word address;
  TCHAR buffer[80];
  TCHAR *disassembly_text[2] = { &buffer[0], &buffer[40] };

  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                      LVM_DELETEALLITEMS, 0, 0 );

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0, address = disassembly_top; i < disassembly_page; i++ ) {
    int l;
    _sntprintf( disassembly_text[0], 40, format_16_bit(), address );
    debugger_disassemble( disassembly_text[1], 40, &length, address );

    /* pad to 16 characters (long instruction) to avoid varying width */
    l = _tcslen( disassembly_text[1] );
    while( l < 16 ) disassembly_text[1][l++] = ' ';
    disassembly_text[1][l] = 0;

    address += length;

    /* append the item */
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.pszText = disassembly_text[0];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = disassembly_text[1];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }

  disassembly_bottom = address;
}

static void
update_events( void )
{
  /* clear the listview */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                      LVM_DELETEALLITEMS, 0, 0 );

  event_foreach( add_event, NULL );
}

static void
add_event( gpointer data, gpointer user_data GCC_UNUSED )
{
  event_t *ptr = data;

  TCHAR buffer[80];
  TCHAR *event_text[2] = { &buffer[0], &buffer[40] };

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  /* Skip events which have been removed */
  if( ptr->type == event_type_null ) return;

  _sntprintf( event_text[0], 40, "%d", ptr->tstates );
  /* FIXME: event_name() is not unicode compliant */
  _tcsncpy( event_text[1], event_name( ptr->type ), 40 );

  /* append the item */
  lvi.iItem = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                  LVM_GETITEMCOUNT, 0, 0 );
  lvi.iSubItem = 0;
  lvi.pszText = event_text[0];
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_INSERTITEM, 0,
                      ( LPARAM ) &lvi );
  lvi.iSubItem = 1;
  lvi.pszText = event_text[1];
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_SETITEM, 0,
                      ( LPARAM ) &lvi );
}

static int
deactivate_debugger( void )
{
  PostMessage( fuse_hWnd, WM_USER_EXIT_PROCESS_MESSAGES, 0, 0 );
  debugger_active = 0;
  fuse_emulation_unpause();
  return 0;
}

/* Set the disassembly to start at 'address' */
int
ui_debugger_disassemble( libspectrum_word address )
{
  /* Note: the scroll bar can not cope with "upper bound - page_size" value and
     higher. PC register, key and wheel scrolling are fine with that */
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  si.nPos = disassembly_top = address;
  SetScrollInfo( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ),
                 SB_CTL, &si, TRUE );

  /* And update the disassembly if the debugger is active */
  if( debugger_active ) update_disassembly();

  return 0;
}

/* Called when the disassembly scrollbar is moved */
static int
move_disassembly( WPARAM scroll_command )
{
  libspectrum_word address;
  int cursor_row;

  /* in Windows we have to read the command and scroll the scrollbar manually */
  switch( LOWORD( scroll_command ) ) {
    case SB_BOTTOM:
      if( disassembly_bottom == disassembly_min ) return 0;
      address = debugger_search_instruction( disassembly_min,
                                             -disassembly_page );
      break;
    case SB_TOP:
      if( disassembly_top == disassembly_min ) return 0;
      address = disassembly_min;
      break;
    case SB_LINEDOWN:
      if( disassembly_bottom == disassembly_min ) return 0;
      address = debugger_search_instruction( disassembly_top, 1 );
      break;
    case SB_LINEUP:
      if( disassembly_top == disassembly_min ) return 0;
      address = debugger_search_instruction( disassembly_top, -1);
      break;
    case SB_PAGEUP:
      address = debugger_search_instruction( disassembly_top,
                                             -disassembly_page );
      break;
    case SB_PAGEDOWN:
      address = debugger_search_instruction( disassembly_top,
                                             disassembly_page );
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      /* just set disassembly_top to that value */
      address = HIWORD( scroll_command );

      /* The scrollbar should constrain to min/max values */
      if( address > disassembly_max - disassembly_page )
        address = debugger_search_instruction( disassembly_min,
                                               -disassembly_page );
      break;
    default:
      return 1;
  }

  /* Get selected row */
  cursor_row = ListView_GetNextItem( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_PC ),
                                     -1, LVNI_SELECTED );

  /* Scroll to new position */
  ui_debugger_disassemble( address );

  /* Mark selected row */
  if( cursor_row >= 0 ) {
    ListView_SetItemState( GetDlgItem( fuse_hDBGWnd, IDC_DBG_LV_PC ),
                           cursor_row, LVIS_FOCUSED|LVIS_SELECTED,
                           LVIS_FOCUSED|LVIS_SELECTED );
  }

  return 0;
}

/* Evaluate the command currently in the entry box */
static void
evaluate_command( void )
{
  TCHAR *buffer;
  int buffer_size; 

  /* poll the size of the value in Evaluate text box first */
  buffer_size = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_ED_EVAL, WM_GETTEXTLENGTH,
                                   (WPARAM) 0, (LPARAM) 0 );
  buffer = malloc( ( buffer_size + 1 ) * sizeof( TCHAR ) );
  if( buffer == NULL ) {
    ui_error( UI_ERROR_ERROR, "Out of memory in %s.", __func__ );
    return;
  }

  /* get the value in Evaluate text box first */
  if( SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_ED_EVAL, WM_GETTEXT,
                          (WPARAM) ( buffer_size + 1 ),
                          (LPARAM) buffer ) != buffer_size ) {
    ui_error( UI_ERROR_ERROR,
              "Couldn't get the content of the Evaluate text box" );
    return;
  }

  /* FIXME: need to convert from TCHAR to char to comply with unicode */
  debugger_command_evaluate( buffer );

  free( buffer );
}

static void
win32ui_debugger_done_step( void )
{
  debugger_step();
}

static void
win32ui_debugger_done_continue( void )
{
  debugger_run();
}

static void
win32ui_debugger_break( void )
{
  debugger_mode = DEBUGGER_MODE_HALTED;

  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ), TRUE);
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), FALSE );
}

static void
delete_dialog( void )
{
  win32ui_debugger_done_close();
}

static void
win32ui_debugger_done_close( void )
{
  ShowWindow( fuse_hDBGWnd, SW_HIDE );
  win32ui_debugger_done_continue();
}

static INT_PTR CALLBACK
win32ui_debugger_proc( HWND hWnd GCC_UNUSED, UINT msg,
                       WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDCLOSE:
        case IDCANCEL:
          win32ui_debugger_done_close();
          return 0;
        case IDC_DBG_BTN_CONT:
          win32ui_debugger_done_continue();
          return 0;
        case IDC_DBG_BTN_BREAK:
          win32ui_debugger_break();
          return 0;
        case IDC_DBG_BTN_STEP:
          win32ui_debugger_done_step();
          return 0;
        case IDC_DBG_BTN_EVAL:
          evaluate_command();
          return 0;

        /* menus */
        case IDM_DBG_REG:
          toggle_display( DEBUGGER_PANE_REGISTERS, IDM_DBG_REG );
          return 0;
        case IDM_DBG_MEMMAP:
          toggle_display( DEBUGGER_PANE_MEMORYMAP, IDM_DBG_MEMMAP );
          return 0;
        case IDM_DBG_BPS:
          toggle_display( DEBUGGER_PANE_BREAKPOINTS, IDM_DBG_BPS );
          return 0;
        case IDM_DBG_DIS:
          toggle_display( DEBUGGER_PANE_DISASSEMBLY, IDM_DBG_DIS );
          return 0;
        case IDM_DBG_STACK:
          toggle_display( DEBUGGER_PANE_STACK, IDM_DBG_STACK );
          return 0;
        case IDM_DBG_EVENTS:
          toggle_display( DEBUGGER_PANE_EVENTS, IDM_DBG_EVENTS );
          return 0;
      }
      break;

    case WM_CLOSE:
      delete_dialog();
      return 0;

    case WM_NOTIFY:
      switch ( ( ( LPNMHDR ) lParam )->code ) {

        case NM_DBLCLK:
          switch( ( ( LPNMHDR ) lParam)->idFrom ) {

            case IDC_DBG_LV_EVENTS:
              events_click( ( LPNMITEMACTIVATE ) lParam );
              return 0;
            case IDC_DBG_LV_STACK:
              stack_click( ( LPNMITEMACTIVATE ) lParam );
              return 0;
          }
      }
      break;

    case WM_VSCROLL:
      if( ( HWND ) lParam != NULL )
        if( ! move_disassembly( wParam ) )
          return 0;
      break;
  }
  return FALSE;
}

static LRESULT CALLBACK
disassembly_key_press( HWND hWnd, WPARAM wParam )
{
  libspectrum_word initial_top, address;
  int cursor_row;

  initial_top = disassembly_top;

  /* Get selected row */
  cursor_row = ListView_GetNextItem( hWnd, -1, LVNI_SELECTED );

  switch( wParam ) {
  case VK_DOWN:
    if( cursor_row == disassembly_page - 1 ) {
      address = debugger_search_instruction( disassembly_top, 1 );
      ui_debugger_disassemble( address );
    }
    break;

  case VK_UP:
    if( cursor_row == 0 ) {
      address = debugger_search_instruction( disassembly_top, -1 );
      ui_debugger_disassemble( address );
    }
    break;

  case VK_NEXT:
    ui_debugger_disassemble( disassembly_bottom );
    break;

  case VK_PRIOR:
    address = debugger_search_instruction( disassembly_top,
                                           -disassembly_page );
    ui_debugger_disassemble( address );
    break;

  case VK_HOME:
    cursor_row = 0;
    ui_debugger_disassemble( disassembly_min );
    break;

  case VK_END:
    cursor_row = disassembly_page - 1;
    address = debugger_search_instruction( disassembly_min,
                                           -disassembly_page );
    ui_debugger_disassemble( address );
    break;
  }

  if( initial_top != disassembly_top ) {
    /* Mark selected row */
    if( cursor_row >= 0 ) {
      ListView_SetItemState( hWnd, cursor_row, LVIS_FOCUSED|LVIS_SELECTED,
                             LVIS_FOCUSED|LVIS_SELECTED );
    }
    return TRUE;
  }

  return FALSE;
}

static LRESULT CALLBACK
disassembly_wheel_scroll( HWND hWnd, WPARAM wParam )
{
  libspectrum_word address;
  int cursor_row;
  short delta;

  /* Get selected row */
  cursor_row = ListView_GetNextItem( hWnd, -1, LVNI_SELECTED );

  /* Convert wheel displacement to instruction displacement */
  delta = (short) HIWORD( wParam ) / WHEEL_DELTA;

  /* Scroll to new position */
  address = debugger_search_instruction( disassembly_top, -delta );
  ui_debugger_disassemble( address );

  /* Mark selected row */
  if( cursor_row >= 0 ) {
    ListView_SetItemState( hWnd, cursor_row, LVIS_FOCUSED|LVIS_SELECTED,
                           LVIS_FOCUSED|LVIS_SELECTED );
  }

  return TRUE;
}

static LRESULT CALLBACK
disassembly_listview_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  WNDPROC orig_proc;

  switch( msg ) {
  case WM_DESTROY:
    orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
    SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) orig_proc );
    RemoveProp( hWnd, "original_proc" );
    break;

  case WM_KEYDOWN:
    if( disassembly_key_press( hWnd, wParam ) ) return 0;
    break;

  case WM_MOUSEWHEEL:
    disassembly_wheel_scroll( hWnd, wParam );
    return 0;
  }

  orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
  return CallWindowProc( orig_proc, hWnd, msg, wParam, lParam );
}
