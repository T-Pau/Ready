/* win32ui.c: Win32 routines for dealing with the user interface
   Copyright (c) 2003-2015 Marek Januszewski, Philip Kendall, Stuart Brady
   Copyright (c) 2015 Kirben
   Copyright (c) 2016 lordhoto

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

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "menu.h"
#include "peripherals/joystick.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "select_template.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "ui/win32/menu_data.h"
#include "utils.h"
#include "win32internals.h"
#include "win32joystick.h"

/* fuse_hPrevInstance is needed only to register window class */
static HINSTANCE fuse_hPrevInstance;

/* specifies how the main window should be shown: minimized, maximized, etc */
static int fuse_nCmdShow;

/* handle to accelartors/keyboard shortcuts */
static HACCEL hAccels;

/* machine select dialog's font object */
static HFONT h_ms_font = NULL;

/* monospaced font to use with debugger and memory browser */
static HFONT monospaced_font = NULL;

/* True if we were paused via the Machine/Pause menu item */
static int paused = 0;

/* this helps pause fuse while the main window is minimized */
static int size_paused = 0;

/* this helps to finalize some blocking operations */
static int exit_process_messages = 0;

/* Structure used by the radio button selection widgets (eg the
   graphics filter selectors and Machine/Select) */
typedef struct win32ui_select_info {

  int length;
  int selected;
  const char **labels;
  LPCTSTR dialog_title;

} win32ui_select_info;

static BOOL win32ui_make_menu( void );

static int win32ui_lose_focus( HWND hWnd, WPARAM wParam, LPARAM lParam );
static int win32ui_gain_focus( HWND hWnd, WPARAM wParam, LPARAM lParam );

static int win32ui_window_paint( HWND hWnd, WPARAM wParam, LPARAM lParam );
static int win32ui_window_resize( HWND hWnd, WPARAM wParam, LPARAM lParam );
static BOOL win32ui_window_resizing( HWND hWnd, WPARAM wParam, LPARAM lParam );

static int
selector_dialog( win32ui_select_info *items );

static void
handle_drop( HDROP hDrop )
{
  size_t bufsize;
  char *namebuf;

  /* Check that only one file was dropped */
  if( DragQueryFile( hDrop, ~0UL, NULL, 0 ) == 1) {
    bufsize = DragQueryFile( hDrop, 0, NULL, 0 ) + 1;
    if( ( namebuf = malloc( bufsize ) ) ) {
      DragQueryFile( hDrop, 0, namebuf, bufsize );

      fuse_emulation_pause();

      utils_open_file( namebuf, tape_can_autoload(), NULL );

      free( namebuf );

      display_refresh_all();

      fuse_emulation_unpause();
    }
  }
  DragFinish( hDrop );
}

