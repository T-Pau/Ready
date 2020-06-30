/* about.c: about dialog box
   Copyright (c) 2011 Philip Kendall

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
#include "hyperlinks.h"
#include "win32internals.h"

#include "about.h"


static INT_PTR CALLBACK dialog_proc( HWND hwndDlg, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam );

static HFONT hBoldFont;

void
menu_help_about( int action )
{
  /* Firstly, stop emulation */
  fuse_emulation_pause();

  if( !IsWindow( fuse_hABOWnd ) ) {
    fuse_hABOWnd = CreateDialog( fuse_hInstance,
	                             MAKEINTRESOURCE( IDD_ABOUT ),
                                 fuse_hWnd, dialog_proc );
    if( fuse_hABOWnd == NULL ) { fuse_emulation_unpause(); return; }
  }

  ShowWindow( fuse_hABOWnd, SW_SHOW );

  /* Carry on with emulation */
  fuse_emulation_unpause();
}

static void
dialog_init( HWND hwndDlg )
{
  HDC hdc;
  long lfHeight;

  hdc = GetDC( NULL );
  lfHeight = -MulDiv( 16, GetDeviceCaps( hdc, LOGPIXELSY ), 72 );
  ReleaseDC( NULL, hdc );

  hBoldFont = CreateFont( lfHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0,
                          0, 0, 0, "Arial" );

  if( hBoldFont ) {
	win32ui_set_font( hwndDlg, IDC_ABOUT_STATIC_VERSION, hBoldFont );
  }

  ConvertCtlStaticToHyperlink( hwndDlg, IDC_ABOUT_STATIC_WEBSITE );
}

static INT_PTR CALLBACK
dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg ) {
    case WM_INITDIALOG:
      dialog_init( hwndDlg );
      return TRUE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDCLOSE:
        case IDCANCEL:
          DestroyWindow( hwndDlg );
		  fuse_hABOWnd = NULL;
          if( hBoldFont ) DeleteObject( hBoldFont );
          return TRUE;
        case IDC_ABOUT_STATIC_WEBSITE:
          ShellExecute( hwndDlg, "open", PACKAGE_URL,
                 NULL, NULL, SW_SHOWNORMAL );
          return TRUE;
      }
      break;

    case WM_CLOSE:
      DestroyWindow( hwndDlg );
      fuse_hABOWnd = NULL;
      return TRUE;
  }

  return FALSE;
}
