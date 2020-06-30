/* binary.c: Win32 routines to load/save chunks of binary data
   Copyright (c) 2003-2008 Philip Kendall, Marek Januszewski

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
#include "ui/ui.h"
#include "utils.h"
#include "win32internals.h"

#include "binary.h"

struct binary_info {

  TCHAR *filename;
  utils_file file;

  TCHAR *dialog_title;
  
  void (*on_change_filename)( HWND hwndDlg, LONG_PTR user_data );
  void (*on_execute)( HWND hwndDlg, LONG_PTR user_data );
};

static void change_load_filename( HWND hwndDlg, LONG_PTR user_data );
static void load_data( HWND hwndDlg, LONG_PTR user_data );

static void change_save_filename( HWND hwndDlg, LONG_PTR user_data );
static void save_data( HWND hwndDlg, LONG_PTR user_data );

static INT_PTR CALLBACK
binarydata_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

void
menu_file_loadbinarydata( int action )
{
  /* FIXME: a way to associate a long type with a window is via SetWindowLong
            with GWL_USERDATA parameter - review past code and implement */
  
  struct binary_info info;

  int error;

  fuse_emulation_pause();

  info.dialog_title = (TCHAR *) TEXT( "Fuse - Load Binary Data" );
  
  info.filename = ui_get_open_filename( info.dialog_title );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  error = utils_read_file( info.filename, &info.file );
  if( error ) { free( info.filename ); fuse_emulation_unpause(); return; }

  info.on_change_filename = &change_load_filename;
  info.on_execute = &load_data;

  /* Information display */
  DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_BINARY ), fuse_hWnd,
                  binarydata_proc, ( LPARAM ) &info );

  free( info.filename );
  utils_close_file( &info.file );

  fuse_emulation_unpause();
}

static INT_PTR CALLBACK
binarydata_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  struct binary_info *info = NULL;  
  
  switch( uMsg ) {
    case WM_INITDIALOG: {
      TCHAR buffer[80];
      info = ( struct binary_info * ) lParam;

      /* save the pointer to info with this dialog window */
      SetWindowLongPtr( hwndDlg, GWLP_USERDATA, ( LONG_PTR ) info );
                          
      SendMessage( hwndDlg, WM_SETTEXT, 0, ( LPARAM ) info->dialog_title );

      SendDlgItemMessage( hwndDlg, IDC_BINARY_STATIC_PATH, WM_SETTEXT,
                          0, ( LPARAM ) info->filename );

      if( info->file.length != -1 ) {
        _sntprintf( buffer, 80, "%lu", (unsigned long) info->file.length );
        SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH, WM_SETTEXT,
                            0, ( LPARAM ) buffer );
      }
      return FALSE;
    }
    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDC_BINARY_BUTTON_BROWSE:
          info = ( struct binary_info * ) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
          info->on_change_filename( hwndDlg, ( LONG_PTR ) info ) ;
          return 0;

        case IDOK:
          info = ( struct binary_info * ) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
          info->on_execute( hwndDlg, ( LONG_PTR ) info ) ;
          return 0;

        case IDCANCEL:
          EndDialog( hwndDlg, 0 );
          return 0;
      }
      break;

    case WM_CLOSE:
      EndDialog( hwndDlg, 0 );
      return 0;
  }
  return FALSE;
}

static void
change_load_filename( HWND hwndDlg, LONG_PTR user_data )
{
  struct binary_info *info = ( struct binary_info * ) user_data;
  
  TCHAR *new_filename;
  utils_file new_file;

  TCHAR buffer[80];
  int error;

  new_filename = ui_get_open_filename( "Fuse - Load Binary Data" );
  if( !new_filename ) return;

  error = utils_read_file( new_filename, &new_file );
  if( error ) { free( new_filename ); return; }

  /* Remove the data for the old file */
  utils_close_file( &info->file );

  free( info->filename );

  /* Put the new data in */
  info->filename = new_filename; info->file = new_file;

  /* And update the displayed information */
  SendDlgItemMessage( hwndDlg, IDC_BINARY_STATIC_PATH, WM_SETTEXT,
                      0, ( LPARAM ) new_filename );

  _sntprintf( buffer, 80, "%lu", (unsigned long) info->file.length );
  SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH, WM_SETTEXT,
                      0, ( LPARAM ) buffer );
}

