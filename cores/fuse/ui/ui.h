/* ui.h: General UI event handling routines
   Copyright (c) 2000-2015 Philip Kendall
   Copyright (c) 2016 BogDan Vatra

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

#ifndef FUSE_UI_H
#define FUSE_UI_H

#include <stdarg.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif

#include <libspectrum.h>

#include "compat.h"
#include "machines/specplus3.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "svg.h"
#include "ui/scaler/scaler.h"

/* The various severities of error level, increasing downwards */
typedef enum ui_error_level {

  UI_ERROR_INFO,		/* Informational message */
  UI_ERROR_WARNING,		/* Something is wrong, but it's not that
				   important */
  UI_ERROR_ERROR,		/* An actual error */

} ui_error_level;

int ui_init(int *argc, char ***argv);
int ui_event(void);
int ui_end(void);

/* Error handling routines */
int ui_error( ui_error_level severity, const char *format, ... )
     GCC_PRINTF( 2, 3 );
libspectrum_error ui_libspectrum_error( libspectrum_error error,
					const char *format, va_list ap )
     GCC_PRINTF( 2, 0 );
int ui_verror( ui_error_level severity, const char *format, va_list ap )
     GCC_PRINTF( 2, 0 );
int ui_error_specific( ui_error_level severity, const char *message );
void ui_error_frame( void );

/* Callbacks used by the debugger */
int ui_debugger_activate( void );
int ui_debugger_deactivate( int interruptable );
int ui_debugger_update( void );
int ui_debugger_disassemble( libspectrum_word address );
void ui_breakpoints_updated( void );

/* Reset anything in the UI which needs to be reset on machine selection */
int ui_widgets_reset( void );

/* Functions defined in ../ui.c */

/* Confirm whether we want to save some data before overwriting it */
typedef enum ui_confirm_save_t {

  UI_CONFIRM_SAVE_SAVE,		/* Save the data */
  UI_CONFIRM_SAVE_DONTSAVE,	/* Don't save the data */
  UI_CONFIRM_SAVE_CANCEL,	/* Cancel the action */

} ui_confirm_save_t;

ui_confirm_save_t ui_confirm_save( const char *format, ... )
     GCC_PRINTF( 1, 2 );
ui_confirm_save_t ui_confirm_save_specific( const char *message );

/* Confirm whether we want to change a joystick setting */
typedef enum ui_confirm_joystick_t {

  UI_CONFIRM_JOYSTICK_NONE,	        /* Don't map joystick */
  UI_CONFIRM_JOYSTICK_KEYBOARD,	        /* Map the joystick to the keyboard */
  UI_CONFIRM_JOYSTICK_JOYSTICK_1,	/* Map the joystick to joystick 1 */
  UI_CONFIRM_JOYSTICK_JOYSTICK_2,	/* Map the joystick to joystick 2 */

} ui_confirm_joystick_t;

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs );

/* Mouse handling */

extern int ui_mouse_present, ui_mouse_grabbed;
void ui_mouse_suspend( void );
void ui_mouse_resume( void );
void ui_mouse_button( int button, int down );
void ui_mouse_motion( int dx, int dy );
int ui_mouse_grab( int startup ); /* UI: grab, return 1 if done */
int ui_mouse_release( int suspend ); /* UI: ungrab, return 0 if done */

/* Write the current tape out */
int ui_tape_write( void );

int ui_mdr_write( int which, int saveas );

/* Get a rollback point from the given list */
int ui_get_rollback_point( GSList *points );

/* Routines to (de)activate certain menu items */

