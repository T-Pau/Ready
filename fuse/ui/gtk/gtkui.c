/* gtkui.c: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2015 Philip Kendall, Russell Marks, Sergio Baldoví

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

#include <math.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <glib.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkcompat.h"
#include "gtkinternals.h"
#include "keyboard.h"
#include "machine.h"
#include "machines/specplus3.h"
#include "menu.h"
#include "peripherals/ide/simpleide.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/joystick.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "utils.h"

/* The main Fuse window */
GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
GtkWidget *gtkui_drawing_area;

static GtkWidget *menu_bar;

/* The UIManager used to create the menu bar */
GtkUIManager *ui_manager_menu = NULL;

/* True if we were paused via the Machine/Pause menu item */
static int paused = 0;

/* Structure used by the radio button selection widgets (eg the
   graphics filter selectors and Machine/Select) */
typedef struct gtkui_select_info {

  GtkWidget *dialog;
  GtkWidget **buttons;

  /* Used by the graphics filter selectors */
  scaler_available_fn selector;
  scaler_type selected;

  /* Used by the joystick confirmation */
  ui_confirm_joystick_t joystick;

} gtkui_select_info;

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkActionEntry *menu_data,
				guint menu_data_size);

static gboolean gtkui_lose_focus( GtkWidget*, GdkEvent*, gpointer );
static gboolean gtkui_gain_focus( GtkWidget*, GdkEvent*, gpointer );

static gboolean gtkui_delete( GtkWidget *widget, GdkEvent *event,
			      gpointer data );

static void menu_options_filter_done( GtkWidget *widget, gpointer user_data );
static void menu_machine_select_done( GtkWidget *widget, gpointer user_data );

static const GtkTargetEntry drag_types[] =
{
    { (gchar *)"text/uri-list", GTK_TARGET_OTHER_APP, 0 }
};

static void gtkui_drag_data_received( GtkWidget *widget GCC_UNUSED,
                                      GdkDragContext *drag_context,
                                      gint x GCC_UNUSED, gint y GCC_UNUSED,
                                      GtkSelectionData *data,
                                      guint info GCC_UNUSED, guint timestamp )
{
  static char uri_prefix[] = "file://";
  char *filename, *selection_filename;
  const guchar *selection_data, *data_begin, *data_end, *p;
  gint selection_length;

  selection_length = gtk_selection_data_get_length( data );

  if ( data && selection_length > (gint) sizeof( uri_prefix ) ) {
    selection_data = gtk_selection_data_get_data( data );
    data_begin = selection_data + sizeof( uri_prefix ) - 1;
    data_end = selection_data + selection_length;
    p = data_begin; 
    do {
      if ( *p == '\r' || *p == '\n' ) {
        data_end = p;
        break;
      }
    } while ( p++ != data_end );

    selection_filename = g_strndup( (const gchar *)data_begin,
                                    data_end - data_begin );

    filename = g_uri_unescape_string( selection_filename, NULL );
    if ( filename ) {
      fuse_emulation_pause();
      utils_open_file( filename, settings_current.auto_load, NULL );
      free( filename );
      display_refresh_all();
      fuse_emulation_unpause();
    }

    g_free( selection_filename );
  }
  gtk_drag_finish( drag_context, FALSE, FALSE, timestamp );
}