static LRESULT WINAPI
fuse_window_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {

    case WM_COMMAND:
      if( ! handle_menu( LOWORD( wParam ), hWnd ) )
        return 0;
      break;

    case WM_DROPFILES:
      handle_drop( ( HDROP )wParam );
      return 0;

    case WM_CLOSE:
      menu_file_exit( 0 );
      return 0;

    case WM_KEYDOWN:
      win32keyboard_keypress( wParam, lParam );
      return 0;

    case WM_KEYUP:
      win32keyboard_keyrelease( wParam, lParam );
      return 0;

    case WM_PAINT:
      if( ! win32ui_window_paint( hWnd, wParam, lParam ) )
        return 0;
      break;

    case WM_SIZING:
      if( win32ui_window_resizing( hWnd, wParam, lParam ) )
        return TRUE;
      break;

    case WM_SIZE:
      if( ! win32ui_window_resize( hWnd, wParam, lParam ) )
        return 0;
      break;

    case WM_DRAWITEM:
      if( wParam == ID_STATUSBAR ) {
        win32statusbar_redraw( hWnd, lParam );
        return TRUE;
      }
      break;

    case WM_DESTROY:
      fuse_exiting = 1;
      PostQuitMessage( 0 );

      /* Stop the paused state to allow us to exit (occurs from main
         emulation loop) */
      if( paused ) menu_machine_pause( 0 );
      return 0;

    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
    {
      fuse_emulation_pause();
      return 0;
    }

    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
    {
      fuse_emulation_unpause();
      return 0;
    }
    
    case WM_LBUTTONUP:
      win32mouse_button( 1, 0 );
      return 0;
      
    case WM_LBUTTONDOWN:
      win32mouse_button( 1, 1 );
      return 0;

    case WM_MBUTTONUP:
      win32mouse_button( 2, 0 );
      return 0;

    case WM_MBUTTONDOWN:
      win32mouse_button( 2, 1 );
      return 0;

    case WM_RBUTTONUP:
      win32mouse_button( 3, 0 );
      return 0;

    case WM_RBUTTONDOWN:
      win32mouse_button( 3, 1 );
      return 0;
      
    case WM_MOUSEMOVE:
      win32mouse_position( lParam );
      return 0;
      
    case WM_SETCURSOR:
    /* prevent the cursor from being redrawn if fuse has grabbed the mouse */
      if( ui_mouse_grabbed )
        return TRUE;
      else
        return( DefWindowProc( hWnd, msg, wParam, lParam ) );

    case WM_ACTIVATE:
      if( ( LOWORD( wParam ) == WA_ACTIVE ) ||
          ( LOWORD( wParam ) == WA_CLICKACTIVE ) )
        win32ui_gain_focus( hWnd, wParam, lParam );
      else if( LOWORD( wParam ) == WA_INACTIVE )
        win32ui_lose_focus( hWnd, wParam, lParam );
      /* We'll call DefWindowProc to get keyboard focus when debugger window
         is open and inactive */
      break;

    case WM_USER_EXIT_PROCESS_MESSAGES:
      /* Odd case when message loop is overridden by a modal dialog. This
         should not be caught here, so we delay this notification */
      exit_process_messages++;
      return 0;

    case WM_ERASEBKGND:
      /* Improves speed and avoid flickering when main window is invalidated by
         another window */
      return TRUE;

#if defined USE_JOYSTICK && !defined HAVE_JSW_H

    case MM_JOY1BUTTONDOWN:
      win32joystick_buttonevent( 0, 1, wParam );
      break;

    case MM_JOY1BUTTONUP:
      win32joystick_buttonevent( 0, 0, wParam );
      break;

    case MM_JOY2BUTTONDOWN:
      win32joystick_buttonevent( 1, 1, wParam );
      break;

    case MM_JOY2BUTTONUP:
      win32joystick_buttonevent( 1, 0, wParam );
      break;

    case MM_JOY1MOVE:
      win32joystick_move( 0, LOWORD( lParam ), HIWORD( lParam ) );
      break;

    case MM_JOY2MOVE:
      win32joystick_move( 1, LOWORD( lParam ), HIWORD( lParam ) );
      break;

#endif			/* if defined USE_JOYSTICK && !defined HAVE_JSW_H */

  }
  return( DefWindowProc( hWnd, msg, wParam, lParam ) );
}

/* this is where windows program begins */
int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
         int nCmdShow )
{
  /* remember those values for window creation in ui_init */
  fuse_hInstance = hInstance;
  fuse_nCmdShow = nCmdShow;
  fuse_hPrevInstance = hPrevInstance;

/* HACK: __argc, __argv are broken and return zero when using mingwrt 4.0+
   on MinGW.
   HACK: MinGW-w64 based toolchains neither feature _argc nor _argv. The 32 bit
   incarnation only defines __MINGW32__. This leads to build breakage due to
   missing declarations. Luckily MinGW-w64 based toolchains define
   __MINGW64_VERSION_foo macros inside _mingw.h, which is included from all
   system headers. Thus we abuse that to detect them.
*/
#if defined( __GNUC__ ) && defined( __MINGW32__ ) \
                        && !defined( __MINGW64_VERSION_MAJOR )
  return fuse_main( _argc, _argv );
#else
  return fuse_main( __argc, __argv );
#endif

  /* FIXME: how do deal with returning wParam */
}

