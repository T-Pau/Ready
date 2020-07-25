/* memorybrowser.c: the Win32 memory browser
   Copyright (c) 2008 Marek Januszewski

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
#include <tchar.h>
#include <windows.h>

#include "compat.h"
#include "fuse.h"
#include "memory_pages.h"
#include "win32internals.h"

#include "memorybrowser.h"

static INT_PTR CALLBACK
memorybrowser_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

static LRESULT CALLBACK
memory_listview_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

static void
memorybrowser_init( HWND hwndDlg );

void
menu_machine_memorybrowser( int action );

/* helper constants for memory listview's scrollbar */
static const int memorysb_min = 0x0000;
static const int memorysb_max = 0x10000;
static const int memorysb_step = 0x10;

/* Visual styles could change visible rows */
static int memorysb_page_inc = 0xa0;
static int memorysb_page_size = 0x140;
static int memorysb_page_rows = 20;

/* Address of first visible row */
static libspectrum_word memaddr = 0x0000;

static void
update_display( HWND hwndDlg, libspectrum_word base )
{
  int i, j;

  TCHAR buffer[ 8 + 64 + 20 ];
  TCHAR *text[] = { &buffer[0], &buffer[ 8 ], &buffer[ 8 + 64 ] };
  TCHAR buffer2[ 8 ];

  memaddr = base;

  SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_DELETEALLITEMS, 0, 0 );

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0; i < memorysb_page_rows; i++ ) {
    _sntprintf( text[0], 8, TEXT( "%04X" ), base );

    text[1][0] = '\0';
    for( j = 0; j < memorysb_step; j++, base++ ) {

      libspectrum_byte b = readbyte_internal( base );

      _sntprintf( buffer2, 4, TEXT( "%02X " ), b );
      _tcsncat( text[1], buffer2, 4 );

      text[2][j] = ( b >= 32 && b < 127 ) ? b : '.';
    }
    text[2][ 0x10 ] = '\0';

    /* append the item */
    lvi.iItem = SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                                    LVM_GETITEMCOUNT, 0, 0 );
    lvi.iSubItem = 0;
    lvi.pszText = text[0];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = text[1];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 2;
    lvi.pszText = text[2];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }
}

static int
scroller( HWND hwndDlg, WPARAM scroll_command )
{
  libspectrum_word base;
  SCROLLINFO si;

  memset( &si, 0, sizeof( si ) );
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  GetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si );

  int value = si.nPos;
  int selected = 0;
  
  /* in Windows we have to read the command and scroll the scrollbar manually */
  switch( LOWORD( scroll_command ) ) {
    case SB_BOTTOM:
      value = memorysb_max;
      selected = memorysb_page_rows - 1;
      break;
    case SB_TOP:
      value = memorysb_min;
      break;
    case SB_LINEDOWN:
      value += memorysb_step;
      selected = 1;
      break;
    case SB_LINEUP:
      value -= memorysb_step;
      break;
    case SB_PAGEUP:
      value -= memorysb_page_inc;
      break;
    case SB_PAGEDOWN:
      value += memorysb_page_inc;
      selected = memorysb_page_rows - 1;
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      value = HIWORD( scroll_command );
      break;
    default:
      return 1;
  }

  if( value > memorysb_max - memorysb_page_size )
    value = memorysb_max - memorysb_page_size;
  if( value < memorysb_min ) value = memorysb_min;

  /* Drop the low bits before displaying anything */
  base = value; base &= 0xfff0;

  if( base != memaddr ) {
    /* set the new scrollbar position */
    memset( &si, 0, sizeof(si) );
    si.cbSize = sizeof(si); 
    si.fMask = SIF_POS; 
    si.nPos = base;
    SetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si, TRUE );

    update_display( hwndDlg, base );
  }

  /* Select row according to last scroll command */
  ListView_SetItemState( GetDlgItem( hwndDlg, IDC_MEM_LV ), selected,
                         LVIS_SELECTED, LVIS_SELECTED );

  return 0;
}

void
menu_machine_memorybrowser( int action GCC_UNUSED )
{
  fuse_emulation_pause();

  DialogBox( fuse_hInstance, MAKEINTRESOURCE( IDD_MEM ),
             fuse_hWnd, (DLGPROC) memorybrowser_proc );

  fuse_emulation_unpause();
  return;
}

