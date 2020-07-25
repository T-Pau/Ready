/* pokemem.c: Win32 interface to the poke memory
   Copyright (c) 2011-2015 Philip Kendall, Sergio Baldoví

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

#include "fuse.h"
#include "pokemem.h"
#include "pokefinder/pokemem.h"
#include "ui/ui.h"
#include "win32internals.h"

void menu_machine_pokememory( int action );
void initialize_dialog( HWND hwnd_dialog );
void create_custom_edit( HWND parent, int item, int subitem );
void move_custom_edit( HWND hwnd_c_edit, HWND hwnd_parent );
void trainer_add( gpointer data, gpointer user_data );
void pokemem_update_list( void );
void pokemem_update_trainer( int index );
void pokemem_add_custom_poke( void );
INT_PTR CALLBACK win32ui_pokemem_proc( HWND hWnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam );
LRESULT CALLBACK listview_proc( HWND hWnd, UINT msg, WPARAM wParam,
                                LPARAM lParam );
LRESULT CALLBACK edit_proc( HWND hWnd, UINT msg, WPARAM wParam,
                            LPARAM lParam );

HWND fuse_hPMWnd = NULL;
HWND hwnd_edit = NULL;
int item_edit, subitem_edit;
BOOL cancel_edit = FALSE;

void
menu_machine_pokememory( int action GCC_UNUSED )
{
  fuse_emulation_pause();

  pokemem_autoload_pokfile();

  DialogBox( fuse_hInstance, MAKEINTRESOURCE( IDD_POKEMEM ),
             fuse_hWnd, (DLGPROC) win32ui_pokemem_proc );

  fuse_emulation_unpause();
}

void
ui_pokemem_selector( const char *filename )
{
  fuse_emulation_pause();

  pokemem_read_from_file( filename );

  DialogBox( fuse_hInstance, MAKEINTRESOURCE( IDD_POKEMEM ),
             fuse_hWnd, (DLGPROC) win32ui_pokemem_proc );

  fuse_emulation_unpause();
}

INT_PTR CALLBACK
win32ui_pokemem_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_INITDIALOG:
      initialize_dialog( hWnd );
      return TRUE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDOK:
          pokemem_update_list();
          EndDialog( hWnd, wParam );
          return 0;
        case IDCLOSE:
        case IDCANCEL:
          EndDialog( hWnd, wParam );
          return 0;
        case IDC_PM_ADD:
          pokemem_add_custom_poke();
          return 0;
      }
      break;

    case WM_CLOSE:
      EndDialog( hWnd, wParam );
      return 0;

    case WM_NOTIFY:
      if( LOWORD( wParam ) == IDC_PM_LIST ) {
        NMHDR *nmhdr = (NMHDR *) lParam;
        LPNMLISTVIEW lpnmitem;

        switch( nmhdr->code ) {

          case LVN_ITEMCHANGING:
          {
            lpnmitem = (LPNMLISTVIEW) lParam;
            if( lpnmitem->uChanged & LVIF_STATE ) {
              unsigned int new_state, old_state;
              new_state = ( lpnmitem->uNewState & LVIS_STATEIMAGEMASK ) >> 12;
              old_state = ( lpnmitem->uOldState & LVIS_STATEIMAGEMASK ) >> 12;
              trainer_t *trainer = (trainer_t *)lpnmitem->lParam;

              /* Prevent the check of disabled trainers */
              if( new_state != old_state && trainer->disabled )
              {
                SetWindowLongPtr( hWnd, DWLP_MSGRESULT, TRUE );
                return TRUE;
              }
            }
            break;
          }

          case LVN_ITEMCHANGED:
          {
            lpnmitem = (LPNMLISTVIEW) lParam;

            if( lpnmitem->uChanged & LVIF_STATE && lpnmitem->iItem >= 0 ) {
              unsigned int new_state, old_state;
              new_state = ( lpnmitem->uNewState & LVIS_STATEIMAGEMASK ) >> 12;
              old_state = ( lpnmitem->uOldState & LVIS_STATEIMAGEMASK ) >> 12;
              trainer_t *trainer = (trainer_t *)lpnmitem->lParam;

              /* Trainer checked, ask for custom value if needed */
              if( new_state != old_state && new_state == 2 && !trainer->active
                  && trainer->ask_value ) {
                ListView_SetItemState( nmhdr->hwndFrom, lpnmitem->iItem,
                                       LVIS_SELECTED, LVIS_SELECTED );
                create_custom_edit( nmhdr->hwndFrom, lpnmitem->iItem, 1 );
              }

              break;
            }
          }
        }
      }
      break;
  }

  return FALSE;
}

