/* roms.c: ROM selector dialog box
   Copyright (c) 2003-2008 Philip Kendall, Marek Januszewski
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

#include "fuse.h"
#include "roms.h"
#include "settings.h"
#include "ui/ui.h"
#include "win32internals.h"

static void add_rom( HWND hwndDlg, size_t start, size_t row,
		     int is_peripheral );
static void select_new_rom( HWND hedit );
static void roms_done( HWND hwndDlg, LONG_PTR lParam );
static INT_PTR CALLBACK roms_proc( HWND hwndDlg, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam );
static void roms_init( HWND hwndDlg, LPARAM lParam );

/* FIXME: use GWL_USERDATA whenever more suitable, and use
          MapDialogRect whenever drawing the interface at runtime (options
          dialog for example */

/* The edit boxes used to display the current ROMs */
static HWND rom[ SETTINGS_ROM_COUNT ];

struct callback_info {

  size_t start, n;
  int is_peripheral;
  TCHAR title[ 256 ];

};

int
menu_select_roms_with_title( const char *title, size_t start, size_t n,
			     int is_peripheral )
{
  struct callback_info info;

  /* Firstly, stop emulation */
  fuse_emulation_pause();

  _sntprintf( info.title, 256, "Fuse - Select ROMs - %s", title );
  info.start = start;
  info.n = n;
  info.is_peripheral = is_peripheral;

  DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_ROMS ), fuse_hWnd,
                  ( DLGPROC ) roms_proc, ( LPARAM ) &info );

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return 0;
}

static INT_PTR CALLBACK
roms_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  HWND hedit;
  
  switch( uMsg ) {

    case WM_INITDIALOG:
      roms_init( hwndDlg, lParam );
      /* save callback_info in userdata of this dialog */
      SetWindowLongPtr( hwndDlg, GWLP_USERDATA, ( LONG_PTR ) lParam );
      return FALSE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {

        case IDOK:
          roms_done( hwndDlg, GetWindowLongPtr( ( HWND ) hwndDlg, GWLP_USERDATA ) );
          EndDialog( hwndDlg, 0 );
          return 0;

        case IDCANCEL:
          EndDialog( hwndDlg, 0 );
          return 0;
          
        default:
          if( HIWORD( wParam ) == BN_CLICKED ) {
            hedit = ( HWND ) GetWindowLongPtr( ( HWND ) lParam, GWLP_USERDATA );
            if( hedit > 0 ) {
              select_new_rom( hedit );
              return 0;
            }
          }
          break;
      }
      break;

    case WM_CLOSE:
      EndDialog( hwndDlg, 0 );
      return 0;
  }
  return FALSE;
}

static void
roms_init( HWND hwndDlg, LPARAM lParam )
{
  size_t i;
  struct callback_info *info;

  info = ( struct callback_info * ) lParam;
  
  for( i = 0; i < info->n; i++ )
    add_rom( hwndDlg, info->start, i, info->is_peripheral );

  /* Move the OK and Cancel buttons */
  RECT rect;

  rect.left = 25; rect.top = ( info->n * 30 ) + 5;
  rect.right = 25 + 50; rect.bottom = ( info->n * 30 ) + 5 + 14;
  MapDialogRect( hwndDlg, &rect );
  MoveWindow( GetDlgItem( hwndDlg, IDOK ),
              rect.left, rect.top,
              rect.right - rect.left, rect.bottom - rect.top,
              FALSE );

  rect.left = 85; rect.top = ( info->n * 30 ) + 5;
  rect.right = 85 + 50; rect.bottom = ( info->n * 30 ) + 5 + 14;
  MapDialogRect( hwndDlg, &rect );
  MoveWindow( GetDlgItem( hwndDlg, IDCANCEL ),
              rect.left, rect.top,
              rect.right - rect.left, rect.bottom - rect.top,
              FALSE );
              
  /* resize the dialog as needed */
  RECT window_rect, client_rect;
  
  GetWindowRect( hwndDlg, &window_rect );
  GetClientRect( hwndDlg, &client_rect );

  rect.left = 0; rect.top = 0;
  rect.right = 163; rect.bottom = ( info->n * 30 ) + 24;
  MapDialogRect( hwndDlg, &rect );
  
  /* rect now contains the size of the client area in pixels,
     now add window's absolute position on the screen */
  rect.left += window_rect.left;
  rect.top += window_rect.top;
  
  /* now just add the difference between client area and window area */
  rect.right += ( window_rect.right - window_rect.left )
              - ( client_rect.right - client_rect.left );
  rect.bottom += ( window_rect.bottom - window_rect.top )
               - ( client_rect.bottom - client_rect.top );
  
  /* MoveWindow doesn't really take rect, instead it's X, Y, sizeX and sizeY */
  MoveWindow( hwndDlg, rect.left, rect.top, rect.right, rect.bottom, FALSE );
}

