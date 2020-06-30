/* statusbar.c: routines for updating the status bar
   Copyright (c) 2004-2008 Marek Januszewski

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

#include "settings.h"
#include "ui/ui.h"
#include "win32internals.h"

HBITMAP
  icon_tape_inactive, icon_tape_active,
  icon_mdr_inactive, icon_mdr_active,
  icon_disk_inactive, icon_disk_active,
  icon_pause_inactive, icon_pause_active,
  icon_mouse_inactive, icon_mouse_active;

ui_statusbar_item icons_order[5] = {
  UI_STATUSBAR_ITEM_MOUSE,
  UI_STATUSBAR_ITEM_PAUSED,
  UI_STATUSBAR_ITEM_DISK,
  UI_STATUSBAR_ITEM_MICRODRIVE,
  UI_STATUSBAR_ITEM_TAPE,
};

ui_statusbar_state icons_status[5] = {
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* disk */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* microdrive */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* mouse */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* paused */
  UI_STATUSBAR_STATE_NOT_AVAILABLE  /* tape */
};

int icons_part_width = 140; /* will be calculated dynamically later */
int icons_part_height = 27;
int icons_part_margin = 2;

void
win32statusbar_create( HWND hWnd )
{
  DWORD dwStyle;

  /* FIXME: destroy those icons later on using DeleteObject */
  
  icon_tape_inactive = LoadImage( fuse_hInstance, "win32bmp_tape_inactive", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_tape_active = LoadImage( fuse_hInstance, "win32bmp_tape_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_mdr_inactive = LoadImage( fuse_hInstance, "win32bmp_mdr_inactive", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_mdr_active = LoadImage( fuse_hInstance, "win32bmp_mdr_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_disk_inactive = LoadImage( fuse_hInstance, "win32bmp_disk_inactive", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_disk_active = LoadImage( fuse_hInstance, "win32bmp_disk_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_pause_inactive = LoadImage( fuse_hInstance, "win32bmp_pause_inactive", 
                                   IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_pause_active = LoadImage( fuse_hInstance, "win32bmp_pause_active", 
                                 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_mouse_inactive = LoadImage( fuse_hInstance, "win32bmp_mouse_inactive", 
                                   IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_mouse_active = LoadImage( fuse_hInstance, "win32bmp_mouse_active", 
                                 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  dwStyle = WS_CHILD | SBARS_SIZEGRIP;
  if( settings_current.statusbar ) dwStyle = dwStyle | WS_VISIBLE;

  fuse_hStatusWindow = CreateWindowEx( 0, STATUSCLASSNAME, NULL, dwStyle,
                                       0, 0, 0, 0, hWnd, (HMENU)ID_STATUSBAR,
                                       fuse_hInstance, NULL );

  /* minimum size of the status bar is the height of the highest icon */
  SendMessage( fuse_hStatusWindow, SB_SETMINHEIGHT, icons_part_height, 0 );

  /* force redraw to apply the minimum height */
  SendMessage( fuse_hStatusWindow, WM_SIZE, 0, 0 );
}

int
win32statusbar_set_visibility( int visible )
{
  int current_state;

  current_state = IsWindowVisible( fuse_hStatusWindow );
  if( current_state == visible ) return 0;

  if( visible ) {
    ShowWindow( fuse_hStatusWindow, SW_SHOW );
  } else {
    ShowWindow( fuse_hStatusWindow, SW_HIDE );
  }

  /* Update main window size */
  win32ui_fuse_resize( win32display_scaled_width(), win32display_scaled_height() );

  return 0;
}

void
win32statusbar_update_machine( const char *name )
{
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 0, (LPARAM) name );
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  /* Fix flickering on +2A, +3 and +3e machines because of high refresh rate */
  if( item == UI_STATUSBAR_ITEM_DISK && icons_status[ item ] == state ) return 0;

  icons_status[ item ] = state;

  SendMessage( fuse_hStatusWindow, SB_SETTEXT, 1 | SBT_OWNERDRAW, 0 );

  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  TCHAR buffer[8];

  _sntprintf( buffer, 8, "\t%3.0f%%", speed ); /* \t centers the text */
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 2,
               (LPARAM) buffer);

  return 0;
}

void
win32statusbar_redraw( HWND hWnd, LPARAM lParam )
{
  DRAWITEMSTRUCT* di;
  HDC src_dc, dest_dc, mask_dc;
  RECT rc_item;
  HBITMAP src_bmp, src_bmp_mask;
  HBITMAP old_bmp, old_bmp_mask;
  BITMAP bmp;
  size_t i;
  int new_icons_part_width;
  
  src_bmp = 0;

  di = ( DRAWITEMSTRUCT* ) lParam;
  dest_dc = di->hDC;
  rc_item = di->rcItem;

  new_icons_part_width = 0;

  for( i=0; i<5; i++ ) {
    
    switch( icons_order[ i ] ) {
      case UI_STATUSBAR_ITEM_DISK:
        switch( icons_status[ UI_STATUSBAR_ITEM_DISK ] ) {
          case UI_STATUSBAR_STATE_NOT_AVAILABLE:
            src_bmp = NULL; break;
          case UI_STATUSBAR_STATE_ACTIVE:
            src_bmp = icon_disk_active; break;
          default:
            src_bmp = icon_disk_inactive; break;
        }
        break;

      case UI_STATUSBAR_ITEM_MOUSE:
        src_bmp = ( icons_status[ UI_STATUSBAR_ITEM_MOUSE ] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_mouse_active : icon_mouse_inactive );
        break;

      case UI_STATUSBAR_ITEM_PAUSED:
        src_bmp = ( icons_status[ UI_STATUSBAR_ITEM_PAUSED ] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_pause_active : icon_pause_inactive );
        break;

      case UI_STATUSBAR_ITEM_MICRODRIVE:
        switch( icons_status[ UI_STATUSBAR_ITEM_MICRODRIVE ] ) {
          case UI_STATUSBAR_STATE_NOT_AVAILABLE:
            src_bmp = NULL; break;
          case UI_STATUSBAR_STATE_ACTIVE:
            src_bmp = icon_mdr_active; break;
          default:
            src_bmp = icon_mdr_inactive; break;
        }
        break;

      case UI_STATUSBAR_ITEM_TAPE:
        src_bmp = ( icons_status[ UI_STATUSBAR_ITEM_TAPE ] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_tape_active : icon_tape_inactive );
        break;
    }

    if( src_bmp != NULL ) {    
      new_icons_part_width += icons_part_margin;

      /* create a bitmap mask on the fly */
      GetObject( src_bmp, sizeof( bmp ), &bmp );
      src_bmp_mask = CreateBitmap( bmp.bmWidth, bmp.bmHeight, 1, 1, NULL );
      src_dc = CreateCompatibleDC( NULL );
      mask_dc = CreateCompatibleDC( NULL );
      old_bmp = SelectObject( src_dc, src_bmp );
      old_bmp_mask = SelectObject( mask_dc, src_bmp_mask );
      SetBkColor( src_dc, RGB( 0, 0, 0 ) );
      BitBlt( mask_dc, 0, 0, bmp.bmWidth, bmp.bmHeight, src_dc, 0, 0, SRCCOPY );
      BitBlt( src_dc, 0, 0, bmp.bmWidth, bmp.bmHeight, mask_dc, 0, 0, SRCINVERT );
      
      /* blit the transparent icon onto the status bar */
      SelectObject( mask_dc, src_bmp_mask );
      BitBlt( dest_dc, rc_item.left + new_icons_part_width,
              rc_item.top + ( icons_part_height - bmp.bmHeight )
              - ( 2 * icons_part_margin ), 
              bmp.bmWidth, bmp.bmHeight, mask_dc, 0, 0, SRCAND );

      SelectObject( src_dc, src_bmp );
      BitBlt( dest_dc, rc_item.left + new_icons_part_width,
              rc_item.top + ( icons_part_height - bmp.bmHeight )
              - ( 2 * icons_part_margin ), 
              bmp.bmWidth, bmp.bmHeight, src_dc, 0, 0, SRCPAINT );

      SelectObject( src_dc, old_bmp );
      SelectObject( mask_dc, old_bmp_mask );
      DeleteDC( src_dc );
      DeleteDC( mask_dc );
      DeleteObject( src_bmp_mask );
      
      new_icons_part_width += bmp.bmWidth;
    }
  }

  if( new_icons_part_width > 0 ) new_icons_part_width += icons_part_margin;
  /* FIXME: if the calculations are correction I shouldn't be adding this */
  new_icons_part_width += ( 2 * icons_part_margin );

  /* Resize icons part if needed */
  if( new_icons_part_width != icons_part_width ) {
	icons_part_width = new_icons_part_width;
    win32statusbar_resize( fuse_hWnd, 0, 0 );
  }
}

void
win32statusbar_resize( HWND hWnd, WPARAM wParam GCC_UNUSED, LPARAM lParam )
{
  const int speed_bar_width = 70;
  RECT rcClient;

  /* divide status bar */
  int parts[3];

  if( LOWORD( lParam ) > 0 ) {
    parts[0] = LOWORD( lParam );
  }
  else {
    GetClientRect( hWnd, &rcClient );
    parts[0] = rcClient.right - rcClient.left;
  }

  parts[0] = parts[0] - icons_part_width - speed_bar_width;
  parts[1] = parts[0] + icons_part_width;  
  parts[2] = parts[1] + speed_bar_width;
  SendMessage( fuse_hStatusWindow, SB_SETPARTS, 3, ( LPARAM ) &parts );
}
