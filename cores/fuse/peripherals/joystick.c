/* joystick.c: Joystick emulation support
   Copyright (c) 2001-2016 Russell Marks, Darren Salt, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "joystick.h"
#include "keyboard.h"
#include "module.h"
#include "periph.h"
#include "rzx.h"
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

/* Number of joysticks known about & initialised */
int joysticks_supported = 0;

/* The bit masks used by the various joysticks. The order is the same
   as the ordering of buttons in joystick.h:joystick_button (left,
   right, up, down, fire ) */
static const libspectrum_byte kempston_mask[5] =
  { 0x02, 0x01, 0x08, 0x04, 0x10 };
static const libspectrum_byte timex_mask[5] =
  { 0x04, 0x08, 0x01, 0x02, 0x80 };

/* The keys used by the Cursor joystick */
static const keyboard_key_name cursor_key[5] =
  { KEYBOARD_5, KEYBOARD_8, KEYBOARD_7, KEYBOARD_6, KEYBOARD_0 };

/* The keys used by the two Sinclair joysticks */
static const keyboard_key_name sinclair1_key[5] =
  { KEYBOARD_6, KEYBOARD_7, KEYBOARD_9, KEYBOARD_8, KEYBOARD_0 };
static const keyboard_key_name sinclair2_key[5] =
  { KEYBOARD_1, KEYBOARD_2, KEYBOARD_4, KEYBOARD_3, KEYBOARD_5 };

/* The current values for the joysticks we can emulate */
static libspectrum_byte kempston_value;
static libspectrum_byte timex1_value;
static libspectrum_byte timex2_value;
static libspectrum_byte fuller_value;

/* The names of the joysticks we can emulate. Order must correspond to
   that of joystick.h:joystick_type_t */
const char *joystick_name[ JOYSTICK_TYPE_COUNT ] = {
  "None",
  "Cursor",
  "Kempston",
  "Sinclair 1", "Sinclair 2",
  "Timex 1", "Timex 2",
  "Fuller"
};

const char *joystick_connection[ JOYSTICK_CONN_COUNT ] = {
  "None",
  "Keyboard",
  "Joystick 1",
  "Joystick 2",
};

static void joystick_from_snapshot( libspectrum_snap *snap );
static void joystick_to_snapshot( libspectrum_snap *snap );

static module_info_t joystick_module_info = {

  /* .reset = */ NULL,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ joystick_from_snapshot,
  /* .snapshot_to = */ joystick_to_snapshot,

};

static const periph_port_t kempston_strict_decoding[] = {
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
  { 0, 0, NULL, NULL }
};

static const periph_t kempston_strict_periph = {
  /* .option = */ &settings_current.joy_kempston,
  /* .ports = */ kempston_strict_decoding,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static const periph_port_t kempston_loose_decoding[] = {
  { 0x0020, 0x0000, joystick_kempston_read, NULL },
  { 0, 0, NULL, NULL }
};

static const periph_t kempston_loose_periph = {
  /* .option = */ &settings_current.joy_kempston,
  /* .ports = */ kempston_loose_decoding,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

/* Init/shutdown functions. Errors aren't important here */

int
joystick_init( void *context )
{
  joysticks_supported = ui_joystick_init();
  kempston_value = timex1_value = timex2_value = 0x00;
  fuller_value = 0xff;

  module_register( &joystick_module_info );
  periph_register( PERIPH_TYPE_KEMPSTON, &kempston_strict_periph );
  periph_register( PERIPH_TYPE_KEMPSTON_LOOSE, &kempston_loose_periph );

  return 0;
}

void
joystick_end( void )
{
  ui_joystick_end();
}

void
joystick_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_LIBSPECTRUM,
    STARTUP_MANAGER_MODULE_SETUID
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_JOYSTICK, dependencies,
                            ARRAY_SIZE( dependencies ), joystick_init,
                            joystick_end, NULL );
}

