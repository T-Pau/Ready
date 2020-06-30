/* phantom_typist.c: starting game loading automatically
   Copyright (c) 2017 Philip Kendall

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

#include <string.h>
#ifdef HAVE_STRINGS_STRCASECMP
#include <strings.h>
#endif      /* #ifdef HAVE_STRINGS_STRCASECMP */

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "infrastructure/startup_manager.h"
#include "keyboard.h"
#include "module.h"
#include "phantom_typist.h"
#include "settings.h"
#include "timer/timer.h"

/* The various high level ways the phantom typist can type */
typedef enum phantom_typist_highlevel_mode_t {
  /* Use keyword entry */
  PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYWORD,

  /* Use keystroke entry */
  PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYSTROKE,

  /* Use the loader option from the menu */
  PHANTOM_TYPIST_HIGHLEVEL_MODE_MENU,

  /* Use the loader option from the menu but with an additional delay */
  PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS2A,

  /* Use the loader option from the menu but with an additional delay
     and special handling of LOAD ""CODE programs */
  PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS3
} phantom_typist_highlevel_mode_t;

/* The various strings of keypresses that the phantom typist can use */
typedef enum phantom_typist_mode_t {
  /* J, SS + P, SS + P, Enter => LOAD "" */
  PHANTOM_TYPIST_MODE_JPP,

  /* J, SS + P, SS + P, CS + SS, I, Enter => LOAD ""CODE */
  PHANTOM_TYPIST_MODE_JPPI,

  PHANTOM_TYPIST_MODE_ENTER, /* Enter only */

  /* Down, Enter, L, O, A, D, SS + P, SS + P, C, O, D, E =>
     LOAD ""CODE in BASIC */
  PHANTOM_TYPIST_MODE_DOWN_LOADPPCODE,

  /* Wait a bit and then perform the above */
  PHANTOM_TYPIST_MODE_WAIT_DOWN_LOADPPCODE,

  /* Wait a bit, Down, Enter,
     L, O, A, D, SS + P, T, SS + Z, SS + P, Enter,
     L, O, A, D, SS + P, SS + P, C, O, D, E, Enter =>
     LOAD "t" followed by LOAD ""CODE in BASIC */
  PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK,

  /* L, O, A, D, SS + P, SS + P, Enter => LOAD "" */
  PHANTOM_TYPIST_MODE_LOADPP,

  /* L, O, A, D, SS + P, SS + P, C, O, D, E, Enter => LOAD ""CODE */
  PHANTOM_TYPIST_MODE_LOADPPCODE
} phantom_typist_mode_t;

typedef enum phantom_typist_state_t {
  /* The phantom typist is inactive */
  PHANTOM_TYPIST_STATE_INACTIVE,

  /* The phantom typist is waiting for the keyboard to be read */
  PHANTOM_TYPIST_STATE_WAITING,

  /* JPP mode */
  PHANTOM_TYPIST_STATE_LOAD,
  PHANTOM_TYPIST_STATE_QUOTE1,
  PHANTOM_TYPIST_STATE_QUOTE2,
  PHANTOM_TYPIST_STATE_ENTER,

  /* Enter mode */
  PHANTOM_TYPIST_STATE_ENTER_ONLY,
  PHANTOM_TYPIST_STATE_WAIT_AFTER_ENTER,

  /* LOAD mode - chains into the quotes of JPP mode */
  PHANTOM_TYPIST_STATE_LOAD_L,
  PHANTOM_TYPIST_STATE_LOAD_O,
  PHANTOM_TYPIST_STATE_LOAD_A,
  PHANTOM_TYPIST_STATE_LOAD_D,

  /* For producing "CODE" in keyword mode */
  PHANTOM_TYPIST_STATE_EXTENDED_MODE,
  PHANTOM_TYPIST_STATE_CODE,

  /* For producing "CODE" in single character entry mode */
  PHANTOM_TYPIST_STATE_CODE_C,
  PHANTOM_TYPIST_STATE_CODE_O,
  PHANTOM_TYPIST_STATE_CODE_D,
  PHANTOM_TYPIST_STATE_CODE_E,

  /* For selecting 128K BASIC or similar */
  PHANTOM_TYPIST_STATE_LONG_WAIT_DOWN,
  PHANTOM_TYPIST_STATE_WAIT_DOWN,
  PHANTOM_TYPIST_STATE_DOWN,
  PHANTOM_TYPIST_STATE_SELECT_BASIC,
  PHANTOM_TYPIST_STATE_WAIT,

  /* For selecting tape on a +3 machine */
  PHANTOM_TYPIST_STATE_TCOLON_T,
  PHANTOM_TYPIST_STATE_TCOLON_COLON,

  /* For typing L and O somewhat slowly */
  PHANTOM_TYPIST_STATE_SLOW_LOAD_L,
  PHANTOM_TYPIST_STATE_SLOW_LOAD_O,

} phantom_typist_state_t;