LRESULT CALLBACK
listview_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {

    case WM_DESTROY:
    {
      WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
      SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) orig_proc );
      RemoveProp( hWnd, "original_proc" );
      break;
    }

    case WM_VSCROLL:
      /* do scroll and move edit */
      if( hwnd_edit ) {
        PostMessage( hWnd, WM_USER, (WPARAM) hwnd_edit, (LPARAM) hWnd );
      }
      break;

    case WM_HSCROLL:
      /* do scroll and move edit */
      if( hwnd_edit ) {
        PostMessage( hWnd, WM_USER, (WPARAM) hwnd_edit, (LPARAM) hWnd );
      }
      break;

    case WM_USER:
      move_custom_edit( (HWND)wParam, (HWND)lParam );
      break;

    case WM_LBUTTONDOWN:
    {
      if( hwnd_edit ) SendMessage( hwnd_edit, WM_KILLFOCUS,0,0 );

      LVHITTESTINFO itemclicked;
      itemclicked.pt.x = (long) LOWORD( lParam );
      itemclicked.pt.y = (long) HIWORD( lParam );

      int result = ListView_SubItemHitTest( hWnd, &itemclicked );

      /* Clicked on Value column? */
      if( result >= 0 && itemclicked.iSubItem ) {

        /* Get trainer */
        LV_ITEM lvi;
        memset( &lvi, 0, sizeof( lvi ) );
        lvi.mask = LVIF_PARAM;
        lvi.iItem = itemclicked.iItem;
        SendMessage( hWnd, LVM_GETITEM, 0, (LPARAM)&lvi );
        trainer_t *trainer = (trainer_t *)lvi.lParam;

        /* Ask for custom value */
        if( !trainer->active && trainer->ask_value  ) {
          ListView_SetItemState( hWnd, itemclicked.iItem, LVIS_SELECTED,
                                 LVIS_SELECTED );
          create_custom_edit( hWnd, itemclicked.iItem, 1 );
          return TRUE;
        }
      }

      break;
    }

    case WM_NOTIFY:
    {
      NMHDR *nmhdr = (NMHDR *) lParam;

      switch( nmhdr->code ) {

        case LVN_ENDLABELEDIT:
        {
          LV_DISPINFO* dispinfo = (LV_DISPINFO*)lParam;
          LVITEM lvi;
          long val;

          /* Edit cancelled? */
          if( !dispinfo->item.pszText ) return TRUE;

          memset( &lvi, 0, sizeof( lvi ) );
          lvi.iItem = dispinfo->item.iItem;
          lvi.iSubItem = dispinfo->item.iSubItem;
          lvi.pszText = ( dispinfo->item.cchTextMax > 1 )?
                        dispinfo->item.pszText : (LPTSTR) TEXT( "0" );

          /* Validate value */
          val = _ttol( lvi.pszText );
          if( val > 256 ) {
            val = 0;
            lvi.pszText = (LPTSTR) TEXT( "0" );
          }

          /* Update listview */
          SendMessage( hWnd, LVM_SETITEMTEXT, (WPARAM) lvi.iItem,
                       (LPARAM) &lvi );

          /* Update trainer */
          memset( &lvi, 0, sizeof( lvi ) );
          lvi.mask = LVIF_PARAM;
          lvi.iItem = dispinfo->item.iItem;
          SendMessage( hWnd, LVM_GETITEM, 0, (LPARAM) &lvi );
          trainer_t *trainer = (trainer_t *)lvi.lParam;
          trainer->value = val;

          return TRUE;
        }
      }
    }
  }

  WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
  return CallWindowProc( orig_proc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK
edit_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {

    case WM_DESTROY:
    {
      WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
      SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR) orig_proc );
      RemoveProp( hWnd, "original_proc" );
      hwnd_edit = NULL;
      break;
    }

    case WM_GETDLGCODE:
    {
      /* Allow ESC and Return keystroke capture */
      MSG *m = (MSG *)lParam;
      if( m && ( m->wParam == VK_RETURN || m->wParam == VK_ESCAPE ) )
        return DLGC_WANTALLKEYS;
      break;
    }

    case WM_KEYDOWN:
    {
      if( wParam == VK_RETURN ) {
        /* Lost focus to confirm value */
        SetFocus( GetParent( hWnd ) );
        return TRUE;
      }
      else if ( wParam == VK_ESCAPE ) {
        /* Lost focus to cancel edit */
        cancel_edit = TRUE;
        SetFocus( GetParent( hWnd ) );
        return TRUE;
      }
      break;
    }

    case WM_KILLFOCUS:
    {
      /* Notify list end of label edit */
      LV_DISPINFO lvDispinfo;
      memset( &lvDispinfo, 0, sizeof( LV_DISPINFO ) );
      lvDispinfo.hdr.hwndFrom = hWnd;
      lvDispinfo.hdr.idFrom = GetDlgCtrlID( hWnd );
      lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
      lvDispinfo.item.mask = LVIF_TEXT;
      lvDispinfo.item.iItem = item_edit;
      lvDispinfo.item.iSubItem = subitem_edit;
      lvDispinfo.item.pszText = NULL;

      if( !cancel_edit ) {
        TCHAR szEditText[4];
        GetWindowText( hWnd, szEditText, 4 );
        lvDispinfo.item.pszText = szEditText;
        lvDispinfo.item.cchTextMax = lstrlen( szEditText ) + 1;
      }
      SendMessage( GetParent( hWnd ), WM_NOTIFY, (WPARAM) IDC_PM_LIST,
                   (LPARAM) &lvDispinfo );

      DestroyWindow( hWnd );
      return TRUE;
    }
  }

  WNDPROC orig_proc = (WNDPROC) GetProp( hWnd, "original_proc" );
  return CallWindowProc( orig_proc, hWnd, msg, wParam, lParam );
}