int
ui_init( int *argc, char ***argv )
{
  GtkWidget *box;
  GtkAccelGroup *accel_group;
  GtkSettings *settings;

  gtk_init(argc,argv);

#if !GTK_CHECK_VERSION( 3, 0, 0 )
  gdk_rgb_init();
  gdk_rgb_set_install( TRUE );
  gtk_widget_set_default_colormap( gdk_rgb_get_cmap() );
  gtk_widget_set_default_visual( gdk_rgb_get_visual() );
#endif                /* #if !GTK_CHECK_VERSION( 3, 0, 0 ) */

  gtkui_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  settings = gtk_widget_get_settings( GTK_WIDGET( gtkui_window ) );
  g_object_set( settings, "gtk-menu-bar-accel", "F1", NULL );
  gtk_window_set_title( GTK_WINDOW(gtkui_window), "Fuse" );

  g_signal_connect(G_OBJECT(gtkui_window), "delete-event",
		   G_CALLBACK(gtkui_delete), NULL);
  g_signal_connect(G_OBJECT(gtkui_window), "key-press-event",
		   G_CALLBACK(gtkkeyboard_keypress), NULL);
  gtk_widget_add_events( gtkui_window, GDK_KEY_RELEASE_MASK );
  g_signal_connect(G_OBJECT(gtkui_window), "key-release-event",
		   G_CALLBACK(gtkkeyboard_keyrelease), NULL);

  /* If we lose the focus, disable all keys */
  g_signal_connect( G_OBJECT( gtkui_window ), "focus-out-event",
		    G_CALLBACK( gtkui_lose_focus ), NULL );
  g_signal_connect( G_OBJECT( gtkui_window ), "focus-in-event",
		    G_CALLBACK( gtkui_gain_focus ), NULL );

  gtk_drag_dest_set( GTK_WIDGET( gtkui_window ),
                     GTK_DEST_DEFAULT_ALL,
                     drag_types,
                     ARRAY_SIZE( drag_types ),
                     GDK_ACTION_COPY | GDK_ACTION_PRIVATE | GDK_ACTION_MOVE );
                     /* GDK_ACTION_PRIVATE alone DNW with ROX-Filer,
                        GDK_ACTION_MOVE allow DnD from KDE */

  g_signal_connect( G_OBJECT( gtkui_window ), "drag-data-received",
		    G_CALLBACK( gtkui_drag_data_received ), NULL );

  box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  gtk_container_add(GTK_CONTAINER(gtkui_window), box);

  if( gtkui_make_menu( &accel_group, &menu_bar, gtkui_menu_data,
                       gtkui_menu_data_size ) ) {
    fprintf(stderr,"%s: couldn't make menus %s:%d\n",
	    fuse_progname,__FILE__,__LINE__);
    return 1;
  }

  gtk_window_add_accel_group( GTK_WINDOW(gtkui_window), accel_group );
  gtk_box_pack_start( GTK_BOX(box), menu_bar, FALSE, FALSE, 0 );

  gtkui_drawing_area = gtk_drawing_area_new();
  if(!gtkui_drawing_area) {
    fprintf(stderr,"%s: couldn't create drawing area at %s:%d\n",
	    fuse_progname,__FILE__,__LINE__);
    return 1;
  }

  /* Set minimum size for drawing area */
  gtk_widget_set_size_request( gtkui_drawing_area, DISPLAY_ASPECT_WIDTH,
                               DISPLAY_SCREEN_HEIGHT );

  gtkmouse_init();

  gtk_box_pack_start( GTK_BOX(box), gtkui_drawing_area, TRUE, TRUE, 0 );

  /* Create the statusbar */
  gtkstatusbar_create( GTK_BOX( box ) );

  gtk_widget_show_all( gtkui_window );
  gtkstatusbar_set_visibility( settings_current.statusbar );

  ui_mouse_present = 1;

  return 0;
}

static void
gtkui_menu_deactivate( GtkMenuShell *menu GCC_UNUSED,
		       gpointer data GCC_UNUSED )
{
  ui_mouse_resume();
}

