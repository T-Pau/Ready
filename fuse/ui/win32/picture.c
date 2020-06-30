/* picture.c: Win32 routines to draw the keyboard picture
   Copyright (c) 2002-2008 Philip Kendall, Marek Januszewski, Stuart Brady

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

#include "display.h"
#include "picture.h"
#include "ui/ui.h"
#include "utils.h"
#include "win32internals.h"

#include <windows.h>

/* An RGB image of the keyboard picture */
/* the memory will be allocated by Windows
   ( DISPLAY_SCREEN_HEIGHT * DISPLAY_ASPECT_WIDTH * 4 bytes ) */
static void *picture;
static const int picture_pitch = DISPLAY_ASPECT_WIDTH * 4;

static HWND hDialogPicture = NULL;
static utils_file screen;
static HBITMAP picture_BMP;

static void draw_screen( libspectrum_byte *screen, int border );

static LRESULT WINAPI picture_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

int
win32ui_picture( const char *filename, int border )
{
  if (!hDialogPicture) {
    hDialogPicture = CreateDialog( fuse_hInstance,
                                   MAKEINTRESOURCE( IDD_PICTURE ),
                                   fuse_hWnd, (DLGPROC)picture_wnd_proc);

    BITMAPINFO picture_BMI;

    /* create the picture buffer */

    memset( &picture_BMI, 0, sizeof( picture_BMI ) );
    picture_BMI.bmiHeader.biSize = sizeof( picture_BMI.bmiHeader );
    picture_BMI.bmiHeader.biWidth = (size_t)( DISPLAY_ASPECT_WIDTH );
    /* negative to avoid "shep-mode": */
    picture_BMI.bmiHeader.biHeight = -DISPLAY_SCREEN_HEIGHT;
    picture_BMI.bmiHeader.biPlanes = 1;
    picture_BMI.bmiHeader.biBitCount = 32;
    picture_BMI.bmiHeader.biCompression = BI_RGB;
    picture_BMI.bmiHeader.biSizeImage = 0;
    picture_BMI.bmiHeader.biXPelsPerMeter = 0;
    picture_BMI.bmiHeader.biYPelsPerMeter = 0;
    picture_BMI.bmiHeader.biClrUsed = 0;
    picture_BMI.bmiHeader.biClrImportant = 0;
    picture_BMI.bmiColors[0].rgbRed = 0;
    picture_BMI.bmiColors[0].rgbGreen = 0;
    picture_BMI.bmiColors[0].rgbBlue = 0;
    picture_BMI.bmiColors[0].rgbReserved = 0;

    HDC dc = GetDC( hDialogPicture );
    picture_BMP = CreateDIBSection( dc, &picture_BMI, DIB_RGB_COLORS, &picture,
                                    NULL, 0 );

    if( utils_read_screen( filename, &screen ) ) {
      return 1;
    }

    draw_screen( screen.buffer, border );

    utils_close_file( &screen );

    ReleaseDC( hDialogPicture, dc );

    ShowWindow( hDialogPicture, SW_SHOW );
  }

  return 0;
}

static LRESULT WINAPI
picture_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HBITMAP old_bmp;
      HDC dest_dc = BeginPaint( hWnd, &ps );
      HDC pic_dc = CreateCompatibleDC( dest_dc );

      old_bmp = SelectObject( pic_dc, picture_BMP );
      BitBlt( dest_dc, 0, 0, DISPLAY_ASPECT_WIDTH,
              DISPLAY_SCREEN_HEIGHT, pic_dc, 0, 0, SRCCOPY );

      EndPaint( hWnd, &ps );
      SelectObject( pic_dc, old_bmp );
      DeleteDC( pic_dc );
      return 0;
    }

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDCLOSE:
        {
          hDialogPicture = NULL;
          DestroyWindow( hWnd );
          return 0;
        }
      }
      break;

    case WM_CLOSE:
    {
      hDialogPicture = NULL;
      DestroyWindow( hWnd );
      return 0;
    }
  }
  return FALSE;
}

static void
draw_screen( libspectrum_byte *screen, int border )
{
  int i, x, y, ink, paper;
  libspectrum_byte attr, data;

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)( picture + y * picture_pitch + 4 * x ) =
        win32display_colours[border];
      *(libspectrum_dword*)(
          picture +
          ( y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) * picture_pitch +
          4 * x
        ) = win32display_colours[ border ];
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)
        (picture + ( y + DISPLAY_BORDER_HEIGHT) * picture_pitch + 4 * x) =
        win32display_colours[ border ];
      *(libspectrum_dword*)(
          picture +
          ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
          4 * ( x+DISPLAY_ASPECT_WIDTH-DISPLAY_BORDER_ASPECT_WIDTH )
        ) = win32display_colours[ border ];
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      attr = screen[ display_attr_start[y] + x ];

      ink = ( attr & 0x07 ) + ( ( attr & 0x40 ) >> 3 );
      paper = ( attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      for( i=0; i<8; i++ ) {
        libspectrum_dword pix =
          win32display_colours[ ( data & 0x80 ) ? ink : paper ];

        /* rearrange pixel components */
        pix = ( pix & 0x0000ff00 ) |
              ( ( pix & 0x000000ff ) << 16 ) |
              ( ( pix & 0x00ff0000 ) >> 16 );

        *(libspectrum_dword*)(
            picture +
            ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
            4 * ( 8 * x + DISPLAY_BORDER_ASPECT_WIDTH + i )
          ) = pix;
        data <<= 1;
      }
    }

  }
}