int
ui_init( int *argc, char ***argv )
{
  /* register window class */
  WNDCLASS wc;

  if( !fuse_hPrevInstance ) {
    wc.lpszClassName = "Fuse";
    wc.lpfnWndProc = fuse_window_proc;
    wc.style = CS_OWNDC;
    wc.hInstance = fuse_hInstance;
    wc.hIcon = LoadIcon( fuse_hInstance, "win32_icon" );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
    wc.lpszMenuName = "win32_menu";
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    if( !RegisterClass( &wc ) )
      return 0;
  }

  /* create the window */
  fuse_hWnd = CreateWindow( "Fuse", "Fuse", WS_OVERLAPPED | WS_CAPTION |
    WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, fuse_hInstance, NULL );

  /* init windows controls such as the status bar */
  InitCommonControls();

  /* menu is created in rc file, but we still need to set initial state */
  win32ui_make_menu();

  /* load keyboard shortcuts */
  hAccels = LoadAccelerators( fuse_hInstance, "win32_accel" );

  /* status bar */
  win32statusbar_create( fuse_hWnd );

  /* set the initial size of the drawing area */
  RECT wr, cr, statr;
  int w_ofs, h_ofs;
  
  GetWindowRect( fuse_hWnd, &wr );
  GetClientRect( fuse_hWnd, &cr );
  GetClientRect( fuse_hStatusWindow, &statr );

  w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
  h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
  if( settings_current.statusbar ) h_ofs += ( statr.bottom - statr.top );

  MoveWindow( fuse_hWnd, wr.left, wr.top,
              DISPLAY_ASPECT_WIDTH + w_ofs,
              DISPLAY_SCREEN_HEIGHT + h_ofs,
              FALSE );

  /* init the display area */
  if( win32display_init() ) return 1;

  win32keyboard_init();

  /* show the window finally */
  ShowWindow( fuse_hWnd, fuse_nCmdShow );
  UpdateWindow( fuse_hWnd );

  /* window will accept dragging and dropping */
  DragAcceptFiles( fuse_hWnd, TRUE );

  ui_mouse_present = 1;

  return 0;
}

static BOOL
win32ui_make_menu( void )
{
  /* Start various menus in the 'off' state */
  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIE_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_MACHINE_PROFILER, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING, 0 );
  ui_menu_activate( UI_MENU_ITEM_RECORDING_ROLLBACK, 0 );
  ui_menu_activate( UI_MENU_ITEM_TAPE_RECORDING, 0 );
#ifdef HAVE_LIB_XML2
  ui_menu_activate( UI_MENU_ITEM_FILE_SVG_CAPTURE, 0 );
#endif
  return FALSE;
}

int
ui_event( void )
{
  win32ui_process_messages( 1 );

  return 0;
}

int
ui_end( void )
{
  int error;

  win32keyboard_end();

  error = win32display_end(); if( error ) return error;

  /* close the monospaced font handle */     
  if( monospaced_font ) {
    DeleteObject( monospaced_font );
    monospaced_font = NULL;
  }
        
  return 0;
}

/* Create a dialog box with the given error message */
int
ui_error_specific( ui_error_level severity, const char *message )
{
  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 0;

  switch( severity ) {

  case UI_ERROR_INFO:
    MessageBox( fuse_hWnd, message, "Fuse - Info", MB_ICONINFORMATION | MB_OK );
    break;
  case UI_ERROR_WARNING:
    MessageBox( fuse_hWnd, message, "Fuse - Warning", MB_ICONWARNING | MB_OK );
    break;
  case UI_ERROR_ERROR:
    MessageBox( fuse_hWnd, message, "Fuse - Error", MB_ICONERROR | MB_OK );
    break;
  default:
    MessageBox( fuse_hWnd, message, "Fuse - (Unknown Error Level)",
                MB_ICONINFORMATION | MB_OK );
    break;

  }

  return 0;
}

/* The callbacks used by various routines */

static int
win32ui_lose_focus( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  keyboard_release_all();
  ui_mouse_suspend();
  return 0;
}

static int
win32ui_gain_focus( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  ui_mouse_resume();
  return 0;
}

/* Called by the menu when File/Exit selected */
void
menu_file_exit( int action )
{
 /* FIXME: this should really be sending WM_CLOSE, not duplicate code */
  if( win32ui_confirm( "Exit Fuse?" ) ) {

    if( menu_check_media_changed() ) return;

    DestroyWindow(fuse_hWnd);
  }
}

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  scaler_type selected_scaler = SCALER_NUM;
  win32ui_select_info items;
  int count, i, selection;
  scaler_type scaler;

  /* Get count of currently applicable scalars first */
  count = 0; 
  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {
    if( selector( scaler ) ) count++;
  }

  /* Populate win32ui_select_info */
  items.dialog_title = TEXT( "Fuse - Select Scaler" );
  items.labels = malloc( count * sizeof( char * ) );
  items.length = count; 

  /* Populate the labels with currently applicable scalars */
  count = 0;
  
  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !selector( scaler ) ) continue;

    items.labels[ count ] = scaler_name( scaler );

    if( current_scaler == scaler ) {
      items.selected = count;
    }

    count++;
  }

  /* Start the selection dialog box */
  selection = selector_dialog( &items );
  
  if( selection >= 0 ) {
    /* Apply the selected scalar */
    count = 0;
    
    for( i = 0; i < SCALER_NUM; i++ ) {
      if( !selector( i ) ) continue;
      	
      if( selection == count ) {
      	selected_scaler = i;
      }
  
      count++;
    }
  }
	
  free( items.labels );

  return selected_scaler;
}