static gboolean
gtkui_make_menu(GtkAccelGroup **accel_group,
                GtkWidget **menu_bar,
                GtkActionEntry *menu_data,
                guint menu_data_size)
{
  *accel_group = NULL;
  *menu_bar = NULL;
  GError *error = NULL;
  char ui_file[ PATH_MAX ];

  ui_manager_menu = gtk_ui_manager_new();

  /* Load actions */
  GtkActionGroup *menu_action_group = gtk_action_group_new( "MenuActionGroup" );
  gtk_action_group_add_actions( menu_action_group, menu_data, menu_data_size,
                                NULL );
  gtk_ui_manager_insert_action_group( ui_manager_menu, menu_action_group, 0 );
  g_object_unref( menu_action_group );

  /* Load the UI */
  if( utils_find_file_path( "menu_data.ui", ui_file, UTILS_AUXILIARY_GTK ) ) {
    fprintf( stderr, "%s: Error getting path for menu_data.ui\n",
                     fuse_progname );
    return TRUE;
  }

  guint ui_menu_id = gtk_ui_manager_add_ui_from_file( ui_manager_menu, ui_file,
                                                      &error );
  if( error ) {
    g_error_free( error );
    return TRUE;
  }
  else if( !ui_menu_id ) return TRUE;

  *accel_group = gtk_ui_manager_get_accel_group( ui_manager_menu );

  *menu_bar = gtk_ui_manager_get_widget( ui_manager_menu, "/MainMenu" );
  g_signal_connect( G_OBJECT( *menu_bar ), "deactivate",
		    G_CALLBACK( gtkui_menu_deactivate ), NULL );

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
ui_event(void)
{
  while(gtk_events_pending())
    gtk_main_iteration();
  return 0;
}

int
ui_end(void)
{
  /* Don't display the window whilst doing all this! */
  gtk_widget_hide( gtkui_window );

  g_object_unref( ui_manager_menu );

  return 0;
}

/* Create a dialog box with the given error message */
int
ui_error_specific( ui_error_level severity, const char *message )
{
  GtkWidget *dialog, *label, *vbox, *content_area;
  const gchar *title;

  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 0;

  /* Set the appropriate title */
  switch( severity ) {
  case UI_ERROR_INFO:	 title = "Fuse - Info"; break;
  case UI_ERROR_WARNING: title = "Fuse - Warning"; break;
  case UI_ERROR_ERROR:	 title = "Fuse - Error"; break;
  default:		 title = "Fuse - (Unknown Error Level)"; break;
  }

  /* Create the dialog box */
  dialog = gtkstock_dialog_new( title, G_CALLBACK( gtk_widget_destroy ) );

  /* Add the OK button into the lower half */
  gtkstock_create_close( dialog, NULL, G_CALLBACK (gtk_widget_destroy),
			 FALSE );

  /* Create a label with that message */
  label = gtk_label_new( message );

  /* Make a new vbox for the top part for saner spacing */
  vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
  gtk_box_pack_start( GTK_BOX( content_area ), vbox, TRUE, TRUE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

  /* Put the label in it */
  gtk_container_add( GTK_CONTAINER( vbox ), label );

  gtk_widget_show_all( dialog );

  return 0;
}

/* The callbacks used by various routines */

static gboolean
gtkui_lose_focus( GtkWidget *widget GCC_UNUSED,
		  GdkEvent *event GCC_UNUSED, gpointer data GCC_UNUSED )
{
  keyboard_release_all();
  ui_mouse_suspend();
  return TRUE;
}

static gboolean
gtkui_gain_focus( GtkWidget *widget GCC_UNUSED,
		  GdkEvent *event GCC_UNUSED, gpointer data GCC_UNUSED )
{
  ui_mouse_resume();
  return TRUE;
}

/* Called by the main window on a "delete-event" */
static gboolean
gtkui_delete( GtkWidget *widget GCC_UNUSED, GdkEvent *event GCC_UNUSED,
              gpointer data GCC_UNUSED )
{
  menu_file_exit( NULL, NULL );
  return TRUE;
}

/* Called by the menu when File/Exit selected */
void
menu_file_exit( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  if( gtkui_confirm( "Exit Fuse?" ) ) {

    if( menu_check_media_changed() ) return;

    fuse_exiting = 1;

    /* Stop the paused state to allow us to exit (occurs from main
       emulation loop) */
    if( paused ) menu_machine_pause( NULL, NULL );

    /* Ensure we break out of the main Z80 loop, there could be active
       breakpoints before the next event */
    debugger_exit_emulator( NULL );
  }
}

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;
  GSList *button_group = NULL;

  int count;
  scaler_type scaler;

  /* Store the function which tells us which scalers are currently
     available */
  dialog.selector = selector;

  /* No scaler currently selected */
  dialog.selected = SCALER_NUM;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( SCALER_NUM * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return SCALER_NUM;
  }

  count = 0;

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Scaler", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !selector( scaler ) ) continue;

    dialog.buttons[ count ] =
      gtk_radio_button_new_with_label( button_group, scaler_name( scaler ) );
    button_group =
      gtk_radio_button_get_group( GTK_RADIO_BUTTON( dialog.buttons[ count ] ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[ count ] ),
				  current_scaler == scaler );

    gtk_container_add( GTK_CONTAINER( content_area ), dialog.buttons[ count ] );

    count++;
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( menu_options_filter_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog.dialog ),
                                   GTK_RESPONSE_OK );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  return dialog.selected;
}