/* States for the phantom typist state machine */
struct state_info_t {
  /* The keys to be pressed in this state */
  keyboard_key_name keys_to_press[2];

  /* The function used to determine the next state */
  phantom_typist_state_t (*next_state_fn)( void );

  /* The next state to move to - used if next_state_fn is NULL */
  phantom_typist_state_t next_state;

  /* The number of frames to wait before acting in this state */
  int delay_before_state;
};

/* Next state selector functions */
static phantom_typist_state_t t_colon_or_quote( void );
static phantom_typist_state_t code_or_enter( void );
static phantom_typist_state_t next_command_or_end( void );

/* Reset function */
static void phantom_typist_reset( int hard_reset );

/* Definitions for the phantom typist's state machine */
static struct state_info_t state_info[] = {
  /* INACTIVE and WAITING - data not used */
  { { 0, 0 }, NULL, 0, 0 },
  { { 0, 0 }, NULL, 0, 0 },

  { { KEYBOARD_j, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_QUOTE1, 8 },
  { { KEYBOARD_Symbol, KEYBOARD_p }, t_colon_or_quote, 0, 0 },
  { { KEYBOARD_Symbol, KEYBOARD_p }, code_or_enter, 0, 5 },
  { { KEYBOARD_Enter, KEYBOARD_NONE }, next_command_or_end, 0, 0 },
  
  { { KEYBOARD_Enter, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_WAIT_AFTER_ENTER, 3 },
  /* This state is here to "swallow" the pause while the +3 checks for a disk */
  { { KEYBOARD_NONE, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_INACTIVE, 600 },

  { { KEYBOARD_l, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_LOAD_O, 2 },
  { { KEYBOARD_o, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_LOAD_A, 0 },
  { { KEYBOARD_a, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_LOAD_D, 5 },
  { { KEYBOARD_d, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_QUOTE1, 4 },

  { { KEYBOARD_Caps, KEYBOARD_Symbol }, NULL, PHANTOM_TYPIST_STATE_CODE, 0 },
  { { KEYBOARD_i, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_ENTER, 4 },

  { { KEYBOARD_c, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_CODE_O, 0 },
  { { KEYBOARD_o, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_CODE_D, 4 },
  { { KEYBOARD_d, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_CODE_E, 0 },
  { { KEYBOARD_e, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_ENTER, 4 },

  { { KEYBOARD_NONE, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_WAIT_DOWN, 50 },
  { { KEYBOARD_NONE, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_DOWN, 24 },
  { { KEYBOARD_Caps, KEYBOARD_6 }, NULL, PHANTOM_TYPIST_STATE_SELECT_BASIC, 4 },
  { { KEYBOARD_Enter, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_WAIT, 10 },
  { { KEYBOARD_NONE, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_LOAD_L, 28 },

  { { KEYBOARD_t, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_TCOLON_COLON, 4 },
  { { KEYBOARD_Symbol, KEYBOARD_z }, NULL, PHANTOM_TYPIST_STATE_QUOTE2, 4 },

  { { KEYBOARD_l, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_SLOW_LOAD_O, 5 },
  { { KEYBOARD_o, KEYBOARD_NONE }, NULL, PHANTOM_TYPIST_STATE_LOAD_A, 30 }
};

static phantom_typist_mode_t phantom_typist_mode;
static phantom_typist_state_t phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static phantom_typist_state_t next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static int delay = 0;
static libspectrum_byte keyboard_ports_read = 0x00;
static int command_count;

static module_info_t phantom_typist_module_info = {
  phantom_typist_reset,
  NULL,
  NULL,
  NULL,
  NULL,
};

static int
phantom_typist_init( void *context )
{
  module_register( &phantom_typist_module_info );
  return 0;
}

void
phantom_typist_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_PHANTOM_TYPIST, dependencies,
                            ARRAY_SIZE( dependencies ), phantom_typist_init,
                            NULL, NULL );
}

static void
phantom_typist_reset( int hard_reset )
{
  phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
  next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
}

static phantom_typist_highlevel_mode_t
get_highlevel_mode( libspectrum_machine machine )
{
  phantom_typist_highlevel_mode_t highlevel_mode;

  switch( machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_48_NTSC:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
    default:
      highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYWORD;
      break;

    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_128E:
    case LIBSPECTRUM_MACHINE_PLUS2:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_MENU;
      break;

    case LIBSPECTRUM_MACHINE_PLUS2A:
      highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS2A;
      break;

    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
      highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS3;
      break;

    case LIBSPECTRUM_MACHINE_SE:
      highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYSTROKE;
      break;
  }

  return highlevel_mode;
}

static void
set_state_waiting( void )
{
  command_count = 0;
  phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;
  next_phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;

  timer_start_fastloading();
}

void
phantom_typist_activate( libspectrum_machine machine, int needs_code )
{
  phantom_typist_highlevel_mode_t highlevel_mode;
  const char *setting = settings_current.phantom_typist_mode;

  if( strcasecmp( setting, "Keyword" ) == 0 ) {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYWORD;
  } else if( strcasecmp( setting, "Keystroke" ) == 0) {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYSTROKE;
  } else if( strcasecmp( setting, "Menu" ) == 0) {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_MENU;
  } else if( strcasecmp( setting, "Plus 2A" ) == 0 ||
             strcasecmp( setting, "plus2a" ) == 0) {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS2A;
  } else if( strcasecmp( setting, "Plus 3" ) == 0 ||
             strcasecmp( setting, "plus3" ) == 0) {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS3;
  } else if( strcasecmp( setting, "Auto" ) == 0) {
    highlevel_mode = get_highlevel_mode( machine );
  } else {
    highlevel_mode = PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYWORD;
  }

  switch( highlevel_mode ) {
    case PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYWORD:
    default:
      phantom_typist_mode = needs_code ?
        PHANTOM_TYPIST_MODE_JPPI :
        PHANTOM_TYPIST_MODE_JPP;
      break;

    case PHANTOM_TYPIST_HIGHLEVEL_MODE_KEYSTROKE:
      phantom_typist_mode = needs_code ?
        PHANTOM_TYPIST_MODE_LOADPPCODE :
        PHANTOM_TYPIST_MODE_LOADPP;
      break;

    case PHANTOM_TYPIST_HIGHLEVEL_MODE_MENU:
      phantom_typist_mode = needs_code ?
        PHANTOM_TYPIST_MODE_DOWN_LOADPPCODE :
        PHANTOM_TYPIST_MODE_ENTER;
      break;

    case PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS2A:
      phantom_typist_mode = needs_code ?
        PHANTOM_TYPIST_MODE_WAIT_DOWN_LOADPPCODE :
        PHANTOM_TYPIST_MODE_ENTER;
      break;

    case PHANTOM_TYPIST_HIGHLEVEL_MODE_PLUS3:
      phantom_typist_mode = needs_code ?
        PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK :
        PHANTOM_TYPIST_MODE_ENTER;
      break;
  }


  set_state_waiting();
}

void
phantom_typist_activate_disk( void )
{
  phantom_typist_mode = PHANTOM_TYPIST_MODE_ENTER;
  set_state_waiting();
}

void
phantom_typist_deactivate( void )
{
  if( phantom_typist_is_active() ) {
    phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;
    next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
  }
}

static void
process_waiting_state( libspectrum_byte high_byte )
{
  switch( high_byte ) {
    case 0xfe:
    case 0xfd:
    case 0xfb:
    case 0xf7:
    case 0xef:
    case 0xdf:
    case 0xbf:
    case 0x7f:
      keyboard_ports_read |= ~high_byte;
      break;
  }

  if( keyboard_ports_read == 0xff ) {
    switch( phantom_typist_mode ) {
      case PHANTOM_TYPIST_MODE_JPP:
      case PHANTOM_TYPIST_MODE_JPPI:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_LOAD;
        break;

      case PHANTOM_TYPIST_MODE_ENTER:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_ENTER_ONLY;
        break;

      case PHANTOM_TYPIST_MODE_DOWN_LOADPPCODE:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_DOWN;
        break;

      case PHANTOM_TYPIST_MODE_WAIT_DOWN_LOADPPCODE:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_WAIT_DOWN;
        break;

      case PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_LONG_WAIT_DOWN;
        break;

      case PHANTOM_TYPIST_MODE_LOADPP:
      case PHANTOM_TYPIST_MODE_LOADPPCODE:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_LOAD_L;
        break;

      default:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
        break;
    }
  }
}

static phantom_typist_state_t
t_colon_or_quote( void )
{
  phantom_typist_state_t next_state;

  if( phantom_typist_mode == PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK &&
      command_count == 0 ) {
    next_state = PHANTOM_TYPIST_STATE_TCOLON_T;
  } else {
    next_state = PHANTOM_TYPIST_STATE_QUOTE2;
  }

  return next_state;
}

static phantom_typist_state_t
code_or_enter( void )
{
  phantom_typist_state_t next_state;

  switch( phantom_typist_mode ) {
    case PHANTOM_TYPIST_MODE_JPP:
    case PHANTOM_TYPIST_MODE_LOADPP:
      next_state = PHANTOM_TYPIST_STATE_ENTER;
      break;
    case PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK:
      next_state = command_count == 0 ?
        PHANTOM_TYPIST_STATE_ENTER :
        PHANTOM_TYPIST_STATE_CODE_C;
      break;
    case PHANTOM_TYPIST_MODE_JPPI:
      next_state = PHANTOM_TYPIST_STATE_EXTENDED_MODE;
      break;
    case PHANTOM_TYPIST_MODE_LOADPPCODE:
    case PHANTOM_TYPIST_MODE_DOWN_LOADPPCODE:
    case PHANTOM_TYPIST_MODE_WAIT_DOWN_LOADPPCODE:
      next_state = PHANTOM_TYPIST_STATE_CODE_C;
      break;
    default:
      next_state = PHANTOM_TYPIST_STATE_INACTIVE;
      break;
  }

  return next_state;
}

static phantom_typist_state_t
next_command_or_end( void )
{
  phantom_typist_state_t next_state;

  if( phantom_typist_mode == PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK &&
      command_count == 0 ) {
    next_state = PHANTOM_TYPIST_STATE_SLOW_LOAD_L;
  } else {
    next_state = PHANTOM_TYPIST_STATE_INACTIVE;
  }

  return next_state;
}

static libspectrum_byte
process_state( libspectrum_byte high_byte )
{
  libspectrum_byte r = 0xff;
  struct state_info_t *this_state = &state_info[phantom_typist_state];

  r &= keyboard_simulate_keypress( high_byte, this_state->keys_to_press[0] );
  r &= keyboard_simulate_keypress( high_byte, this_state->keys_to_press[1] );
  next_phantom_typist_state = this_state->next_state_fn ?
    this_state->next_state_fn() :
    this_state->next_state;

  return r;
}

libspectrum_byte
phantom_typist_ula_read( libspectrum_word port )
{
  libspectrum_byte r = 0xff;
  libspectrum_byte high_byte = port >> 8;

  if( delay != 0 ) {
    return r;
  }

  switch( phantom_typist_state ) {
    case PHANTOM_TYPIST_STATE_INACTIVE:
      /* Do nothing */
      break;

    case PHANTOM_TYPIST_STATE_WAITING:
      process_waiting_state( high_byte );
      break;

     default: 
      r &= process_state( high_byte );
      break;
  }

  return r;
}

int
phantom_typist_is_active( void )
{
  return phantom_typist_state != PHANTOM_TYPIST_STATE_INACTIVE;
}

void
phantom_typist_frame( void )
{
  keyboard_ports_read = 0x00;
  if( next_phantom_typist_state != phantom_typist_state ) {
    if( phantom_typist_mode == PHANTOM_TYPIST_MODE_PLUS3_CODE_BLOCK &&
        phantom_typist_state == PHANTOM_TYPIST_STATE_ENTER ) {
      command_count++;
    }

    phantom_typist_state = next_phantom_typist_state;
    delay = state_info[phantom_typist_state].delay_before_state;

    if( next_phantom_typist_state == PHANTOM_TYPIST_STATE_INACTIVE ) {
      timer_stop_fastloading();
    }
  }

  if( delay > 0 ) {
    delay--;
  }
}