/* Machine/Pause */
void
menu_machine_pause( int action )
{
  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
                         UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    PostMessage( fuse_hWnd, WM_USER_EXIT_PROCESS_MESSAGES, 0, 0 );

    /* Resume emulation */
    fuse_emulation_unpause();
  } else {

    /* Stop emulation */
    fuse_emulation_pause();

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );
    win32ui_process_messages( 0 );
  }
}

/* Called by the menu when Machine/Reset selected */
void
menu_machine_reset( int action )
{
  int hard_reset = action;
  const char *message = "Reset?";

  if( hard_reset )
    message = "Hard reset?";

  if( !win32ui_confirm( message ) )
    return;

  /* Stop any ongoing RZX */
  rzx_stop_recording();
  rzx_stop_playback( 1 );

  if( machine_reset( hard_reset ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

    /* FIXME: abort() seems a bit extreme here, but it'll do for now */
    fuse_abort();
  }
}

/* Called by the menu when Machine/Select selected */
void
menu_machine_select( int action )
{
  /* FIXME: choosing spectrum SE crashes Fuse sound_frame () at sound.c:477 "ay_change[f].ofs = ( ay_change[f].tstates * sfreq ) / cpufreq;" */
  /* FIXME: choosing some Timexes crashes (win32) fuse as well */

  int selected_machine;
  win32ui_select_info items;
  int i;

  /* Stop emulation */
  fuse_emulation_pause();

  /* Populate win32ui_select_info */
  items.dialog_title = TEXT( "Fuse - Select Machine" );
  items.labels = malloc( machine_count * sizeof( char * ) );
  items.length = machine_count; 

  for( i=0; i<machine_count; i++ ) {

    items.labels[i] = libspectrum_machine_name( machine_types[i]->machine );

    if( machine_current == machine_types[i] ) {
      items.selected = i;
    }
  }

  /* start the machine select dialog box */
  selected_machine = selector_dialog( &items );

  if( selected_machine >= 0 &&
      machine_types[ selected_machine ] != machine_current ) {
    machine_select( machine_types[ selected_machine ]->machine );
  }

  free( items.labels );

  /* Resume emulation */
  fuse_emulation_unpause();
}

void
menu_machine_debugger( int action )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  win32ui_pokefinder_clear();
  return 0;
}

void
menu_help_keyboard( int action )
{
  win32ui_picture( "keyboard.scr", 0 );
}

/* Functions to activate and deactivate certain menu items */
static int
set_active( HMENU menu, const char *path, int active )
{
  int i, menu_count;
  char menu_text[255];
  MENUITEMINFO mii;
  
  if( *path == '/' ) path++;

  menu_count = GetMenuItemCount( menu );
  for( i = 0; i < menu_count; i++ ) {

    if( GetMenuString( menu, i, menu_text, 255, MF_BYPOSITION ) == 0 ) continue;

    const char *p = menu_text, *q = path;

    /* Compare the two strings, but skip hotkey-delimiter characters */
    /* Anything after \t is a shortcut key on Win32 */
    do {
      if( *p == '&' ) p++;
      if( ! *p || *p == '\t' || *p != *q ) break;
      p++; q++;
    } while( 1 );

    if( *p && *p != '\t' ) continue;		/* not matched */

    /* match, but with a submenu */
    if( *q == '/' ) return set_active( GetSubMenu( menu, i ), q, active );

    if( *q ) continue;		/* not matched */

    /* we have a match */
    mii.fState = active ? MFS_ENABLED : MFS_DISABLED;
    mii.fMask = MIIM_STATE;
    mii.cbSize = sizeof( MENUITEMINFO );
    SetMenuItemInfo( menu, i, TRUE, &mii );

    return 0;
  }

  return 1;
}