/* Callback used by the filter selection dialog */
static void
menu_options_filter_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i, count;
  gtkui_select_info *ptr = (gtkui_select_info*)user_data;

  count = 0;

  for( i = 0; i < SCALER_NUM; i++ ) {

    if( !ptr->selector( i ) ) continue;

    if( gtk_toggle_button_get_active(
	  GTK_TOGGLE_BUTTON( ptr->buttons[ count ] )
	)
      ) {
      ptr->selected = i;
    }

    count++;
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

static gboolean
gtkui_run_main_loop( gpointer user_data GCC_UNUSED )
{
  gtk_main();
  return FALSE;
}

/* Machine/Pause */
void
menu_machine_pause( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  int error;

  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
			 UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    gtk_main_quit();
  } else {

    /* Stop recording any competition mode RZX file */
    if( rzx_recording && rzx_competition_mode ) {
      ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
      error = rzx_stop_recording(); if( error ) return;
    }

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );

    /* Create nested main loop outside this callback to allow unpause */
    g_idle_add( (GSourceFunc)gtkui_run_main_loop, NULL );
  }

}

/* Called by the menu when Machine/Reset selected */
void
menu_machine_reset( GtkAction *gtk_action GCC_UNUSED, guint action )
{
  int hard_reset = action;
  const char *message = "Reset?";

  if( hard_reset )
    message = "Hard reset?";

  if( !gtkui_confirm( message ) )
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
menu_machine_select( GtkAction *gtk_action GCC_UNUSED,
                     gpointer data GCC_UNUSED )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;

  int i;

  /* Some space to store the radio buttons in */
  dialog.buttons = malloc( machine_count * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  dialog.dialog = gtkstock_dialog_new( "Fuse - Select Machine", NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  dialog.buttons[0] =
    gtk_radio_button_new_with_label(
      NULL, libspectrum_machine_name( machine_types[0]->machine )
    );

  for( i=1; i<machine_count; i++ ) {
    dialog.buttons[i] =
      gtk_radio_button_new_with_label(
        gtk_radio_button_get_group( GTK_RADIO_BUTTON( dialog.buttons[i-1] ) ),
	libspectrum_machine_name( machine_types[i]->machine )
      );
  }

  for( i=0; i<machine_count; i++ ) {
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[i] ),
				  machine_current == machine_types[i] );
    gtk_container_add( GTK_CONTAINER( content_area ), dialog.buttons[i] );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( menu_machine_select_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog.dialog ),
                                   GTK_RESPONSE_OK );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();
}

