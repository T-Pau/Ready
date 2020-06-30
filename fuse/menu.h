/* menu.h: general menu callbacks
   Copyright (c) 2004-2015 Philip Kendall

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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#ifndef FUSE_MENU_H
#define FUSE_MENU_H

#include <libspectrum.h>

#include <config.h>

#include "ui/scaler/scaler.h"

#ifdef UI_GTK

#include "compat.h"
#include <gtk/gtk.h>

#define MENU_CALLBACK( name ) \
  void name( GtkAction *gtk_action GCC_UNUSED, gpointer data GCC_UNUSED )
#define MENU_CALLBACK_WITH_ACTION( name ) \
  void name( GtkAction *gtk_action GCC_UNUSED, guint action )

#else			/* #ifdef UI_GTK */

#define MENU_CALLBACK( name ) void name( int action )
#define MENU_CALLBACK_WITH_ACTION( name ) void name( int action )

#endif			/* #ifdef UI_GTK */

#define MENU_DETAIL( name ) const char* name( void )

/*
 * Things defined in menu.c
 */

MENU_CALLBACK( menu_file_open );
MENU_CALLBACK( menu_file_recording_continuerecording );
MENU_CALLBACK( menu_file_recording_insertsnapshot );
MENU_CALLBACK( menu_file_recording_rollback );
MENU_CALLBACK( menu_file_recording_rollbackto );
MENU_CALLBACK( menu_file_recording_play );
MENU_CALLBACK( menu_file_recording_stop );
MENU_CALLBACK( menu_file_recording_finalise );
MENU_CALLBACK( menu_file_aylogging_stop );
MENU_CALLBACK( menu_file_screenshot_openscrscreenshot );
MENU_CALLBACK( menu_file_screenshot_openmltscreenshot );

MENU_CALLBACK( menu_file_scalablevectorgraphics_startcaptureinlinemode );
MENU_CALLBACK( menu_file_scalablevectorgraphics_startcaptureindotmode );
MENU_CALLBACK( menu_file_scalablevectorgraphics_stopcapture );

MENU_CALLBACK( menu_file_movie_stop );
MENU_CALLBACK( menu_file_movie_pause );

MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_machine_select );
MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_peripheral_select );
MENU_CALLBACK( menu_options_filter );
MENU_DETAIL( menu_filter_detail );
MENU_CALLBACK( menu_options_fullscreen );
MENU_CALLBACK( menu_options_save );

MENU_CALLBACK( menu_machine_profiler_start );
MENU_CALLBACK( menu_machine_profiler_stop );
MENU_CALLBACK( menu_machine_nmi );
MENU_CALLBACK( menu_machine_multifaceredbutton );
MENU_CALLBACK( menu_machine_didaktiksnap );

MENU_CALLBACK( menu_media_tape_browse );
MENU_CALLBACK( menu_media_tape_open );
MENU_CALLBACK( menu_media_tape_play );
MENU_CALLBACK( menu_media_tape_rewind );
MENU_CALLBACK( menu_media_tape_clear );
MENU_CALLBACK( menu_media_tape_write );
MENU_CALLBACK( menu_media_tape_recordstart );
MENU_CALLBACK( menu_media_tape_recordstop );
MENU_DETAIL( menu_tape_detail );

MENU_CALLBACK_WITH_ACTION( menu_media_insert_new );
MENU_CALLBACK_WITH_ACTION( menu_media_insert );
MENU_CALLBACK_WITH_ACTION( menu_media_eject );
MENU_CALLBACK_WITH_ACTION( menu_media_save );
MENU_CALLBACK_WITH_ACTION( menu_media_flip );
MENU_CALLBACK_WITH_ACTION( menu_media_writeprotect );

MENU_CALLBACK_WITH_ACTION( menu_media_if1_rs232 );

MENU_CALLBACK( menu_media_cartridge_timexdock_insert );
MENU_CALLBACK( menu_media_cartridge_timexdock_eject );
MENU_CALLBACK( menu_media_cartridge_interface2_insert );
MENU_CALLBACK( menu_media_cartridge_interface2_eject );

MENU_CALLBACK_WITH_ACTION( menu_media_ide_insert );
MENU_CALLBACK_WITH_ACTION( menu_media_ide_commit );
MENU_CALLBACK_WITH_ACTION( menu_media_ide_eject );

/*
 * Things to be defined elsewhere
 */

/* Direct menu callbacks */

MENU_CALLBACK( menu_file_savesnapshot );
MENU_CALLBACK( menu_file_recording_record );
MENU_CALLBACK( menu_file_recording_recordfromsnapshot );
MENU_CALLBACK( menu_file_loadbinarydata );
MENU_CALLBACK( menu_file_savebinarydata );
MENU_CALLBACK( menu_file_exit );

MENU_CALLBACK( menu_file_aylogging_record );

MENU_CALLBACK( menu_file_screenshot_savescreenasscr );
MENU_CALLBACK( menu_file_screenshot_savescreenaspng );
MENU_CALLBACK( menu_file_screenshot_savescreenasmlt );

MENU_CALLBACK( menu_file_movie_record );
MENU_CALLBACK( menu_file_movie_record_recordfromrzx );

MENU_CALLBACK( menu_options_general );
MENU_CALLBACK( menu_options_media );
MENU_CALLBACK( menu_options_sound );
MENU_CALLBACK( menu_options_peripherals_general );
MENU_CALLBACK( menu_options_peripherals_disk );
MENU_CALLBACK( menu_options_rzx );
MENU_CALLBACK( menu_options_movie );
MENU_CALLBACK( menu_options_diskoptions );
MENU_DETAIL( menu_plus3a_detail );
MENU_DETAIL( menu_plus3b_detail );
MENU_DETAIL( menu_beta128a_detail );
MENU_DETAIL( menu_beta128b_detail );
MENU_DETAIL( menu_beta128c_detail );
MENU_DETAIL( menu_beta128d_detail );
MENU_DETAIL( menu_opus1_detail );
MENU_DETAIL( menu_opus2_detail );
MENU_DETAIL( menu_plusd1_detail );
MENU_DETAIL( menu_plusd2_detail );
MENU_DETAIL( menu_didaktik_a_detail );
MENU_DETAIL( menu_didaktik_b_detail );
MENU_DETAIL( menu_disciple1_detail );
MENU_DETAIL( menu_disciple2_detail );
MENU_CALLBACK_WITH_ACTION( menu_options_joysticks_select );
MENU_DETAIL( menu_keyboard_joystick_detail );
MENU_DETAIL( menu_joystick_1_detail );
MENU_DETAIL( menu_joystick_2_detail );

MENU_CALLBACK( menu_machine_pause );
MENU_CALLBACK_WITH_ACTION( menu_machine_reset );
MENU_CALLBACK( menu_machine_select );
MENU_DETAIL( menu_machine_detail );
MENU_CALLBACK( menu_machine_debugger );
MENU_CALLBACK( menu_machine_pokefinder );
MENU_CALLBACK( menu_machine_pokememory );
MENU_CALLBACK( menu_machine_memorybrowser );

MENU_CALLBACK( menu_help_keyboard );
MENU_CALLBACK( menu_help_about );

/* Called from elsewhere (generally from one of the routines defined
   in menu.c) */

int menu_select_roms_with_title( const char *title, size_t start,
				 size_t count, int is_peripheral );
scaler_type menu_get_scaler( scaler_available_fn selector );
int menu_check_media_changed( void );

#endif				/* #ifndef FUSE_MENU_H */
