/* win32internals.h: stuff internal to the Win32 UI
   Copyright (c) 2004 Marek Januszewski

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

#ifndef FUSE_WIN32INTERNALS_H
#define FUSE_WIN32INTERNALS_H

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

/* FIXME: this should be included as part of windows.h, but is not
          because WIN32_LEAN_AND_MEAN is defined along the way somewhere */
#include <mmsystem.h>

#include <libspectrum.h>

#define ID_STATUSBAR 900

/* Reduce listview flickering. Defined from WINVER >= 6.00 */
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

/* window handler */
HWND fuse_hWnd;

/* application instance */
HINSTANCE fuse_hInstance;

/* status bar handle */
HWND fuse_hStatusWindow;

/* pokefinder window handle */
HWND fuse_hPFWnd;

/* debugger window handle */
HWND fuse_hDBGWnd;

/* about window handle */
HWND fuse_hABOWnd;

/*
 * Display routines (win32display.c)
 */

/* The colour palette in use */
extern libspectrum_dword win32display_colours[16];

int win32display_init( void );
int win32display_end( void );
int win32display_scaled_height( void );
int win32display_scaled_width( void );


/* Below variables and functions are shared
   between win32display.c and win32ui.c */

void win32display_area(int x, int y, int width, int height);
int win32display_drawing_area_resize( int width, int height, int force_scaler );

void blit( void );

/*
 * Keyboard routines (win32keyboard.c)
 */

void win32keyboard_init( void );
void win32keyboard_end( void );
void win32keyboard_keypress( WPARAM wParam, LPARAM lParam );
void win32keyboard_keyrelease( WPARAM wParam, LPARAM lParam );

/*
 * Mouse routines (win32mouse.c)
 */

void win32mouse_position( LPARAM lParam );
void win32mouse_button( int button, int down );

/*
 * General user interface routines (win32ui.c)
 */

void win32ui_fuse_resize( int width, int height );

int win32ui_confirm( const char *string );

int win32ui_picture( const char *filename, int border );

int win32ui_get_monospaced_font( HFONT *font );
void win32ui_set_font( HWND hDlg, int nIDDlgItem, HFONT font );

int handle_menu( DWORD cmd, HWND okno );

void win32_verror( int is_error );

void win32ui_process_messages( int process_queue_once );

#define WM_USER_EXIT_PROCESS_MESSAGES WM_USER

/*
 * Statusbar routines (statusbar.c)
 */

void win32statusbar_create( HWND hWnd );
int win32statusbar_set_visibility( int visible );
void win32statusbar_redraw( HWND hWnd, LPARAM lParam );
void win32statusbar_resize( HWND hWnd, WPARAM wParam, LPARAM lParam );
void win32statusbar_update_machine( const char *name );

/*
 * Dialog box reset
 */

void win32ui_pokefinder_clear( void );

#endif                          /* #ifndef FUSE_WIN32INTERNALS_H */
