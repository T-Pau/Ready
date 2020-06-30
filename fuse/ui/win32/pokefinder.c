/* pokefinder.c: Win32 interface to the poke finder
   Copyright (c) 2004 Marek Januszwski
   Copyright (c) 2015 Sergio Baldoví
   Copyright (c) 2015 Stuart Brady

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

#include <tchar.h>
#include <windows.h>

#include "debugger/debugger.h"
#include "pokefinder.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"
#include "win32internals.h"

void menu_machine_pokefinder( int action );
void move_button( int button, int dlg_height );
static void possible_click( LPNMITEMACTIVATE lpnmitem );
static void update_pokefinder( void );
static void win32ui_pokefinder_incremented( void );
static void win32ui_pokefinder_decremented( void );
static void win32ui_pokefinder_search( void );
static void win32ui_pokefinder_reset( void );
static void win32ui_pokefinder_close( void );

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];
int initial_width = 0;
int initial_height = 0;
HWND lv_hWnd = NULL;
int lv_width = 0;

static INT_PTR CALLBACK
win32ui_pokefinder_proc( HWND hWnd GCC_UNUSED, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
  int height;

  switch( msg ) {

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDCLOSE:
        case IDCANCEL:
          win32ui_pokefinder_close();
          return TRUE;
        case IDC_PF_INC:
          win32ui_pokefinder_incremented();
          return TRUE;
        case IDC_PF_DEC:
          win32ui_pokefinder_decremented();
          return TRUE;
        case IDC_PF_SEARCH:
          win32ui_pokefinder_search();
          return TRUE;
        case IDC_PF_RESET:
          win32ui_pokefinder_reset();
          return TRUE;
      }
      break;

    case WM_SIZE:
      height = HIWORD( lParam );
      move_button( IDC_PF_INC, height );
      move_button( IDC_PF_DEC, height );
      move_button( IDC_PF_SEARCH, height );
      move_button( IDC_PF_RESET, height );
      move_button( IDCLOSE, height );
      return TRUE;
	
    case WM_CLOSE:
      win32ui_pokefinder_close();
      return TRUE;

    case WM_NOTIFY:
      switch ( ( ( LPNMHDR ) lParam )->code ) {
        case NM_DBLCLK:
          possible_click( ( LPNMITEMACTIVATE ) lParam );
          return TRUE; /* The return value for this notification is not used */
      }
      break;      
  }
  return FALSE;
}

void
move_button( int button, int dlg_height )
{
  HWND ctrl_hWnd;
  RECT rect;
  int x, y;

  ctrl_hWnd = GetDlgItem( fuse_hPFWnd, button );
  GetWindowRect( ctrl_hWnd, &rect );
  MapWindowPoints( 0, fuse_hPFWnd, (POINT *)&rect, 2 );
  x = rect.left;
  y = dlg_height - ( rect.bottom - rect.top ) - 10;
  SetWindowPos( ctrl_hWnd, 0, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
}

static void
update_pokefinder( void )
{
  size_t page, offset, bank, bank_offset;
  TCHAR buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };
  int rcx, rcy;
  DWORD dw_res;

  /* clear the listview */
  SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_DELETEALLITEMS, 0, 0 );

  /* display suspected locations if < 20 */
  int i = 0;
  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which = 0;

    for( page = 0; page < MEMORY_PAGES_IN_16K * SPECTRUM_RAM_PAGES; page++ ) {
      memory_page *mapping = &memory_map_ram[page];
      bank = mapping->page_num;

      for( offset = 0; offset < MEMORY_PAGE_SIZE; offset++ )
	if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
	  bank_offset = mapping->offset + offset;

	  possible_page[ which ] = bank;
	  possible_offset[ which ] = bank_offset;
	  which++;
	
	  _sntprintf( possible_text[0], 128, "%d", (unsigned)bank );
	  _sntprintf( possible_text[1], 128, "0x%04X", (unsigned)bank_offset );

          /* set new count of items */
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_SETITEMCOUNT,
                              i, 0 );

          /* add the item */
          lvi.iItem = i;
          lvi.iSubItem = 0;
          lvi.pszText = possible_text[0];
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTITEM, 0,
                              ( LPARAM ) &lvi );
          lvi.iSubItem = 1;
          lvi.pszText = possible_text[1];
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_SETITEM, 0,
                              ( LPARAM ) &lvi );

          i++;
        }
    }

    /* show the listview */
    ShowWindow( lv_hWnd, SW_SHOW );

    /* change the size of the listview */
    dw_res = SendMessage( lv_hWnd, LVM_APPROXIMATEVIEWRECT, pokefinder_count,
                          MAKELPARAM( -1, -1 ) );
    rcx = lv_width; /* same width */
    rcy = HIWORD( dw_res );
    SetWindowPos( lv_hWnd, NULL, 0, 0, rcx, rcy,
                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

    rcx = initial_width;
    rcy += initial_height + 10;
  } else {
    /* hide the listview */
    ShowWindow( lv_hWnd, SW_HIDE );

    rcx = initial_width;
    rcy = initial_height;
  }

  /* change the size of the dialog */
  SetWindowPos( fuse_hPFWnd, NULL, 0, 0, rcx, rcy,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

  /* print possible locations */
  _sntprintf( buffer, 256, "Possible locations: %lu",
              (unsigned long)pokefinder_count );
  SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LOCATIONS, WM_SETTEXT, 0,
                      (LPARAM) buffer );
}