static void
load_data( HWND hwndDlg, LONG_PTR user_data )
{
  struct binary_info *info = ( struct binary_info * )user_data;
  HWND hwnd_control;

  long start, length; size_t i;

  TCHAR *temp_buffer, *endptr;
  size_t temp_buffer_len;
  int base;

  errno = 0;
  temp_buffer_len = SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH,
                                        WM_GETTEXTLENGTH, 0, 0 );
  temp_buffer = malloc( sizeof( TCHAR ) * ( temp_buffer_len + 1 ) );
  SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH, WM_GETTEXT,
                      temp_buffer_len + 1, ( LPARAM ) temp_buffer );

  errno = 0;
  base = ( !_tcsncmp( _T("0x"), temp_buffer, strlen( _T("0x") ) ) )? 16 : 10;
  length = _tcstol( temp_buffer, &endptr, base );
  if( errno || length < 1 || length > 0x10000 || endptr == temp_buffer ) {
    free( temp_buffer );
    ui_error( UI_ERROR_ERROR, "Length must be between 1 and 65536" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_LENGTH );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }
  free( temp_buffer );

  if( length > info->file.length ) {
    ui_error( UI_ERROR_ERROR,
	      "'%s' contains only %lu bytes",
	      info->filename, (unsigned long)info->file.length );
    return;
  }

  errno = 0;
  temp_buffer_len = SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_START,
                                        WM_GETTEXTLENGTH, 0, 0 );
  temp_buffer = malloc( sizeof( TCHAR ) * ( temp_buffer_len + 1 ) );
  SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_START, WM_GETTEXT,
                      temp_buffer_len + 1, ( LPARAM ) temp_buffer );

  errno = 0;
  base = ( !_tcsncmp( _T("0x"), temp_buffer, strlen( _T("0x") ) ) )? 16 : 10;
  start = _tcstol( temp_buffer, &endptr, base );
  if( errno || start < 0 || start > 0xffff || endptr == temp_buffer ) {
    free( temp_buffer );
    ui_error( UI_ERROR_ERROR, "Start must be between 0 and 65535" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_START );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }
  free( temp_buffer );

  if( start + length > 0x10000 ) {
    ui_error( UI_ERROR_ERROR, "Block ends after address 65535" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_LENGTH );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }

  for( i = 0; i < length; i++ )
    writebyte_internal( start + i, info->file.buffer[ i ] );

  EndDialog( hwndDlg, 0 );
}
  
void
menu_file_savebinarydata( int action )
{
  struct binary_info info;

  fuse_emulation_pause();

  info.dialog_title = (TCHAR *) TEXT( "Fuse - Save Binary Data" );

  info.filename = ui_get_save_filename( info.dialog_title );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  info.file.length = -1; /* let the dialog know to leave length box blank */
  info.on_change_filename = &change_save_filename;
  info.on_execute = &save_data;

  /* Information display */
  DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_BINARY ), fuse_hWnd,
                  binarydata_proc, ( LPARAM ) &info );

  free( info.filename );

  fuse_emulation_unpause();
}

static void
change_save_filename( HWND hwndDlg, LONG_PTR user_data )
{
  struct binary_info *info = ( struct binary_info * ) user_data;
  TCHAR *new_filename;

  new_filename = ui_get_save_filename( "Fuse - Save Binary Data" );
  if( !new_filename ) return;

  free( info->filename );

  info->filename = new_filename;

  SendDlgItemMessage( hwndDlg, IDC_BINARY_STATIC_PATH, WM_SETTEXT,
                      0, ( LPARAM ) new_filename );
}

static void
save_data( HWND hwndDlg, LONG_PTR user_data )
{
  struct binary_info *info = ( struct binary_info * ) user_data;
  long start, length;
  HWND hwnd_control;

  TCHAR *temp_buffer, *endptr;
  size_t temp_buffer_len;
  int base;

  int error;

  errno = 0;
  temp_buffer_len = SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH,
                                        WM_GETTEXTLENGTH, 0, 0 );
  temp_buffer = malloc( sizeof( TCHAR ) * ( temp_buffer_len + 1 ) );
  SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_LENGTH, WM_GETTEXT,
                      temp_buffer_len + 1, ( LPARAM ) temp_buffer );

  errno = 0;
  base = ( !_tcsncmp( _T("0x"), temp_buffer, strlen( _T("0x") ) ) )? 16 : 10;
  length = _tcstol( temp_buffer, &endptr, base );
  if( errno || length < 1 || length > 0x10000 || endptr == temp_buffer ) {
    free( temp_buffer );
    ui_error( UI_ERROR_ERROR, "Length must be between 1 and 65536" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_LENGTH );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }
  free( temp_buffer );

  errno = 0;
  temp_buffer_len = SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_START,
                                        WM_GETTEXTLENGTH, 0, 0 );
  temp_buffer = malloc( sizeof( TCHAR ) * ( temp_buffer_len + 1 ) );
  SendDlgItemMessage( hwndDlg, IDC_BINARY_EDIT_START, WM_GETTEXT,
                      temp_buffer_len + 1, ( LPARAM ) temp_buffer );

  errno = 0;
  base = ( !_tcsncmp( _T("0x"), temp_buffer, strlen( _T("0x") ) ) )? 16 : 10;
  start = _tcstol( temp_buffer, &endptr, base );
  if( errno || start < 0 || start > 0xffff || endptr == temp_buffer ) {
    free( temp_buffer );
    ui_error( UI_ERROR_ERROR, "Start must be between 0 and 65535" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_START );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }
  free( temp_buffer );

  if( start + length > 0x10000 ) {
    ui_error( UI_ERROR_ERROR, "Block ends after address 65535" );
    hwnd_control = GetDlgItem( hwndDlg, IDC_BINARY_EDIT_LENGTH );
    SendMessage( hwndDlg, WM_NEXTDLGCTL, (WPARAM) hwnd_control, TRUE );
    return;
  }

  error = utils_save_binary( start, length, info->filename );
  if( error ) return;

  EndDialog( hwndDlg, 0 );
}