static INT_PTR CALLBACK
memorybrowser_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                    LPARAM lParam GCC_UNUSED )
{
  switch( uMsg )
  {
    case WM_INITDIALOG:
      memorybrowser_init( hwndDlg );
      return TRUE;

    case WM_COMMAND:
      if( LOWORD( wParam ) == IDCLOSE ||
          LOWORD( wParam ) == IDCANCEL ) {
        EndDialog( hwndDlg, 0 );
        return TRUE;
      }
      break;

    case WM_CLOSE: {
      EndDialog( hwndDlg, 0 );
      return TRUE;
    }

    case WM_VSCROLL:
      /* Accept vertical scroll from listview too */
      scroller( hwndDlg, wParam );
      return TRUE;
   
    case WM_MOUSEWHEEL:
    {
      /* get current position */
      SCROLLINFO si;
      memset( &si, 0, sizeof(si) );
      si.cbSize = sizeof(si);
      si.fMask = SIF_POS;
      GetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si );

      /* convert delta displacement to memory displacement */
      short delta = (short) HIWORD( wParam ) / WHEEL_DELTA;
      int value = si.nPos - delta * memorysb_step;
      if( value > memorysb_max - memorysb_page_size )
        value = memorysb_max - memorysb_page_size;
      if( value < memorysb_min ) value = memorysb_min;

      /* scroll to new position */
      scroller( hwndDlg, MAKEWPARAM( SB_THUMBPOSITION, value ) );
      return TRUE;
    }
  }

  return FALSE;
}

static LRESULT CALLBACK
memory_listview_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {

    case WM_DESTROY:
    {
      WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
      SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) orig_proc );
      RemoveProp( hWnd, "original_proc" );
      break;
    }

    case WM_KEYDOWN:
    {    
      WORD scroll_notify = 0xffff;

      switch( wParam )
      {
        case VK_UP:
            scroll_notify = SB_LINEUP;
            break;

        case VK_PRIOR:
            scroll_notify = SB_PAGEUP;
            break;

        case VK_NEXT:
            scroll_notify = SB_PAGEDOWN;
            break;

        case VK_DOWN:
            scroll_notify = SB_LINEDOWN;
            break;

        case VK_HOME:
            scroll_notify = SB_TOP;
            break;

        case VK_END:
            scroll_notify = SB_BOTTOM;
            break;
      }

      /* Inform parent window about key scrolling */
      if( scroll_notify != 0xffff ) {
        SendMessage( GetParent( hWnd ), WM_VSCROLL,
                     MAKEWPARAM( scroll_notify, 0 ), (LPARAM) NULL );
        return 0;
      }

      break;
    }

    case WM_MOUSEWHEEL:
    {
      /* Inform parent window about mouse scrolling */
      SendMessage( GetParent( hWnd ), WM_MOUSEWHEEL, wParam, lParam );
      return 0;
    }

  }

  WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
  return CallWindowProc( orig_proc, hWnd, msg, wParam, lParam );
}

static void
memorybrowser_init( HWND hwndDlg )
{
  size_t i;
  int error;
  HFONT font;

  const TCHAR *titles[] = { "Address", "Hex", "Data" };
  int column_widths[] = { 62, 348, 124 };

  error = win32ui_get_monospaced_font( &font ); if( error ) return;

  /* subclass listview to catch keydown and mousewheel messages */
  HWND hwnd_list = GetDlgItem( hwndDlg, IDC_MEM_LV );
  WNDPROC orig_proc = (WNDPROC) GetWindowLongPtr( hwnd_list, GWLP_WNDPROC );
  SetProp( hwnd_list, "original_proc", (HANDLE) orig_proc );
  SetWindowLongPtr( hwnd_list, GWLP_WNDPROC, 
                    (LONG_PTR) (WNDPROC) memory_listview_proc );

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  lv_ext_style |= LVS_EX_DOUBLEBUFFER;
  SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 3; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = column_widths[i];
    lvc.pszText = (TCHAR *)titles[i];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  /* set font of the listview to monospaced one */
  SendDlgItemMessage( hwndDlg, IDC_MEM_LV , WM_SETFONT,
                      (WPARAM) font, FALSE );

  /* Recalculate visible rows, Visual Styles could change rows height */
  memorysb_page_rows = SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                                           LVM_GETCOUNTPERPAGE, 0, 0 );
  memorysb_page_size = memorysb_page_rows * memorysb_step;
  memorysb_page_inc = memorysb_page_size / 2;

  /* set the scrollbar parameters */
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE; 
  si.nPos = memaddr;
  si.nMin = memorysb_min;
  si.nMax = memorysb_max;
  si.nPage = memorysb_page_size;
  SetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si, TRUE );

  update_display( hwndDlg, memaddr );

  /* Recalculate columns width, high DPI resolutions have larger sizes */
  ListView_SetColumnWidth( hwnd_list, 0, LVSCW_AUTOSIZE_USEHEADER );
  ListView_SetColumnWidth( hwnd_list, 1, LVSCW_AUTOSIZE );
  ListView_SetColumnWidth( hwnd_list, 2, LVSCW_AUTOSIZE_USEHEADER );
}