void
menu_machine_pokefinder( int action GCC_UNUSED )
{
  RECT rect;
  int cx;

  if (fuse_hPFWnd == NULL) {
    /* FIXME: Implement accelerators for this dialog */
    fuse_hPFWnd = CreateDialog( fuse_hInstance,
                                MAKEINTRESOURCE( IDD_POKEFINDER ),
                                fuse_hWnd, 
                                ( DLGPROC ) win32ui_pokefinder_proc );
    if ( fuse_hPFWnd == NULL ) {
      win32_verror( 1 ); /* FIXME: improve this function */
      return;
    }

    /* store initial dialog dimensions */
    GetWindowRect( fuse_hPFWnd, &rect );
    initial_width = rect.right - rect.left;
    initial_height = rect.bottom - rect.top;

    /* Set text limit */
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_EDIT, EM_LIMITTEXT, 4, 0 );

    /* set extended listview style to select full row, when an item
       is selected */
    DWORD lv_ext_style;
    lv_ext_style = SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST,
                                       LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
    lv_ext_style |= LVS_EX_FULLROWSELECT;
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST,
                        LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

    /* calculate columns width */
    lv_hWnd = GetDlgItem( fuse_hPFWnd, IDC_PF_LIST );
    GetClientRect( lv_hWnd, &rect );
    cx = rect.right - rect.left;
    cx >>= 1;

    /* create columns */
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = cx;
    lvc.pszText = (LPTSTR) TEXT( "Page" );
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTCOLUMN, 0,
                        ( LPARAM ) &lvc );
    lvc.mask |= LVCF_SUBITEM;
    lvc.cx = cx;
    lvc.pszText = (LPTSTR) TEXT( "Offset" );
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTCOLUMN, 1,
                        ( LPARAM ) &lvc );

    /* store listview width */
    GetWindowRect( lv_hWnd, &rect );
    lv_width = rect.right - rect.left;
  } else {
    SetActiveWindow( fuse_hPFWnd );
  }
  update_pokefinder();
}

static void
win32ui_pokefinder_incremented( void )
{
  pokefinder_incremented();
  update_pokefinder();
}

static void
win32ui_pokefinder_decremented( void )
{
  pokefinder_decremented();
  update_pokefinder();
}

static void
win32ui_pokefinder_search( void )
{
  long value;
  TCHAR *buffer, *endptr;
  int buffer_size, base;
  HWND hwnd_control;

  /* poll the size of the value in Search box first */
  buffer_size = SendDlgItemMessage( fuse_hPFWnd, IDC_PF_EDIT, WM_GETTEXTLENGTH,
                                   (WPARAM) 0, (LPARAM) 0 );
  buffer = malloc( ( buffer_size + 1 ) * sizeof( TCHAR ) );
  if( buffer == NULL ) {
    ui_error( UI_ERROR_ERROR, "Out of memory in %s.", __func__ );
    return;
  }

  /* get the value in Search box first */
  if( SendDlgItemMessage( fuse_hPFWnd, IDC_PF_EDIT, WM_GETTEXT,
                          (WPARAM) ( buffer_size + 1 ),
                          (LPARAM) buffer ) != buffer_size ) {
    ui_error( UI_ERROR_ERROR, "Couldn't get the content of the Search text box" );
    return;
  }

  errno = 0;
  base = ( !_tcsncmp( _T("0x"), buffer, strlen( _T("0x") ) ) )? 16 : 10;
  value = _tcstol( buffer, &endptr, base );

  if( errno || value < 0 || value > 255 || endptr == buffer ) {
    free( buffer );
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 255" );
    hwnd_control = GetDlgItem( fuse_hPFWnd, IDC_PF_EDIT );
    SendMessage( fuse_hPFWnd, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }
  free( buffer );

  pokefinder_search( value );
  update_pokefinder();
}

static void
win32ui_pokefinder_reset( void )
{
  pokefinder_clear();
  update_pokefinder();
}

static void
win32ui_pokefinder_close( void )
{
  DestroyWindow( fuse_hPFWnd );
  fuse_hPFWnd = NULL;
}

static void
possible_click( LPNMITEMACTIVATE lpnmitem )
{
  /* FIXME: implement equivalent of GTK's select-via-keyboard to enter here */
 
  int error;
  libspectrum_word row;

  if( lpnmitem->iItem < 0 ) return;
        
  row = lpnmitem->iItem;
  
  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_WRITE, memory_source_ram, possible_page[ row ],
    possible_offset[ row ], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL
  );
  if( error ) return;
  
  ui_debugger_update();
}

void
win32ui_pokefinder_clear( void )
{
  pokefinder_clear();
  if( fuse_hPFWnd != NULL ) update_pokefinder();
}