int
ui_menu_item_set_active( const char *path, int active )
{
  return set_active( GetMenu( fuse_hWnd ), path, active );
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  win32ui_select_info items;
  TCHAR title[ 80 ];
  int i, selection;
  int selected_joystick;

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  /* Some space to store the radio options in */
  items.labels = malloc( JOYSTICK_CONN_COUNT * sizeof( char * ) );
  if( !items.labels ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return UI_CONFIRM_JOYSTICK_NONE;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Populate win32ui_select_info */
  /* FIXME: libspectrum_joystick_name is not unicode compliant */
  _sntprintf( title, ARRAY_SIZE( title ), _T( "Fuse - Configure %s Joystick" ),
	    libspectrum_joystick_name( libspectrum_type ) );
  items.dialog_title = title;
  items.length = JOYSTICK_CONN_COUNT; 

  for( i=0; i<JOYSTICK_CONN_COUNT; i++ ) {
    items.labels[i] = joystick_connection[ i ];
  }

  items.selected = UI_CONFIRM_JOYSTICK_NONE;

  /* start the joystick select dialog box */
  selection = selector_dialog( &items );

  selected_joystick = ( selection >= 0 )? selection : UI_CONFIRM_JOYSTICK_NONE;

  free( items.labels );

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return selected_joystick;
}

/*
 * Font code
 */

int
win32ui_get_monospaced_font( HFONT *font )
{
  if( ! monospaced_font ) {
    /* Get font height in pixels for current DPI resolution */
    HDC hdc = GetDC( NULL );
    long font_height = -MulDiv( 8, GetDeviceCaps( hdc, LOGPIXELSY ), 72 );
    ReleaseDC( NULL, hdc );

    *font = CreateFont( font_height, 0, 0, 0, 400, FALSE, FALSE, FALSE, 0,
                        400, 2, 1, 1, TEXT( "Courier New" ) );
    if( *font == NULL ) {
      ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
      return 1;
    }
    monospaced_font = *font;
  }
  else {
    *font = monospaced_font;
  }

  return 0;
}

int
window_recommended_width( HWND hwndDlg, LPCTSTR title )
{
  HDC dc;
  SIZE sz;
  LONG_PTR window_style;
  NONCLIENTMETRICS ncm;
  HFONT hCaptionFont, hDefaultFont;
  RECT cr, wr;
  int width, buttons;

  /* Get window style */
  window_style = GetWindowLongPtr( hwndDlg, GWL_STYLE );
  if( !( window_style & WS_CAPTION ) ) return 0;

  /* Get caption bar font */
  dc = GetDC( hwndDlg );
  if( !dc ) return 0;
  ncm.cbSize = sizeof( NONCLIENTMETRICS );
  /* FIXME: iPaddedBorderWidth,
     http://msdn.microsoft.com/en-us/library/ms724506%28VS.85%29.aspx */
  SystemParametersInfo( SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0 );
  hCaptionFont = CreateFontIndirect( &ncm.lfCaptionFont );
  hDefaultFont = SelectObject( dc, hCaptionFont );

  /* Calculate title width (pixels) */
  SetMapMode( dc, MM_TEXT );
  GetTextExtentPoint32( dc, title, _tcslen( title ), &sz );
  width = sz.cx; /* Actually in pixels because of MM_TEXT map mode */
  SelectObject( dc, hDefaultFont );
  DeleteObject( hCaptionFont );
  ReleaseDC( hwndDlg, dc );

  /* Calculate buttons width (pixels) */
  buttons = 1; /* close  button */
  if( window_style & WS_MAXIMIZEBOX ) buttons++;
  if( window_style & WS_MINIMIZEBOX ) buttons++;
  width += ncm.iCaptionWidth * buttons;

  /* Window decorations width (pixels) */
  GetWindowRect( hwndDlg, &wr );
  GetClientRect( hwndDlg, &cr );
  width += ( wr.right - wr.left ) - ( cr.right - cr.left ); 

  /* Icon width (pixels) */
  window_style = GetWindowLongPtr( hwndDlg, GWL_EXSTYLE );
  if( !(window_style & WS_EX_DLGMODALFRAME) )
    width += GetSystemMetrics( SM_CXSMICON );

  /* Padding, space between text and buttons */
  width += 20;

  return width;
}

void
win32ui_set_font( HWND hDlg, int nIDDlgItem, HFONT font )
{
  SendDlgItemMessage( hDlg, nIDDlgItem , WM_SETFONT, (WPARAM) font, FALSE );
}  

static void
selector_dialog_build( HWND hwndDlg, win32ui_select_info *items )
{
  int i, left, caption_width, decor_width, decor_height;
  int window_width, window_height, client_width, client_height;
  DWORD dwStyle;
  RECT wr, cr, or;

  /* set the title of the window */
  SendMessage( hwndDlg, WM_SETTEXT, 0, (LPARAM) items->dialog_title );

  /* Get decorations (pixels) */
  GetWindowRect( hwndDlg, &wr );
  GetClientRect( hwndDlg, &cr );
  decor_width = ( wr.right - wr.left ) - ( cr.right - cr.left );
  decor_height = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );

  /* calculate window width (pixels) */
  or.left = or.top = or.bottom = 0;
  or.right = 100 + 14 + 5; /* 2 buttons, 2 margins, 1 separation (DLUs) */
  MapDialogRect( hwndDlg, &or );

  window_width = or.right + decor_width;
  caption_width = window_recommended_width( hwndDlg, items->dialog_title );
  if( caption_width > window_width ) window_width = caption_width;
  client_width = window_width - decor_width;

  /* create radio buttons */
  client_height = 7; /* Top margin (DLUs) */
  for( i=0; i< items->length; i++ ) {

    dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON;
    /* Need for WS_GROUP to allow using arrow up/down to cycle thru this group */
    if( i == 0 ) dwStyle = dwStyle | WS_GROUP;

    or.left = 7; /* Left margin (DLUs) */
    or.top = client_height; /* Top position (DLUs) */
    or.right = 160; /* Control width (DLUs) */
    or.bottom = 9; /* Control height (DLUs) */
    MapDialogRect( hwndDlg, &or );
    client_height += 9; /* Control height (DLUs) */

    CreateWindow( TEXT( "BUTTON" ), items->labels[i],
                  dwStyle,
                  or.left, or.top, or.right, or.bottom,
                  hwndDlg, (HMENU) (LONG_PTR) ( IDC_SELECT_OFFSET + i ), fuse_hInstance, 0 );
    SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), WM_SETFONT,
                        (WPARAM) h_ms_font, FALSE );

    /* check the radiobutton corresponding to current label */
    if( i == items->selected ) {
      SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), BM_SETCHECK,
                          BST_CHECKED, 0 );
    }

    client_height += 2; /* Separation between radio buttons (DLUs) */
  }
  client_height += 5; /* Space after radio buttons (actually 7 DLUs) */

  /* create OK and Cancel buttons */
  or.left = 5; /* Vertical space between buttons (DLUs) */
  or.top = client_height; /* Position Y (DLUs) */
  or.right = 50; /* Typical width of buttons (DLUs) */
  or.bottom = 14; /* Typical height of buttons (DLUs) */
  MapDialogRect( hwndDlg, &or );

  left = ( client_width - or.left - ( or.right * 2 ) ) / 2; /* centered */
  CreateWindow( TEXT( "BUTTON" ), TEXT( "&OK" ),
                WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP | BS_DEFPUSHBUTTON,
                left, or.top, or.right, or.bottom,
                hwndDlg, (HMENU) IDOK, fuse_hInstance, 0 );
  SendDlgItemMessage( hwndDlg, IDOK, WM_SETFONT,
                      (WPARAM) h_ms_font, FALSE );

  left += or.right + or.left;
  CreateWindow( TEXT( "BUTTON" ), TEXT( "&Cancel" ),
                WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                left, or.top, or.right, or.bottom,
                hwndDlg, (HMENU) IDCANCEL, fuse_hInstance, 0 );
  SendDlgItemMessage( hwndDlg, IDCANCEL, WM_SETFONT,
                      (WPARAM) h_ms_font, FALSE );

  client_height += 21; /* Button height (14) + bottom margin (7) (DLUs) */

  /* Calculate window heigth (pixels) */
  wr.left = wr.top = wr.right = 0;
  wr.bottom = client_height;
  MapDialogRect( hwndDlg, &wr );
  window_height = decor_height + wr.bottom;

  /* the following will only change the size of the window */
  SetWindowPos( hwndDlg, NULL, 0, 0, window_width, window_height,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

static INT_PTR CALLBACK
selector_dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  int i;
  HWND next_item = NULL;

  switch( uMsg )
  {
    case WM_INITDIALOG: 
      /* items are passed to WM_INITDIALOG as lParam */
      selector_dialog_build( hwndDlg, ( win32ui_select_info * ) lParam );
      return TRUE;

    case WM_SETFONT:
      h_ms_font = (HFONT) wParam;
      return TRUE; /* "This message does not return a value." */

    case WM_COMMAND:
      if ( HIWORD( wParam ) != BN_CLICKED ) break;

      /* service OK and Cancel buttons */
      switch( LOWORD( wParam ) )
      {
        case IDOK:
        {
          /* check which radiobutton is selected and return the selection */
          i = 0;
          while( ( next_item = GetNextDlgGroupItem( hwndDlg, next_item,
                                                    FALSE ) ) != NULL ) {
            if( SendDlgItemMessage( hwndDlg, ( IDC_SELECT_OFFSET + i ), 
                                    BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
              EndDialog( hwndDlg, i );
              return TRUE;
            }
            i++;
          }
          break; /* program should never reach here */
        }
        case IDCANCEL:
          EndDialog( hwndDlg, -1 );
          return TRUE;
      }
      /* service clicking radiobuttons */
      /* FIXME should also be checking if wParam < offset + radio count */
      if( LOWORD( wParam ) >= IDC_SELECT_OFFSET )
        return 0;
      break;

    case WM_DESTROY:
      EndDialog( hwndDlg, -1 );
      return TRUE;
  }
  return FALSE;
}

static int
selector_dialog( win32ui_select_info *items )
{
  /* selector_dialog will display a modal dialog, with a list of grouped
     radiobuttons, OK and Cancel buttons. The radiobuttons' labels, their count,
     current selection and dialog title are provided via win32ui_select_info.
     The function returns an int corresponding to the selected radiobutton */
  
  /* FIXME: fix accelerators for this window */

  return DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_SELECT_DIALOG ),
                         fuse_hWnd, selector_dialog_proc, ( LPARAM )items );
}

