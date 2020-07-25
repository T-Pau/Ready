/* ui.c: User interface routines, but those which are independent of any UI
   Copyright (c) 2002-2015 Philip Kendall
   Copyright (c) 2016 Sergio Baldoví

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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "fuse.h"
#include "peripherals/if1.h"
#include "peripherals/kempmouse.h"
#include "settings.h"
#include "tape.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "ui/widget/widget.h"

#define MESSAGE_MAX_LENGTH 256

/* We don't start in a widget */
int ui_widget_level = -1;

static char last_message[ MESSAGE_MAX_LENGTH ] = "";
static size_t frames_since_last_message = 0;

static int
print_error_to_stderr( ui_error_level severity, const char *message );

int
ui_error( ui_error_level severity, const char *format, ... )
{
  int error;
  va_list ap;

  va_start( ap, format );
  error = ui_verror( severity, format, ap );
  va_end( ap );

  return error;
}

int
ui_verror( ui_error_level severity, const char *format, va_list ap )
{
  char message[ MESSAGE_MAX_LENGTH ];

  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );

  /* Skip the message if the same message was displayed recently */
  if( frames_since_last_message < 50 && !strcmp( message, last_message ) ) {
    frames_since_last_message = 0;
    return 0;
  }

  /* And store the 'last message' */
  strncpy( last_message, message, MESSAGE_MAX_LENGTH );
  last_message[ MESSAGE_MAX_LENGTH - 1 ] = '\0';

  print_error_to_stderr( severity, message );

  /* Do any UI-specific bits as well */
  ui_error_specific( severity, message );

  return 0;
}

ui_confirm_save_t
ui_confirm_save( const char *format, ... )
{
  va_list ap;
  char message[ MESSAGE_MAX_LENGTH ];
  ui_confirm_save_t confirm;

  va_start( ap, format );

  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );
  confirm = ui_confirm_save_specific( message );

  va_end( ap );

  return confirm;
}

static int
print_error_to_stderr( ui_error_level severity, const char *message )
{
  /* Print the error to stderr if it's more significant than just
     informational */
  if( severity > UI_ERROR_INFO ) {

    /* For the fb and svgalib UIs, we don't want to write to stderr if
       it's a terminal, as it's then likely to be what we're currently
       using for graphics output, and writing text to it isn't a good
       idea. Things are OK if we're exiting though */
#if defined( UI_FB ) || defined( UI_SVGA )
    if( isatty( STDERR_FILENO ) && !fuse_exiting ) return 1;
#endif			/* #if defined( UI_FB ) || defined( UI_SVGA ) */

    fprintf( stderr, "%s: ", fuse_progname );

    switch( severity ) {

    case UI_ERROR_INFO: break;		/* Shouldn't happen */

    case UI_ERROR_WARNING: fprintf( stderr, "warning: " ); break;
    case UI_ERROR_ERROR: fprintf( stderr, "error: " ); break;
    }

    fprintf( stderr, "%s\n", message );
  }

  return 0;
}

libspectrum_error
ui_libspectrum_error( libspectrum_error error GCC_UNUSED, const char *format,
		      va_list ap )
{
  char new_format[ 257 ];
  snprintf( new_format, 256, "libspectrum: %s", format );

  ui_verror( UI_ERROR_ERROR, new_format, ap );

  return LIBSPECTRUM_ERROR_NONE;
}

void
ui_error_frame( void )
{
  frames_since_last_message++;
}

int ui_mouse_present = 0;
int ui_mouse_grabbed = 0;
static int mouse_grab_suspended = 0;