void
initialize_dialog( HWND hwnd_dialog )
{
  HWND hwnd_list;
  RECT rect;
  int cx;

  fuse_hPMWnd = hwnd_dialog;
  hwnd_list = GetDlgItem( hwnd_dialog, IDC_PM_LIST );

  /* Replace message handler */
  WNDPROC orig_proc = (WNDPROC) GetWindowLongPtr( hwnd_list, GWLP_WNDPROC );
  SetProp( hwnd_list, "original_proc", (HANDLE) orig_proc );
  SetWindowLongPtr( hwnd_list, GWLP_WNDPROC, (LONG_PTR) (WNDPROC) listview_proc );

  /* Set text limits */
  SendDlgItemMessage( hwnd_dialog, IDC_PM_BANK_EDIT,  EM_LIMITTEXT, 1, 0 );
  SendDlgItemMessage( hwnd_dialog, IDC_PM_ADDR_EDIT,  EM_LIMITTEXT, 5, 0 );
  SendDlgItemMessage( hwnd_dialog, IDC_PM_VALUE_EDIT, EM_LIMITTEXT, 3, 0 );

  /* Set extended listview style to select full row, when an item
     is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendMessage( hwnd_list, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  lv_ext_style |= LVS_EX_CHECKBOXES;
  SendMessage( hwnd_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style );

  /* Calculate columns width, reserve space for vertical scrollbar */
  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof( NONCLIENTMETRICS );
  SystemParametersInfo( SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0 );
  GetClientRect( hwnd_list, &rect );
  cx = rect.right - rect.left - ncm.iScrollWidth;

  /* Create trainer column */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = cx - ( cx >> 2 );
  lvc.pszText = (LPTSTR) TEXT( "Trainer" );
  SendMessage( hwnd_list, LVM_INSERTCOLUMN, 0, (LPARAM) &lvc );

  /* Create value column */
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  lvc.cx = cx >> 2;
  lvc.pszText = (LPTSTR) TEXT( "Value" );
  SendMessage( hwnd_list, LVM_INSERTCOLUMN, 1, (LPARAM) &lvc );

  /* Fill listview with data */
  if( trainer_list ) {
    /* Allocate memory */
    guint length = g_slist_length( trainer_list );
    SendMessage( hwnd_list, LVM_SETITEMCOUNT, length, 0 );

    /* Loop data model */
    g_slist_foreach( trainer_list, trainer_add, hwnd_list );
  }
}