void
win32_verror( int is_error )
{
  if( !is_error ) return;

  DWORD last_error;
  static LPVOID err_msg;
  last_error = GetLastError();
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, last_error, LANG_USER_DEFAULT,
                 (LPTSTR) &err_msg, 0, NULL );
  MessageBox( fuse_hWnd, err_msg, "Error", MB_OK );
}

/* Handler for the main window's WM_PAINT notification.
   The handler is an equivalent of GTK's expose_event */
static int
win32ui_window_paint( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  blit();
  return 0;
}

/* Handler for the main window's WM_SIZE notification */
static int
win32ui_window_resize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  if( wParam == SIZE_MINIMIZED ) {
    if( !size_paused ) {
      size_paused = 1;
      fuse_emulation_pause();

      /* Process UI events until the window is restored */
      win32ui_process_messages( 0 );
    }
  } else {
    win32display_drawing_area_resize( LOWORD( lParam ), HIWORD( lParam ), 1 );

    /* Resize statusbar and inner parts */
    SendMessage( fuse_hStatusWindow, WM_SIZE, wParam, lParam );
    win32statusbar_resize( hWnd, wParam, lParam );

    if( size_paused ) {
      timer_estimate_reset();
      PostMessage( fuse_hWnd, WM_USER_EXIT_PROCESS_MESSAGES, 0, 0 );

      size_paused = 0;
      fuse_emulation_unpause();
    }
  }

  return 0;
}