typedef enum ui_menu_item {

  UI_MENU_ITEM_INVALID = 0,
  UI_MENU_ITEM_FILE_SVG_CAPTURE,
  UI_MENU_ITEM_FILE_MOVIE_RECORDING,
  UI_MENU_ITEM_FILE_MOVIE_PAUSE,
  UI_MENU_ITEM_MACHINE_PROFILER,
  UI_MENU_ITEM_MACHINE_MULTIFACE,
  UI_MENU_ITEM_MACHINE_DIDAKTIK80_SNAP,
  UI_MENU_ITEM_MEDIA_CARTRIDGE,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT,
  UI_MENU_ITEM_MEDIA_IF1,
  UI_MENU_ITEM_MEDIA_IF1_M1_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M1_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M2_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M2_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M3_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M3_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M4_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M4_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M5_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M5_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M6_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M6_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M7_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M7_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_M8_EJECT,
  UI_MENU_ITEM_MEDIA_IF1_M8_WP_SET,
  UI_MENU_ITEM_MEDIA_IF1_RS232_UNPLUG_R,
  UI_MENU_ITEM_MEDIA_IF1_RS232_UNPLUG_T,
  UI_MENU_ITEM_MEDIA_IF1_SNET_UNPLUG,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT,
  UI_MENU_ITEM_MEDIA_DISK,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_B,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA,
  UI_MENU_ITEM_MEDIA_DISK_BETA_A,
  UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_B,
  UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_C,
  UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_D,
  UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_1,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_2,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_OPUS,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_1,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_1_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_1_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_1_WP_SET,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_2,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_2_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_2_FLIP_SET,
  UI_MENU_ITEM_MEDIA_DISK_OPUS_2_WP_SET,
  UI_MENU_ITEM_MEDIA_IDE,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXCF,
  UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_DIVIDE,
  UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_DIVMMC,
  UI_MENU_ITEM_MEDIA_IDE_DIVMMC_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXMMC,
  UI_MENU_ITEM_MEDIA_IDE_ZXMMC_EJECT,
  UI_MENU_ITEM_RECORDING,
  UI_MENU_ITEM_RECORDING_ROLLBACK,
  UI_MENU_ITEM_AY_LOGGING,
  UI_MENU_ITEM_TAPE_RECORDING,

} ui_menu_item;

int ui_menu_activate( ui_menu_item item, int active );
int ui_menu_item_set_active( const char *path, int active );

void ui_menu_disk_update( void );

/* Functions to update the statusbar */

typedef enum ui_statusbar_item {

  UI_STATUSBAR_ITEM_DISK,
  UI_STATUSBAR_ITEM_MICRODRIVE,
  UI_STATUSBAR_ITEM_MOUSE,
  UI_STATUSBAR_ITEM_PAUSED,
  UI_STATUSBAR_ITEM_TAPE,

} ui_statusbar_item;

typedef enum ui_statusbar_state {

  UI_STATUSBAR_STATE_NOT_AVAILABLE,
  UI_STATUSBAR_STATE_INACTIVE,
  UI_STATUSBAR_STATE_ACTIVE,

} ui_statusbar_state;

int ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state );
int ui_statusbar_update_speed( float speed );

typedef enum ui_tape_browser_update_type {

  UI_TAPE_BROWSER_NEW_TAPE,             /* Whole tape image has changed
                                           implies modified reset */
  UI_TAPE_BROWSER_SELECT_BLOCK,         /* Tape block selected has changed */
  UI_TAPE_BROWSER_NEW_BLOCK,            /* A new block has been appended,
                                           implies modified set */
  UI_TAPE_BROWSER_MODIFIED,             /* Tape modified status has changed */

} ui_tape_browser_update_type;

/* Cause the tape browser to be updated */
int ui_tape_browser_update( ui_tape_browser_update_type change,
                            libspectrum_tape_block *block );

char *ui_get_open_filename( const char *title );
char *ui_get_save_filename( const char *title );
int ui_query( const char *message );

#ifdef USE_WIDGET
#include "ui/widget/widget.h"
#define ui_widget_finish() widget_finish()
#else				/* #ifdef USE_WIDGET */
#define ui_widget_finish()
#endif				/* #ifdef USE_WIDGET */

/* Code called at start and end of emulation if widget system is used */
int ui_widget_init( void );
int ui_widget_end( void );

/* How many levels deep have we recursed through widgets; -1 => none */
extern int ui_widget_level;

/* widget system popup the apropriate menu */
void ui_popup_menu( int native_key );

void ui_widget_keyhandler( int native_key );

void ui_pokemem_selector( const char *filename );

#endif			/* #ifndef FUSE_UI_H */