void
trainer_add( gpointer data, gpointer user_data )
{
  trainer_t *trainer = data;
  HWND hwnd_list = (HWND) user_data;
  TCHAR buffer[81];
  int i, state;
  LV_ITEM lvi;

  if( !trainer ) return;

  /* Get count of items */
  i = SendMessage( hwnd_list, LVM_GETITEMCOUNT, 0, 0 );

  /* add trainer */
  memset( &lvi, 0, sizeof( lvi ) );
  lvi.mask = LVIF_TEXT | LVIF_PARAM;
  lvi.iItem = i;
  lvi.iSubItem = 0;
  _sntprintf( buffer, 80, "%s", trainer->name );
  buffer[80] = '\0';
  lvi.pszText = buffer;
  lvi.lParam = (LPARAM)trainer;
  SendMessage( hwnd_list, LVM_INSERTITEM, 0, (LPARAM) &lvi );

  /* add value */
  if( trainer->ask_value ) {
    _sntprintf( buffer, 80, "%d", trainer->value );
    memset( &lvi, 0, sizeof( lvi ) );
    lvi.mask = LVIF_TEXT;
    lvi.iItem = i;
    lvi.iSubItem = 1;
    lvi.pszText = buffer;
    SendMessage( hwnd_list, LVM_SETITEM, 0, (LPARAM) &lvi );
  }

  /* mark trainer checked or disabled */
  if( trainer->disabled )
    state = 0;
  else
    state = ( trainer->active )? 2 : 1;

  memset( &lvi, 0, sizeof( lvi ) );
  lvi.mask = LVIF_STATE;
  lvi.state = INDEXTOSTATEIMAGEMASK( state );
  lvi.stateMask = LVIS_STATEIMAGEMASK;
  SendMessage( hwnd_list, LVM_SETITEMSTATE, i, (LPARAM) &lvi );
}

void
create_custom_edit( HWND parent, int item, int subitem )
{
  RECT subitemrect;
  int width, height;

  /* Get item bounds */
  subitemrect.top = subitem;
  subitemrect.left = LVIR_BOUNDS;
  SendMessage( parent, LVM_GETSUBITEMRECT, item, (LPARAM) &subitemrect );
  height = subitemrect.bottom - subitemrect.top;
  width = subitemrect.right - subitemrect.left;

  /* Create custom edit */
  hwnd_edit = CreateWindowEx( WS_EX_CLIENTEDGE, "EDIT", "",
                              WS_CHILD | WS_VISIBLE | ES_NUMBER,
                              subitemrect.left, subitemrect.top, width, height,
                              parent, (HMENU) IDC_PM_LIST_EDIT,
                              GetModuleHandle( NULL ), NULL );
  if( !hwnd_edit ) return;
 
  /* Replace message handler */
  WNDPROC orig_proc = (WNDPROC) GetWindowLongPtr( hwnd_edit, GWLP_WNDPROC );
  SetProp( hwnd_edit, "original_proc", (HANDLE) orig_proc );
  SetWindowLongPtr( hwnd_edit, GWLP_WNDPROC, (LONG_PTR) (WNDPROC) edit_proc );

  /* Set proper font custom edit */
  HFONT hFont = (HFONT) SendMessage( parent, WM_GETFONT, 0, 0 );
  SendMessage( hwnd_edit, WM_SETFONT, (WPARAM) hFont, (LPARAM) FALSE );

  /* Set custom edit text */
  TCHAR szEditText[4];
  SendMessage( hwnd_edit, EM_LIMITTEXT, 3, 0 );
  ListView_GetItemText( parent, item, subitem, szEditText, 4 );
  SendMessage( hwnd_edit, WM_SETTEXT, 0, (LPARAM) szEditText );

  /* Focus edit control and select text */
  SetFocus( hwnd_edit );
  SendMessage( hwnd_edit, EM_SETSEL, 0, -1 );

  item_edit = item;
  subitem_edit = subitem;
  cancel_edit = FALSE;
}

void
move_custom_edit( HWND hwnd_c_edit, HWND hwnd_parent )
{
  HWND hwnd_header;
  RECT header_rect, item_rect;
  int width, height;

  /* Get current item position */
  item_rect.top = subitem_edit;
  item_rect.left = LVIR_BOUNDS;
  SendMessage( hwnd_parent, LVM_GETSUBITEMRECT, item_edit, (LPARAM) &item_rect );

  /* Get header position */
  hwnd_header = (HWND)SendMessage( hwnd_parent, LVM_GETHEADER, 0, 0 );
  GetWindowRect( hwnd_header, &header_rect );
  MapWindowPoints( HWND_DESKTOP, hwnd_parent, (LPPOINT) &header_rect, 2 );

  /* Move Edit along with item */
  if( item_rect.top >= header_rect.bottom ) {
    width = item_rect.right - item_rect.left;
    height = item_rect.bottom - item_rect.top;
    InvalidateRect( hwnd_parent, &item_rect, FALSE );
    SetWindowPos( hwnd_c_edit, NULL, item_rect.left, item_rect.top,
                  width, height, SWP_NOZORDER | SWP_SHOWWINDOW );
  }
  else {
    /* Hide custom edit when scrolls above header */
    SetWindowPos( hwnd_c_edit, NULL, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_HIDEWINDOW );
  }
}