/* Callback used by the machine selection dialog */
static void
menu_machine_select_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i=0; i<machine_count; i++ ) {
    if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ptr->buttons[i]) ) &&
        machine_current != machine_types[i]
      )
    {
      machine_select( machine_types[i]->machine );
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

void
menu_machine_debugger( GtkAction *gtk_action GCC_UNUSED,
                       gpointer data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  gtkui_pokefinder_clear();
  return 0;
}

void
menu_help_keyboard( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_picture( "keyboard.scr", 0 );
}

void
menu_help_about( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
{
  /* TODO: show Fuse icon */
  gtk_show_about_dialog( GTK_WINDOW( gtkui_window ),
                         "program-name", "Fuse",
                         "comments", "The Free Unix Spectrum Emulator",
                         "copyright", FUSE_COPYRIGHT,
                         "logo-icon-name", NULL,
                         "version", VERSION,
                         "website", PACKAGE_URL,
                         NULL );
}

/* Generic `tidy-up' callback */
void
gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data GCC_UNUSED )
{
  gtk_widget_destroy( widget );
  gtk_main_quit();
}

/* Functions to activate and deactivate certain menu items */

int
ui_menu_item_set_active( const char *path, int active )
{
  GtkWidget *menu_item;

  /* Translate UI-indepentment path to GTK UI path */
  gchar *full_path = g_strdup_printf ("/MainMenu%s", path );

  menu_item = gtk_ui_manager_get_widget( ui_manager_menu, full_path );
  g_free( full_path );

  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from menu_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  return 0;
}

static void
confirm_joystick_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = user_data;

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkToggleButton *button = GTK_TOGGLE_BUTTON( ptr->buttons[ i ] );

    if( gtk_toggle_button_get_active( button ) ) {
      ptr->joystick = i;
      break;
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type,
		     int inputs GCC_UNUSED )
{
  GtkWidget *content_area;
  gtkui_select_info dialog;
  char title[ 80 ];
  int i;
  GSList *group = NULL;

  if( !settings_current.joy_prompt ) return UI_CONFIRM_JOYSTICK_NONE;

  /* Some space to store the radio buttons in */
  dialog.buttons =
    malloc( JOYSTICK_CONN_COUNT * sizeof( *dialog.buttons ) );
  if( !dialog.buttons ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return UI_CONFIRM_JOYSTICK_NONE;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  snprintf( title, sizeof( title ), "Fuse - Configure %s Joystick",
	    libspectrum_joystick_name( libspectrum_type ) );
  dialog.dialog = gtkstock_dialog_new( title, NULL );
  content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog.dialog ) );

  for( i = 0; i < JOYSTICK_CONN_COUNT; i++ ) {

    GtkWidget **button = &( dialog.buttons[ i ] );

    *button =
      gtk_radio_button_new_with_label( group, joystick_connection[ i ] );
    group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( *button ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( *button ), i == 0 );
    gtk_container_add( GTK_CONTAINER( content_area ), *button );
  }

  /* Create and add the actions buttons to the dialog box */
  gtkstock_create_ok_cancel( dialog.dialog, NULL,
                             G_CALLBACK( confirm_joystick_done ),
                             (gpointer) &dialog, DEFAULT_DESTROY,
                             DEFAULT_DESTROY );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog.dialog ),
                                   GTK_RESPONSE_OK );

  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  dialog.joystick = UI_CONFIRM_JOYSTICK_NONE;
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return dialog.joystick;
}

/*
 * Font code
 */

int
gtkui_get_monospaced_font( PangoFontDescription **font )
{
  *font = pango_font_description_from_string( "Monospace 10" );
  if( !(*font) ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a monospaced font" );
    return 1;
  }

  return 0;
}

void
gtkui_free_font( PangoFontDescription *font )
{
  pango_font_description_free( font );
}

void
gtkui_list_set_cursor( GtkTreeView *list, int row )
{
  GtkTreePath *path;

  if( row >= 0 ) {
    path = gtk_tree_path_new_from_indices( row, -1 );
    gtk_tree_view_set_cursor( list, path, NULL, FALSE );
    gtk_tree_path_free( path );
  }
}