/* Handler for the main window's WM_SIZING notification.
   The handler is an equivalent of setting window geometry in GTK */
static BOOL
win32ui_window_resizing( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
  RECT *selr, wr, cr, statr, or;
  int width, height, w_ofs, h_ofs, w_max, h_max;

  selr = (RECT *)lParam;
  GetWindowRect( fuse_hWnd, &wr );
  GetClientRect( fuse_hWnd, &cr );
  GetClientRect( fuse_hStatusWindow, &statr );

  w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
  h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
  if( settings_current.statusbar ) h_ofs += ( statr.bottom - statr.top );

  /* max scaler size in desktop workarea */
  SystemParametersInfo( SPI_GETWORKAREA, 0, &or, 0 );
  w_max = ( or.right - or.left ) - w_ofs + DISPLAY_ASPECT_WIDTH / 2;
  w_max /= DISPLAY_ASPECT_WIDTH;
  h_max = ( or.bottom - or.top ) - h_ofs + DISPLAY_SCREEN_HEIGHT / 2;
  h_max /= DISPLAY_SCREEN_HEIGHT;

  if( w_max < h_max ) {
    h_max = w_max;
  } else {
    w_max = h_max;
  }

  /* current scaler */
  width = selr->right - selr->left + DISPLAY_ASPECT_WIDTH / 2;
  height = selr->bottom - selr->top + DISPLAY_SCREEN_HEIGHT / 2;

  width -= w_ofs; height -= h_ofs;
  width /= DISPLAY_ASPECT_WIDTH; height /= DISPLAY_SCREEN_HEIGHT;

  if( wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT ) {
    height = width;
  } else if( wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM ) {
    width = height;
  }

  if( width < 1 || height < 1 ) {
    width = 1; height = 1;
  }

  if( width > w_max || height > h_max ) {
    width = w_max; height = h_max;
  }

  if( width > 3 || height > 3 ) {
    width = 3; height = 3;
  }

  if( width < height ) {
    height = width;
  } else {
    width = height;
  }

  width *= DISPLAY_ASPECT_WIDTH; height *= DISPLAY_SCREEN_HEIGHT;
  width += w_ofs; height += h_ofs;

  /* Set window size */
  if( wParam == WMSZ_TOP ||
      wParam == WMSZ_TOPLEFT ||
      wParam == WMSZ_TOPRIGHT ) {
    selr->top = selr->bottom - height;
  } else {
    selr->bottom = selr->top + height;
  }
  if( wParam == WMSZ_LEFT ||
      wParam == WMSZ_TOPLEFT ||
      wParam == WMSZ_BOTTOMLEFT ) {
    selr->left = selr->right - width;
  } else {
    selr->right = selr->left + width;
  }

  return TRUE;
}