void
ui_mouse_button( int button, int down )
{
  int kempston_button = !settings_current.mouse_swap_buttons;

  if( !ui_mouse_grabbed && !mouse_grab_suspended ) button = 2;

  /* Possibly we'll end up handling _more_ than one mouse interface... */
  switch( button ) {
  case 1:
    if( ui_mouse_grabbed ) kempmouse_update( 0, 0, kempston_button, down );
    break;
  case 3:
    if( ui_mouse_grabbed ) kempmouse_update( 0, 0, !kempston_button, down );
    break;
  case 2:
    if( ui_mouse_present && settings_current.kempston_mouse
	&& !down && !mouse_grab_suspended )
      ui_mouse_grabbed =
	ui_mouse_grabbed ? ui_mouse_release( 1 ) : ui_mouse_grab( 0 );
    break;
  }
}

void
ui_mouse_motion( int x, int y )
{
  if( ui_mouse_grabbed ) kempmouse_update( x, y, -1, 0 );
}

void
ui_mouse_suspend( void )
{
  mouse_grab_suspended = ui_mouse_grabbed ? 2 : 1;
  ui_mouse_grabbed = ui_mouse_release( 1 );
}

void
ui_mouse_resume( void )
{
  if( mouse_grab_suspended == 2) ui_mouse_grabbed = ui_mouse_grab( 0 );
  mouse_grab_suspended = 0;
}

struct menu_item_entries {
  ui_menu_item item;
  const char *string1;
  const char *string2; int string2_inverted;
  const char *string3; int string3_inverted;
  const char *string4; int string4_inverted;
  const char *string5; int string5_inverted;
  const char *string6; int string6_inverted;
  const char *string7; int string7_inverted;
};