int
gtkui_list_get_cursor( GtkTreeView *list )
{
  GtkTreePath *path;
  GtkTreeViewColumn *focus_column;
  int *indices;
  int row = -1;

  /* Get selected row */
  gtk_tree_view_get_cursor( list, &path, &focus_column );
  if( path ) {
    indices = gtk_tree_path_get_indices( path );
    if( indices ) row = indices[0];
    gtk_tree_path_free( path );
  }

  return row;
}

static gboolean
key_press( GtkTreeView *list, GdkEventKey *event, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  gdouble base, oldbase, base_limit;
  gdouble page_size, step_increment;
  int cursor_row;
  int num_rows;

  base = oldbase = gtk_adjustment_get_value( adjustment );
  page_size = gtk_adjustment_get_page_size( adjustment );
  step_increment = gtk_adjustment_get_step_increment( adjustment );
  num_rows = ( page_size + 1 ) / step_increment;

  /* Get selected row */
  cursor_row = gtkui_list_get_cursor( list );

  switch( event->keyval )
  {
  case GDK_KEY_Up:
    if( cursor_row == 0 )
      base -= step_increment;
    break;

  case GDK_KEY_Down:
    if( cursor_row == num_rows - 1 )
      base += step_increment;
    break;

  case GDK_KEY_Page_Up:
    base -= gtk_adjustment_get_page_increment( adjustment );
    break;

  case GDK_KEY_Page_Down:
    base += gtk_adjustment_get_page_increment( adjustment );
    break;

  case GDK_KEY_Home:
    cursor_row = 0;
    base = gtk_adjustment_get_lower( adjustment );
    break;

  case GDK_KEY_End:
    cursor_row = num_rows - 1;
    base = gtk_adjustment_get_upper( adjustment ) - page_size;
    break;

  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) - page_size;
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) {
    gtk_adjustment_set_value( adjustment, base );

    /* Mark selected row */
    gtkui_list_set_cursor( list, cursor_row );
    return TRUE;
  }

  return FALSE;
}

static gboolean
wheel_scroll_event( GtkTreeView *list, GdkEvent *event, gpointer user_data )
{
  GtkAdjustment *adjustment = user_data;
  gdouble base, oldbase, base_limit;
  int cursor_row;

  base = oldbase = gtk_adjustment_get_value( adjustment );

  switch( event->scroll.direction )
  {
  case GDK_SCROLL_UP:
    base -= gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;
  case GDK_SCROLL_DOWN:
    base += gtk_adjustment_get_page_increment( adjustment ) / 2;
    break;

#if GTK_CHECK_VERSION( 3, 4, 0 )
  case GDK_SCROLL_SMOOTH:
    {
      static gdouble total_dy = 0;
      gdouble dx, dy, page_size;
      int delta;

      if( gdk_event_get_scroll_deltas( event, &dx, &dy ) ) {
        total_dy += dy;
        page_size = gtk_adjustment_get_page_size( adjustment );
        delta = total_dy * pow( page_size, 2.0 / 3.0 );

        /* Is movement significative? */
        if( delta ) {
          base += delta;
          total_dy = 0;
        }
      }
      break;
    }
#endif

  default:
    return FALSE;
  }

  if( base < 0 ) {
    base = 0;
  } else {
    base_limit = gtk_adjustment_get_upper( adjustment ) - 
                 gtk_adjustment_get_page_size( adjustment );
    if( base > base_limit ) base = base_limit;
  }

  if( base != oldbase ) {
    cursor_row = gtkui_list_get_cursor( list );
    gtk_adjustment_set_value( adjustment, base );
    gtkui_list_set_cursor( list, cursor_row );
  }

  return TRUE;
}

void
gtkui_scroll_connect( GtkTreeView *list, GtkAdjustment *adj )
{
  g_signal_connect( list, "key-press-event",
                    G_CALLBACK( key_press ), adj );
  g_signal_connect( list, "scroll-event",
                    G_CALLBACK( wheel_scroll_event ), adj );
}

int
gtkui_menubar_get_height( void )
{
  GtkAllocation alloc;

  gtk_widget_get_allocation( menu_bar, &alloc );

  return alloc.height;
}
