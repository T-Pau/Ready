/* rollback.c: select a rollback point
   Copyright (c) 2004-2008 Philip Kendall, Marek Januszewski
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

#include <tchar.h>
#include <windows.h>
#include <commctrl.h> /* windows.h must be included prior to commctrl.h */

#include "fuse.h"
#include "ui/ui.h"
#include "win32internals.h"

#include "rollback.h"

static int current_block;

static void
dialog_init( HWND hwndDlg )
{
  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* Create the column in the listview */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = 100; /* FIXME: preferably calculate the whole length */
  lvc.pszText = (LPTSTR) TEXT( "Seconds" );
  SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV, LVM_INSERTCOLUMN, 0,
                        ( LPARAM ) &lvc );
}

static int
update_list( HWND hwndDlg, GSList *points )
{
  LV_ITEM lvi;

  SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV, LVM_DELETEALLITEMS, 0, 0 );

  lvi.iSubItem = 0;
  lvi.mask = LVIF_TEXT;

  while( points ) {
    TCHAR buffer[256];
    TCHAR *buffer2[1] = { buffer };

    _sntprintf( buffer, 256, "%.2f", GPOINTER_TO_INT( points->data ) / 50.0 );

    lvi.iItem = SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV,
                                    LVM_GETITEMCOUNT, 0, 0 );
    lvi.pszText = buffer2[0];
    SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );

    points = points->next;
  }

  return 0;
}

static INT_PTR CALLBACK
dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg ) {

    case WM_INITDIALOG:
      dialog_init( hwndDlg );
      update_list( hwndDlg, ( GSList * ) lParam );
      return FALSE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDOK:
          EndDialog( hwndDlg, SendDlgItemMessage( hwndDlg, IDC_ROLLBACK_LV,
                     LVM_GETSELECTIONMARK, 0, 0 ) );
          return 0;

        case IDCANCEL:
          EndDialog( hwndDlg, -1 );
          return 0;
      }
      break;

    case WM_CLOSE:
      EndDialog( hwndDlg, -1 );
      return 0;      
  }
  return FALSE;
}

int
ui_get_rollback_point( GSList *points )
{
  int result;
  
  fuse_emulation_pause();

  current_block = -1;

  result = DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_ROLLBACK ),
                           fuse_hWnd, dialog_proc, ( LPARAM ) points );
                           
  fuse_emulation_unpause();

  return result;
}