/* The function is an equivalent of GTK's gtk_window_resize,
   take care of resizing main window into visible screen */
void
win32ui_fuse_resize( int width, int height )
{
  RECT wr, cr, statr, or;
  int w_ofs, h_ofs;

  /* Calculate decorations before resizing */
  GetWindowRect( fuse_hWnd, &wr );
  GetClientRect( fuse_hWnd, &cr );
  GetClientRect( fuse_hStatusWindow, &statr );

  w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
  h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
  if( settings_current.statusbar ) h_ofs += ( statr.bottom - statr.top );

  /* Set position inside workarea */
  SystemParametersInfo( SPI_GETWORKAREA, 0, &or, 0 );
  if( wr.left + width + w_ofs > or.right ) wr.left = or.right - width - w_ofs;
  if( wr.top + height + h_ofs > or.bottom ) wr.top = or.bottom - height - h_ofs;
  if( wr.left < or.left ) wr.left = or.left;
  if( wr.top < or.top ) wr.top = or.top;

  MoveWindow( fuse_hWnd, wr.left, wr.top,
              width + w_ofs,
              height + h_ofs,
              TRUE );
}

/* win32ui_process_messages() is an equivalent of gtk's gtk_main(),
 * it processes messages until it receives WM_USER_EXIT_PROCESS_MESSAGES
 * message, which should be sent using:
 * PostMessage( fuse_hWnd, WM_USER_EXIT_PROCESS_MESSAGES, 0, 0 );
 * ( equivalent of gtk_main_quit() );
 * With process_queue_once = 1 it checks for messages pending for fuse 
 *   window, processes them and exists.
 * With process_queue_once = 0 it processes the messages until it receives
 *   WM_USER_EXIT_PROCESS_MESSAGES message.
 */
void
win32ui_process_messages( int process_queue_once )
{
  MSG msg;
  int i, processMsg;
  HWND hModelessDlgs[] = { fuse_hPFWnd, fuse_hDBGWnd, fuse_hABOWnd };

  while( 1 ) {
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      /* FIXME: rethink this loop, IsDialogMessage in particular */
      processMsg = TRUE;

      for( i = 0; processMsg && i < ARRAY_SIZE( hModelessDlgs ); i++) {
        if( IsDialogMessage( hModelessDlgs[i], &msg ) ) processMsg = FALSE;
      }

      if( processMsg ) {
        if( !TranslateAccelerator( fuse_hWnd, hAccels, &msg ) ) {
          if( ( LOWORD( msg.message ) == WM_QUIT ) ||
              ( LOWORD( msg.message ) == WM_USER_EXIT_PROCESS_MESSAGES ) )
            return;
          /* FIXME: set exit flag somewhere */
          TranslateMessage( &msg );
          DispatchMessage( &msg );
        }
      }
    }
    if( process_queue_once ) return;

    /* If we miss WM_USER_EXIT_PROCESS_MESSAGES, the window procedure will
       kindly notify us */
    if( exit_process_messages ) {
      exit_process_messages--;
      return;
    }

    WaitMessage();
  }
  /* FIXME: somewhere there should be return msg.wParam */
}
