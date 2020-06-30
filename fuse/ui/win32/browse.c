/* browse.c: tape browser dialog box
   Copyright (c) 2002-2008 Philip Kendall, Marek Januszewski
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

#include <libspectrum.h>
#include <tchar.h>
#include <windows.h>

#include "fuse.h"
#include "tape.h"
#include "ui/ui.h"
#include "win32internals.h"

#include "browse.h"

static INT_PTR CALLBACK dialog_proc( HWND hwndDlg, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam );
static void add_block_details( libspectrum_tape_block *block, void *user_data );
static void select_row( LPNMITEMACTIVATE lpnmitem );

static HWND dialog;             /* The dialog box itself */

static int dialog_created;	/* Have we created the dialog box yet? */

void
menu_media_tape_browse( int action )
{
  /* Firstly, stop emulation */
  fuse_emulation_pause();

  if( !dialog_created ) {
    dialog = CreateDialog( fuse_hInstance, MAKEINTRESOURCE( IDD_BROWSE ),
                           fuse_hWnd, dialog_proc );
    if( dialog == NULL ) { fuse_emulation_unpause(); return; }
  }

  if( ui_tape_browser_update( UI_TAPE_BROWSER_NEW_TAPE, NULL ) ) {
    fuse_emulation_unpause();
    return;
  }

  ShowWindow( dialog, SW_SHOW );

  /* Carry on with emulation */
  fuse_emulation_unpause();
}

static void
dialog_init( HWND hwndDlg )
{
  size_t i;
  LPCTSTR titles[3] = { _T( "" ), _T( "Block type" ), _T( "Data" ) };
  int titles_widths[3] = { 16, 115, 150 };

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( hwndDlg, IDC_BROWSE_LV,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( hwndDlg, IDC_BROWSE_LV,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* Create columns in the listview */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 3; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = titles_widths[i];
    lvc.pszText = (LPTSTR)titles[i];
    SendDlgItemMessage( hwndDlg, IDC_BROWSE_LV, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }

  /* create image list for the listview */
  HBITMAP icon_tape_marker, icon_tape_marker_mask;
  BITMAP bmp;
  HIMAGELIST himl;

  /* FIXME: need to destroy those objects later */
  icon_tape_marker = LoadImage( fuse_hInstance, "win32bmp_tape_marker", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_tape_marker_mask = LoadImage( fuse_hInstance, "win32bmp_tape_marker_mask", 
                                     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  GetObject( icon_tape_marker, sizeof( bmp ), &bmp );
  
  /* FIXME: destroy the list later */
  himl = ImageList_Create( bmp.bmWidth, bmp.bmHeight,
                           ILC_COLOR | ILC_MASK, 1, 0 );
  
  ImageList_Add( himl, icon_tape_marker, icon_tape_marker_mask );
  
  SendDlgItemMessage( hwndDlg, IDC_BROWSE_LV, LVM_SETIMAGELIST,
                      LVSIL_SMALL, ( LPARAM ) himl );

  dialog_created = 1;
}

static INT_PTR CALLBACK
dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
/* FIXME: implement resizing the dialog */
  switch( uMsg ) {

    case WM_INITDIALOG:
      dialog_init( hwndDlg );      
      return FALSE;

    case WM_NOTIFY:
      if( ( ( LPNMHDR ) lParam )->code == NM_DBLCLK ) {
        if( ( ( LPNMHDR ) lParam )->idFrom == IDC_BROWSE_LV ) {
          select_row( ( LPNMITEMACTIVATE ) lParam );
          return 0;
        }
      }
      break;

    case WM_COMMAND:
      if( LOWORD( wParam ) == IDCLOSE ) {
        ShowWindow( dialog, SW_HIDE );
        return 0;
      }
      break;

    case WM_CLOSE:
      /* Catch attempts to delete the window and just hide it instead */
      ShowWindow( dialog, SW_HIDE );
      return 0;      
  }
  return FALSE;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change GCC_UNUSED,
                        libspectrum_tape_block *block GCC_UNUSED )
{
  int error, current_block;

  if( !dialog_created ) return 0;

  fuse_emulation_pause();

  SendDlgItemMessage( dialog, IDC_BROWSE_LV, LVM_DELETEALLITEMS, 0, 0 );

  error = tape_foreach( add_block_details, NULL );
  if( error ) {
    fuse_emulation_unpause();
    return 1;
  }

  current_block = tape_get_current_block();
  if( current_block != -1 ) {
    LVITEM li;
    li.mask = LVIF_IMAGE;
    li.iItem = current_block;
    li.iSubItem = 0;
    li.iImage = 0;
    SendDlgItemMessage( dialog, IDC_BROWSE_LV, LVM_SETITEM, 0, ( LPARAM ) &li );
  }

  if( tape_modified ) {
    SendDlgItemMessage( dialog, IDC_BROWSE_MODIFIED, WM_SETTEXT,
                        0, ( LPARAM ) TEXT( "Tape modified" ) );
  } else {
    SendDlgItemMessage( dialog, IDC_BROWSE_MODIFIED, WM_SETTEXT,
                        0, ( LPARAM ) TEXT( "Tape not modified" ) );
 }

  fuse_emulation_unpause();

  return 0;
}

static void
add_block_details( libspectrum_tape_block *block, void *user_data )
{
  TCHAR buffer[256];
  TCHAR *details[3] = { &buffer[0], &buffer[80], &buffer[160] };
  LV_ITEM lvi;
  size_t i;

  _tcscpy( details[0], "" );
  libspectrum_tape_block_description( details[1], 80, block );
  /* FIXME: why does it give such a big number of bytes? */
  tape_block_details( details[2], 80, block );

  lvi.mask = LVIF_TEXT | LVIF_IMAGE;
  lvi.iImage = -1;
  lvi.iItem = SendDlgItemMessage( dialog, IDC_BROWSE_LV,
                                  LVM_GETITEMCOUNT, 0, 0 );
  for( i = 0; i < 3; i++ ) {
    lvi.iSubItem = i;
    lvi.pszText = details[i];
    if( i == 0 )
      SendDlgItemMessage( dialog, IDC_BROWSE_LV, LVM_INSERTITEM, 0,
                          ( LPARAM ) &lvi );
    else
      SendDlgItemMessage( dialog, IDC_BROWSE_LV, LVM_SETITEM, 0,
                          ( LPARAM ) &lvi );
  }
}

/* Called when a row is selected */
static void
select_row( LPNMITEMACTIVATE lpnmitem )
{
  int current_block, row;
  
  row = lpnmitem->iItem;

  /* Don't do anything if the current block was clicked on */
  current_block = tape_get_current_block();
  if( row == current_block ) return;

  /* Otherwise, select the new block */
  tape_select_block_no_update( row );

  /* set the marker at the current item, clear others */
  size_t i, count;
  count = SendDlgItemMessage( dialog, IDC_BROWSE_LV,
                              LVM_GETITEMCOUNT, 0, 0 );
  LVITEM li;
  li.mask = LVIF_IMAGE;
  li.iSubItem = 0;
  
  for( i=0; i<count; i++ ) {
    li.iImage = ( i==row ? 0 : -1 );
    li.iItem = i;
    SendDlgItemMessage( dialog, IDC_BROWSE_LV, LVM_SETITEM, 0, ( LPARAM ) &li );
  }
}