void
pokemem_update_list( void )
{
  int i, items;

  /* Get count of items */
  items = SendDlgItemMessage( fuse_hPMWnd, IDC_PM_LIST, LVM_GETITEMCOUNT,
                              0, 0 );

  for( i = 0; i < items; i++ ) {
    pokemem_update_trainer( i );
  }
}

void
pokemem_update_trainer( int index )
{
  int selected;
  trainer_t *trainer;
  LV_ITEM lvi;

  memset( &lvi, 0, sizeof( lvi ) );
  lvi.mask = LVIF_STATE | LVIF_PARAM;
  lvi.iItem = index;
  lvi.stateMask = LVIS_STATEIMAGEMASK;
  SendDlgItemMessage( fuse_hPMWnd, IDC_PM_LIST, LVM_GETITEM, 0, (LPARAM)&lvi );

  trainer = (trainer_t *) lvi.lParam;
  selected = ( ( lvi.state >> 12 ) == 2 );

  if( selected ) {
    pokemem_trainer_activate( trainer );
  } else {
    pokemem_trainer_deactivate( trainer );
  }
}

void
pokemem_add_custom_poke( void )
{
  long b, a, v;
  TCHAR buffer[8];
  int length, buffer_size = 8;
  HWND hwnd_control;
  trainer_t *trainer;

  /* Parse bank */
  length = SendDlgItemMessage( fuse_hPMWnd, IDC_PM_BANK_EDIT, WM_GETTEXT,
                               (WPARAM) buffer_size, (LPARAM) buffer );

  b = ( length )? _ttol( buffer ) : 8;

  if( b < 0 || b > 8 ) {
    ui_error( UI_ERROR_ERROR, "Invalid bank: use a number from 0 to 8" );
    hwnd_control = GetDlgItem( fuse_hPMWnd, IDC_PM_BANK_EDIT );
    SendMessage( fuse_hPMWnd, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }

  /* Parse address */
  length = SendDlgItemMessage( fuse_hPMWnd, IDC_PM_ADDR_EDIT, WM_GETTEXT,
                               (WPARAM) buffer_size, (LPARAM) buffer );

  /* TODO: accept hex address */
  a = ( length )? _ttol( buffer ) : 0;

  if( !length || a < 0 || a > 65535  ) {
    ui_error( UI_ERROR_ERROR,
              "Invalid address: use a number from 0 to 65535" );
    hwnd_control = GetDlgItem( fuse_hPMWnd, IDC_PM_ADDR_EDIT );
    SendMessage( fuse_hPMWnd, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }

  /* Parse value */
  length = SendDlgItemMessage( fuse_hPMWnd, IDC_PM_VALUE_EDIT, WM_GETTEXT,
                               (WPARAM) buffer_size, (LPARAM) buffer );

  v = ( length )? _ttol( buffer ) : 0;

  if( !length || v < 0 || v > 256 ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use a number from 0 to 256" );
    hwnd_control = GetDlgItem( fuse_hPMWnd, IDC_PM_VALUE_EDIT );
    SendMessage( fuse_hPMWnd, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }

  /* Updadate model and view */
  trainer = pokemem_trainer_list_add( b, a, v );
  if( !trainer ) {
    ui_error( UI_ERROR_ERROR, "Cannot add trainer" );
    return;
  }

  hwnd_control = GetDlgItem( fuse_hPMWnd, IDC_PM_LIST );
  trainer_add( trainer, hwnd_control );

  /* Mark custom trainer for activate */
  if( !trainer->active && !trainer->disabled ) {
    LV_ITEM lvi;
    int index;

    memset( &lvi, 0, sizeof( lvi ) );
    lvi.mask = LVIF_STATE;
    lvi.state = INDEXTOSTATEIMAGEMASK( 2 );
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    index = SendDlgItemMessage( fuse_hPMWnd, IDC_PM_LIST, LVM_GETITEMCOUNT,
                                0, 0 ) - 1;
    SendDlgItemMessage( fuse_hPMWnd, IDC_PM_LIST, LVM_SETITEMSTATE,
                        (WPARAM) index, (LPARAM) &lvi );
  }

  /* Clear custom fields */
  SetDlgItemText( fuse_hPMWnd, IDC_PM_BANK_EDIT,  NULL );
  SetDlgItemText( fuse_hPMWnd, IDC_PM_ADDR_EDIT,  NULL );
  SetDlgItemText( fuse_hPMWnd, IDC_PM_VALUE_EDIT, NULL );

  /* Focus for new input */
  hwnd_control = GetDlgItem( fuse_hPMWnd, IDC_PM_ADDR_EDIT );
  SendMessage( fuse_hPMWnd, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
}