int
joystick_press( int which, joystick_button button, int press )
{
  joystick_type_t type;

  switch( which ) {
  case 0: type = settings_current.joystick_1_output; break;
  case 1: type = settings_current.joystick_2_output; break;

  case JOYSTICK_KEYBOARD:
    type = settings_current.joystick_keyboard_output; break;

  default:
    return 0;
  }

  switch( type ) {

  case JOYSTICK_TYPE_CURSOR:
    if( press ) {
      keyboard_press( cursor_key[ button ] );
    } else {
      keyboard_release( cursor_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_KEMPSTON:
    if( press ) {
      kempston_value |=  kempston_mask[ button ];
    } else {
      kempston_value &= ~kempston_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_1:
    if( press ) {
      keyboard_press( sinclair1_key[ button ] );
    } else {
      keyboard_release( sinclair1_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_2:
    if( press ) {
      keyboard_press( sinclair2_key[ button ] );
    } else {
      keyboard_release( sinclair2_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_1:
    if( press ) {
      timex1_value |=  timex_mask[ button ];
    } else {
      timex1_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_2:
    if( press ) {
      timex2_value |=  timex_mask[ button ];
    } else {
      timex2_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_FULLER:
    if( press ) {
      fuller_value &= ~timex_mask[ button ];
    } else {
      fuller_value |=  timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_NONE: return 0;
  }

  ui_error( UI_ERROR_ERROR, "%s:joystick_press:unknown joystick type %d",
	    __FILE__, type );
  fuse_abort();
}

/* Read functions for specific interfaces */

libspectrum_byte
joystick_kempston_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return kempston_value;
}

libspectrum_byte
joystick_timex_read( libspectrum_word port GCC_UNUSED, libspectrum_byte which )
{
  return which ? timex2_value : timex1_value;
}

libspectrum_byte
joystick_fuller_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return fuller_value;
}

static void
joystick_from_snapshot( libspectrum_snap *snap )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  joystick_type_t fuse_type;

  for( i = 0; i < num_joysticks; i++ ) {
    switch( libspectrum_snap_joystick_list( snap, i ) ) {
    case LIBSPECTRUM_JOYSTICK_CURSOR:
      fuse_type = JOYSTICK_TYPE_CURSOR;
      break;
    case LIBSPECTRUM_JOYSTICK_KEMPSTON:            
      fuse_type = JOYSTICK_TYPE_KEMPSTON;
      break;
    case LIBSPECTRUM_JOYSTICK_SINCLAIR_1:
      fuse_type = JOYSTICK_TYPE_SINCLAIR_1;
      break;
    case LIBSPECTRUM_JOYSTICK_SINCLAIR_2:           
      fuse_type = JOYSTICK_TYPE_SINCLAIR_2;
      break;
    case LIBSPECTRUM_JOYSTICK_TIMEX_1:
      fuse_type = JOYSTICK_TYPE_TIMEX_1;
      break;
    case LIBSPECTRUM_JOYSTICK_TIMEX_2:
      fuse_type = JOYSTICK_TYPE_TIMEX_2;
      break;
    case LIBSPECTRUM_JOYSTICK_FULLER:
      fuse_type = JOYSTICK_TYPE_FULLER;
      break;
    default:
      ui_error( UI_ERROR_INFO, "Ignoring unsupported joystick in snapshot %s", 
        libspectrum_joystick_name( libspectrum_snap_joystick_list( snap, i ) ));
      continue;
    }

    if( settings_current.joystick_keyboard_output != fuse_type &&
        settings_current.joystick_1_output != fuse_type &&
        settings_current.joystick_2_output != fuse_type &&
        !rzx_playback ) {
      switch( ui_confirm_joystick( libspectrum_snap_joystick_list(snap,i),
                                   libspectrum_snap_joystick_inputs(snap,i)) ) {
      case UI_CONFIRM_JOYSTICK_KEYBOARD:
        settings_current.joystick_keyboard_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_JOYSTICK_1:
        settings_current.joystick_1_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_JOYSTICK_2:
        settings_current.joystick_2_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_NONE:
        break;
      }
    }

    /* If the snap was configured for a Kempston joystick, enable
       our Kempston emulation in case the snap was reading from
       the joystick to prevent things going haywire */
    if( fuse_type == JOYSTICK_TYPE_KEMPSTON )
      settings_current.joy_kempston = 1;
  }
}

static void
add_joystick( libspectrum_snap *snap, joystick_type_t fuse_type, int inputs )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  libspectrum_joystick libspectrum_type;

  switch( fuse_type ) {
  case JOYSTICK_TYPE_CURSOR:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_CURSOR;
    break;
  case JOYSTICK_TYPE_KEMPSTON:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_KEMPSTON;
    break;
  case JOYSTICK_TYPE_SINCLAIR_1:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_SINCLAIR_1;
    break;
  case JOYSTICK_TYPE_SINCLAIR_2:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_SINCLAIR_2;
    break;
  case JOYSTICK_TYPE_TIMEX_1:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_TIMEX_1;
    break;
  case JOYSTICK_TYPE_TIMEX_2:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_TIMEX_2;
    break;
  case JOYSTICK_TYPE_FULLER:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_FULLER;
    break;
  
  case JOYSTICK_TYPE_NONE:
  default:
    return;
  }

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) == libspectrum_type ) {
      libspectrum_snap_set_joystick_inputs( snap, i, inputs |
                                  libspectrum_snap_joystick_inputs( snap, i ) );
      return;
    }
  }

  libspectrum_snap_set_joystick_list( snap, num_joysticks, libspectrum_type );
  libspectrum_snap_set_joystick_inputs( snap, num_joysticks, inputs );
  libspectrum_snap_set_joystick_active_count( snap, num_joysticks + 1 );
}

static void
joystick_to_snapshot( libspectrum_snap *snap )
{
  if( settings_current.joy_kempston ) {
    add_joystick( snap, JOYSTICK_TYPE_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_NONE );
  }
  add_joystick( snap, settings_current.joystick_keyboard_output,
                LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
  add_joystick( snap, settings_current.joystick_1_output,
                LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
  add_joystick( snap, settings_current.joystick_2_output,
                LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
}