static void
add_rom( HWND hwndDlg, size_t start, size_t row, int is_peripheral )
{
  RECT rect;
  HFONT font;
  HWND hgroup, hedit, hbutton;
  TCHAR buffer[ 80 ], **setting;

  _sntprintf( buffer, 80, "ROM %lu", (unsigned long)row );

  font = ( HFONT ) SendMessage( hwndDlg, WM_GETFONT, 0, 0 );
  
  /* create a groupbox */
  rect.left = 0; rect.top = ( row * 30 );
  rect.right = 160; rect.bottom = ( row * 30 ) + 30;
  MapDialogRect( hwndDlg, &rect );
  hgroup = CreateWindowEx( 0, WC_BUTTON, buffer,
                           WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                           rect.left, rect.top,
                           rect.right - rect.left, rect.bottom - rect.top,
                           hwndDlg, 0, fuse_hInstance, 0 );
  SendMessage( hgroup, WM_SETFONT, ( WPARAM ) font, FALSE );

  /* create an edit */
  setting = settings_get_rom_setting( &settings_current, start + row,
				      is_peripheral );

  rect.left = 5; rect.top = ( row * 30 ) + 10;
  rect.right = 5 + 110; rect.bottom = ( row * 30 ) + 10 + 14;
  MapDialogRect( hwndDlg, &rect );
  hedit = CreateWindowEx( 0, WC_EDIT, *setting,
                          WS_VISIBLE | WS_CHILD | WS_TABSTOP
                          | WS_BORDER | ES_AUTOHSCROLL,
                          rect.left, rect.top,
                          rect.right - rect.left, rect.bottom - rect.top,
                          hwndDlg, 0, fuse_hInstance, 0 );
  SendMessage( hedit, WM_SETFONT, ( WPARAM ) font, FALSE );
  
  rom[ row ] = hedit;

  /* create a select... button */
  rect.left = 120; rect.top = ( row * 30 ) + 10;
  rect.right = 120 + 35; rect.bottom = ( row * 30 ) + 10 + 14;
  MapDialogRect( hwndDlg, &rect );
  hbutton = CreateWindowEx( 0, WC_BUTTON, TEXT( "Select..." ),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                            rect.left, rect.top,
                            rect.right - rect.left, rect.bottom - rect.top,
                            hwndDlg, 0, fuse_hInstance, 0 );
  SendMessage( hbutton, WM_SETFONT, ( WPARAM ) font, FALSE );
  
  /* associate handle to the edit box with each Select button as user data */
  SetWindowLongPtr( hbutton, GWLP_USERDATA, ( LONG_PTR ) hedit );
}

static void
select_new_rom( HWND hedit )
{
  TCHAR *filename;

  filename = ui_get_open_filename( "Fuse - Select ROM" );
  if( !filename ) return;

  SendMessage( hedit, WM_SETTEXT, 0, ( LPARAM ) filename );
}

static void
roms_done( HWND hwndDlg, LONG_PTR lParam )
{
  size_t i;
  
  TCHAR **setting; TCHAR *string;
  size_t string_len;
  
  struct callback_info *info = ( struct callback_info * ) lParam;

  for( i = 0; i < info->n; i++ ) {

    setting = settings_get_rom_setting( &settings_current, info->start + i,
					info->is_peripheral );

    string_len = SendMessage( rom[i], WM_GETTEXTLENGTH, 0, 0 );
    string = malloc( sizeof( TCHAR ) * ( string_len + 1 ) );
    SendMessage( rom[i], WM_GETTEXT, string_len + 1, ( LPARAM ) string );

    settings_set_string( setting, string );
    free( string );
  }
}