static const struct menu_item_entries menu_item_lookup[] = {

  { UI_MENU_ITEM_FILE_SVG_CAPTURE,
    "/File/Scalable Vector Graphics/Stop capture",
    "/File/Scalable Vector Graphics/Start capture in dot mode...", 1,
    "/File/Scalable Vector Graphics/Start capture in line mode...", 1
  },

  { UI_MENU_ITEM_FILE_MOVIE_RECORDING, "/File/Movie/Stop",
    "/File/Movie/Pause", 0,
    "/File/Movie/Continue", 0,
    "/File/Movie/Record...", 1,
    "/File/Movie/Record from RZX...", 1
  },
  
  { UI_MENU_ITEM_FILE_MOVIE_PAUSE, "/File/Movie/Pause",
    "/File/Movie/Continue", 1,
  },
  
  { UI_MENU_ITEM_MACHINE_PROFILER, "/Machine/Profiler/Stop",
    "/Machine/Profiler/Start", 1 },

  { UI_MENU_ITEM_MACHINE_MULTIFACE, "/Machine/Multiface Red Button" },

  { UI_MENU_ITEM_MACHINE_DIDAKTIK80_SNAP, "/Machine/Didaktik SNAP" },

  { UI_MENU_ITEM_MEDIA_CARTRIDGE, "/Media/Cartridge" },

  { UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK, "/Media/Cartridge/Timex Dock" },

  { UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT,
    "/Media/Cartridge/Timex Dock/Eject" },

  { UI_MENU_ITEM_MEDIA_IF1, "/Media/Interface 1" },

  { UI_MENU_ITEM_MEDIA_IF1_M1_EJECT,
    "/Media/Interface 1/Microdrive 1/Eject",
    "/Media/Interface 1/Microdrive 1/Save As...", 0,
    "/Media/Interface 1/Microdrive 1/Save", 0,
    "/Media/Interface 1/Microdrive 1/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M1_WP_SET,
    "/Media/Interface 1/Microdrive 1/Write protect/Enable",
    "/Media/Interface 1/Microdrive 1/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M2_EJECT,
    "/Media/Interface 1/Microdrive 2/Eject",
    "/Media/Interface 1/Microdrive 2/Save As...", 0,
    "/Media/Interface 1/Microdrive 2/Save", 0,
    "/Media/Interface 1/Microdrive 2/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M2_WP_SET,
    "/Media/Interface 1/Microdrive 2/Write protect/Enable",
    "/Media/Interface 1/Microdrive 2/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M3_EJECT,
    "/Media/Interface 1/Microdrive 3/Eject",
    "/Media/Interface 1/Microdrive 3/Save As...", 0,
    "/Media/Interface 1/Microdrive 3/Save", 0,
    "/Media/Interface 1/Microdrive 3/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M3_WP_SET,
    "/Media/Interface 1/Microdrive 3/Write protect/Enable",
    "/Media/Interface 1/Microdrive 3/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M4_EJECT,
    "/Media/Interface 1/Microdrive 4/Eject",
    "/Media/Interface 1/Microdrive 4/Save As...", 0,
    "/Media/Interface 1/Microdrive 4/Save", 0,
    "/Media/Interface 1/Microdrive 4/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M4_WP_SET,
    "/Media/Interface 1/Microdrive 4/Write protect/Enable",
    "/Media/Interface 1/Microdrive 4/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M5_EJECT,
    "/Media/Interface 1/Microdrive 5/Eject",
    "/Media/Interface 1/Microdrive 5/Save As...", 0,
    "/Media/Interface 1/Microdrive 5/Save", 0,
    "/Media/Interface 1/Microdrive 5/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M5_WP_SET,
    "/Media/Interface 1/Microdrive 5/Write protect/Enable",
    "/Media/Interface 1/Microdrive 5/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M6_EJECT,
    "/Media/Interface 1/Microdrive 6/Eject",
    "/Media/Interface 1/Microdrive 6/Save As...", 0,
    "/Media/Interface 1/Microdrive 6/Save", 0,
    "/Media/Interface 1/Microdrive 6/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M6_WP_SET,
    "/Media/Interface 1/Microdrive 6/Write protect/Enable",
    "/Media/Interface 1/Microdrive 6/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M7_EJECT,
    "/Media/Interface 1/Microdrive 7/Eject",
    "/Media/Interface 1/Microdrive 7/Save As...", 0,
    "/Media/Interface 1/Microdrive 7/Save", 0,
    "/Media/Interface 1/Microdrive 7/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M7_WP_SET,
    "/Media/Interface 1/Microdrive 7/Write protect/Enable",
    "/Media/Interface 1/Microdrive 7/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_M8_EJECT,
    "/Media/Interface 1/Microdrive 8/Eject",
    "/Media/Interface 1/Microdrive 8/Save As...", 0,
    "/Media/Interface 1/Microdrive 8/Save", 0,
    "/Media/Interface 1/Microdrive 8/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_IF1_M8_WP_SET,
    "/Media/Interface 1/Microdrive 8/Write protect/Enable",
    "/Media/Interface 1/Microdrive 8/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IF1_RS232_UNPLUG_R,
    "/Media/Interface 1/RS232/Unplug RxD" },

  { UI_MENU_ITEM_MEDIA_IF1_RS232_UNPLUG_T,
    "/Media/Interface 1/RS232/Unplug TxD" },

  { UI_MENU_ITEM_MEDIA_IF1_SNET_UNPLUG,
    "/Media/Interface 1/Sinclair NET/Unplug" },

  { UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2, "/Media/Cartridge/Interface 2" },

  { UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT,
    "/Media/Cartridge/Interface 2/Eject" },

  { UI_MENU_ITEM_MEDIA_DISK, "/Media/Disk" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3, "/Media/Disk/+3" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT,
    "/Media/Disk/+3/Drive A:/Eject",
    "/Media/Disk/+3/Drive A:/Save As...", 0,
    "/Media/Disk/+3/Drive A:/Save", 0,
    "/Media/Disk/+3/Drive A:/Flip disk", 0,
    "/Media/Disk/+3/Drive A:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_FLIP_SET,
    "/Media/Disk/+3/Drive A:/Flip disk/Turn upside down",
    "/Media/Disk/+3/Drive A:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_WP_SET,
    "/Media/Disk/+3/Drive A:/Write protect/Enable",
    "/Media/Disk/+3/Drive A:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_B, "/Media/Disk/+3/Drive B:" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT,
    "/Media/Disk/+3/Drive B:/Eject",
    "/Media/Disk/+3/Drive B:/Save As...", 0,
    "/Media/Disk/+3/Drive B:/Save", 0,
    "/Media/Disk/+3/Drive B:/Flip disk", 0,
    "/Media/Disk/+3/Drive B:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_FLIP_SET,
    "/Media/Disk/+3/Drive B:/Flip disk/Turn upside down",
    "/Media/Disk/+3/Drive B:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_WP_SET,
    "/Media/Disk/+3/Drive B:/Write protect/Enable",
    "/Media/Disk/+3/Drive B:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA, "/Media/Disk/Beta" },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_A, "/Media/Disk/Beta/Drive A:" },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_A_EJECT,
    "/Media/Disk/Beta/Drive A:/Eject",
    "/Media/Disk/Beta/Drive A:/Save As...", 0,
    "/Media/Disk/Beta/Drive A:/Save", 0,
    "/Media/Disk/Beta/Drive A:/Flip disk", 0,
    "/Media/Disk/Beta/Drive A:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_A_FLIP_SET,
    "/Media/Disk/Beta/Drive A:/Flip disk/Turn upside down",
    "/Media/Disk/Beta/Drive A:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_A_WP_SET,
    "/Media/Disk/Beta/Drive A:/Write protect/Enable",
    "/Media/Disk/Beta/Drive A:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_B, "/Media/Disk/Beta/Drive B:" },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_B_EJECT,
    "/Media/Disk/Beta/Drive B:/Eject",
    "/Media/Disk/Beta/Drive B:/Save As...", 0,
    "/Media/Disk/Beta/Drive B:/Save", 0,
    "/Media/Disk/Beta/Drive B:/Flip disk", 0,
    "/Media/Disk/Beta/Drive B:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_B_FLIP_SET,
    "/Media/Disk/Beta/Drive B:/Flip disk/Turn upside down",
    "/Media/Disk/Beta/Drive B:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_B_WP_SET,
    "/Media/Disk/Beta/Drive B:/Write protect/Enable",
    "/Media/Disk/Beta/Drive B:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_C, "/Media/Disk/Beta/Drive C:" },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_C_EJECT,
    "/Media/Disk/Beta/Drive C:/Eject",
    "/Media/Disk/Beta/Drive C:/Save As...", 0,
    "/Media/Disk/Beta/Drive C:/Save", 0,
    "/Media/Disk/Beta/Drive C:/Flip disk", 0,
    "/Media/Disk/Beta/Drive C:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_C_FLIP_SET,
    "/Media/Disk/Beta/Drive C:/Flip disk/Turn upside down",
    "/Media/Disk/Beta/Drive C:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_C_WP_SET,
    "/Media/Disk/Beta/Drive C:/Write protect/Enable",
    "/Media/Disk/Beta/Drive C:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_D, "/Media/Disk/Beta/Drive D:" },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_D_EJECT,
    "/Media/Disk/Beta/Drive D:/Eject",
    "/Media/Disk/Beta/Drive D:/Save As...", 0,
    "/Media/Disk/Beta/Drive D:/Save", 0,
    "/Media/Disk/Beta/Drive D:/Flip disk", 0,
    "/Media/Disk/Beta/Drive D:/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_D_FLIP_SET,
    "/Media/Disk/Beta/Drive D:/Flip disk/Turn upside down",
    "/Media/Disk/Beta/Drive D:/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_BETA_D_WP_SET,
    "/Media/Disk/Beta/Drive D:/Write protect/Enable",
    "/Media/Disk/Beta/Drive D:/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD, "/Media/Disk/+D" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_1, "/Media/Disk/+D/Drive 1" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT,
    "/Media/Disk/+D/Drive 1/Eject",
    "/Media/Disk/+D/Drive 1/Save As...", 0,
    "/Media/Disk/+D/Drive 1/Save", 0,
    "/Media/Disk/+D/Drive 1/Flip disk", 0,
    "/Media/Disk/+D/Drive 1/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_FLIP_SET,
    "/Media/Disk/+D/Drive 1/Flip disk/Turn upside down",
    "/Media/Disk/+D/Drive 1/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
    "/Media/Disk/+D/Drive 1/Write protect/Enable",
    "/Media/Disk/+D/Drive 1/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_2, "/Media/Disk/+D/Drive 2" },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT,
    "/Media/Disk/+D/Drive 2/Eject",
    "/Media/Disk/+D/Drive 2/Save As...", 0,
    "/Media/Disk/+D/Drive 2/Save", 0,
    "/Media/Disk/+D/Drive 2/Flip disk", 0,
    "/Media/Disk/+D/Drive 2/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_FLIP_SET,
    "/Media/Disk/+D/Drive 2/Flip disk/Turn upside down",
    "/Media/Disk/+D/Drive 2/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
    "/Media/Disk/+D/Drive 2/Write protect/Enable",
    "/Media/Disk/+D/Drive 2/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK, "/Media/Disk/Didaktik 80" },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A, "/Media/Disk/Didaktik 80/Drive A" },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_EJECT,
    "/Media/Disk/Didaktik 80/Drive A/Eject",
    "/Media/Disk/Didaktik 80/Drive A/Save As...", 0,
    "/Media/Disk/Didaktik 80/Drive A/Save", 0,
    "/Media/Disk/Didaktik 80/Drive A/Flip disk", 0,
    "/Media/Disk/Didaktik 80/Drive A/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_FLIP_SET,
    "/Media/Disk/Didaktik 80/Drive A/Flip disk/Turn upside down",
    "/Media/Disk/Didaktik 80/Drive A/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_A_WP_SET,
    "/Media/Disk/Didaktik 80/Drive A/Write protect/Enable",
    "/Media/Disk/Didaktik 80/Drive A/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B, "/Media/Disk/Didaktik 80/Drive B" },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_EJECT,
    "/Media/Disk/Didaktik 80/Drive B/Eject",
    "/Media/Disk/Didaktik 80/Drive B/Save As...", 0,
    "/Media/Disk/Didaktik 80/Drive B/Save", 0,
    "/Media/Disk/Didaktik 80/Drive B/Flip disk", 0,
    "/Media/Disk/Didaktik 80/Drive B/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_FLIP_SET,
    "/Media/Disk/Didaktik 80/Drive B/Flip disk/Turn upside down",
    "/Media/Disk/Didaktik 80/Drive B/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DIDAKTIK_B_WP_SET,
    "/Media/Disk/Didaktik 80/Drive B/Write protect/Enable",
    "/Media/Disk/Didaktik 80/Drive B/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE, "/Media/Disk/DISCiPLE" },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1, "/Media/Disk/DISCiPLE/Drive 1" },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_EJECT,
    "/Media/Disk/DISCiPLE/Drive 1/Eject",
    "/Media/Disk/DISCiPLE/Drive 1/Save As...", 0,
    "/Media/Disk/DISCiPLE/Drive 1/Save", 0,
    "/Media/Disk/DISCiPLE/Drive 1/Flip disk", 0,
    "/Media/Disk/DISCiPLE/Drive 1/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_FLIP_SET,
    "/Media/Disk/DISCiPLE/Drive 1/Flip disk/Turn upside down",
    "/Media/Disk/DISCiPLE/Drive 1/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_1_WP_SET,
    "/Media/Disk/DISCiPLE/Drive 1/Write protect/Enable",
    "/Media/Disk/DISCiPLE/Drive 1/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2, "/Media/Disk/DISCiPLE/Drive 2" },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_EJECT,
    "/Media/Disk/DISCiPLE/Drive 2/Eject",
    "/Media/Disk/DISCiPLE/Drive 2/Save As...", 0,
    "/Media/Disk/DISCiPLE/Drive 2/Save", 0,
    "/Media/Disk/DISCiPLE/Drive 2/Flip disk", 0,
    "/Media/Disk/DISCiPLE/Drive 2/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_FLIP_SET,
    "/Media/Disk/DISCiPLE/Drive 2/Flip disk/Turn upside down",
    "/Media/Disk/DISCiPLE/Drive 2/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_DISCIPLE_2_WP_SET,
    "/Media/Disk/DISCiPLE/Drive 2/Write protect/Enable",
    "/Media/Disk/DISCiPLE/Drive 2/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS, "/Media/Disk/Opus" },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_1, "/Media/Disk/Opus/Drive 1" },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_1_EJECT,
    "/Media/Disk/Opus/Drive 1/Eject",
    "/Media/Disk/Opus/Drive 1/Save As...", 0,
    "/Media/Disk/Opus/Drive 1/Save", 0,
    "/Media/Disk/Opus/Drive 1/Flip disk", 0,
    "/Media/Disk/Opus/Drive 1/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_1_FLIP_SET,
    "/Media/Disk/Opus/Drive 1/Flip disk/Turn upside down",
    "/Media/Disk/Opus/Drive 1/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_1_WP_SET,
    "/Media/Disk/Opus/Drive 1/Write protect/Enable",
    "/Media/Disk/Opus/Drive 1/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_2, "/Media/Disk/Opus/Drive 2" },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_2_EJECT,
    "/Media/Disk/Opus/Drive 2/Eject",
    "/Media/Disk/Opus/Drive 2/Save As...", 0,
    "/Media/Disk/Opus/Drive 2/Save", 0,
    "/Media/Disk/Opus/Drive 2/Flip disk", 0,
    "/Media/Disk/Opus/Drive 2/Write protect", 0 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_2_FLIP_SET,
    "/Media/Disk/Opus/Drive 2/Flip disk/Turn upside down",
    "/Media/Disk/Opus/Drive 2/Flip disk/Turn back", 1 },

  { UI_MENU_ITEM_MEDIA_DISK_OPUS_2_WP_SET,
    "/Media/Disk/Opus/Drive 2/Write protect/Enable",
    "/Media/Disk/Opus/Drive 2/Write protect/Disable", 1 },

  { UI_MENU_ITEM_MEDIA_IDE, "/Media/IDE" },

  { UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT, "/Media/IDE/Simple 8-bit" },

  { UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
    "/Media/IDE/Simple 8-bit/Master/Commit",
    "/Media/IDE/Simple 8-bit/Master/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT,
    "/Media/IDE/Simple 8-bit/Slave/Commit",
    "/Media/IDE/Simple 8-bit/Slave/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_ZXATASP, "/Media/IDE/ZXATASP" },

  { UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
    "/Media/IDE/ZXATASP/Master/Commit",
    "/Media/IDE/ZXATASP/Master/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT,
    "/Media/IDE/ZXATASP/Slave/Commit",
    "/Media/IDE/ZXATASP/Slave/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_ZXCF, "/Media/IDE/ZXCF CompactFlash" },

  { UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT,
    "/Media/IDE/ZXCF CompactFlash/Commit",
    "/Media/IDE/ZXCF CompactFlash/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_DIVIDE, "/Media/IDE/DivIDE" },

  { UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT,
    "/Media/IDE/DivIDE/Master/Commit",
    "/Media/IDE/DivIDE/Master/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT,
    "/Media/IDE/DivIDE/Slave/Commit",
    "/Media/IDE/DivIDE/Slave/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_DIVMMC, "/Media/IDE/DivMMC" },

  { UI_MENU_ITEM_MEDIA_IDE_DIVMMC_EJECT,
    "/Media/IDE/DivMMC/Commit",
    "/Media/IDE/DivMMC/Eject", 0 },

  { UI_MENU_ITEM_MEDIA_IDE_ZXMMC, "/Media/IDE/ZXMMC" },

  { UI_MENU_ITEM_MEDIA_IDE_ZXMMC_EJECT,
    "/Media/IDE/ZXMMC/Commit",
    "/Media/IDE/ZXMMC/Eject", 0 },

  { UI_MENU_ITEM_RECORDING,
    "/File/Recording/Stop", 
    "/File/Recording/Record...", 1,
    "/File/Recording/Record from snapshot...", 1,
    "/File/Recording/Continue recording...", 1,
    "/File/Recording/Play...", 1,
    "/File/Recording/Finalise...", 1 },

  { UI_MENU_ITEM_RECORDING_ROLLBACK,
    "/File/Recording/Insert snapshot",
    "/File/Recording/Rollback", 0,
    "/File/Recording/Rollback to...", 0 },

  { UI_MENU_ITEM_AY_LOGGING,
    "/File/AY Logging/Stop",
    "/File/AY Logging/Record...", 1, },

  { UI_MENU_ITEM_TAPE_RECORDING,
    "/Media/Tape/Record Stop",
    "/Media/Tape/Record Start", 1,
    "/Media/Tape/Open...", 1,
    "/Media/Tape/Play", 1,
    "/Media/Tape/Rewind", 1,
    "/Media/Tape/Clear", 1,
    "/Media/Tape/Write...", 1 },
  
  { UI_MENU_ITEM_TAPE_RECORDING, NULL },	/* End marker */

};

int
ui_menu_activate( ui_menu_item item, int active )
{
  const struct menu_item_entries *ptr;

  for( ptr = menu_item_lookup; ptr->string1; ptr++ ) {

    if( item == ptr->item ) {
      ui_menu_item_set_active( ptr->string1, active );
      if( ptr->string2 ) 
	ui_menu_item_set_active( ptr->string2,
				 ptr->string2_inverted ? !active : active );
      if( ptr->string3 ) 
	ui_menu_item_set_active( ptr->string3,
				 ptr->string3_inverted ? !active : active );
      if( ptr->string4 )
	ui_menu_item_set_active( ptr->string4,
				 ptr->string4_inverted ? !active : active );
      if( ptr->string5 )
	ui_menu_item_set_active( ptr->string5,
				 ptr->string5_inverted ? !active : active );
      if( ptr->string6 )
	ui_menu_item_set_active( ptr->string6,
				 ptr->string6_inverted ? !active : active );
      if( ptr->string7 )
	ui_menu_item_set_active( ptr->string7,
				 ptr->string7_inverted ? !active : active );
      return 0;
    }

  }

  ui_error( UI_ERROR_ERROR, "ui_menu_activate: unknown item %d", item );
  return 1;
}

void
ui_menu_disk_update( void )
{
  int drives_avail;

  drives_avail = ui_media_drive_any_available();

  /* Set the disk menu items and statusbar appropriately */

  if( drives_avail ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 1 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
  } else {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 0 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
                         UI_STATUSBAR_STATE_NOT_AVAILABLE );
  }

  ui_media_drive_update_parent_menus();
}

int
ui_tape_write( void )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Write Tape" );
  if( !filename ) { fuse_emulation_unpause(); return 1; }

  tape_write( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();

  return 0;
}

int
ui_mdr_write( int which, int saveas )
{
  int err;
  char *filename = NULL, title[80];

  fuse_emulation_pause();

  snprintf( title, 80, "Fuse - Write Microdrive Cartridge %i", which + 1 );

  if( saveas ) {
    filename = ui_get_save_filename( title );
    if( !filename ) { fuse_emulation_unpause(); return 1; }
  }

  err = if1_mdr_write( which, filename );

  if( saveas ) libspectrum_free( filename );

  fuse_emulation_unpause();

  return err;
}

#ifdef USE_WIDGET
int
ui_widget_init( void )
{
  return widget_init();
}

int
ui_widget_end( void )
{
  return widget_end();
}
#else
int
ui_widget_init( void )
{
  return 0;
}

int
ui_widget_end( void )
{
  return 0;
}

void
ui_popup_menu( int native_key )
{
}

void
ui_widget_keyhandler( int native_key )
{
}
#endif				/* #ifndef USE_WIDGET */
