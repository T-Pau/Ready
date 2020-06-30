/* input.h: generalised input events layer for Fuse
   Copyright (c) 2004 Philip Kendall

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

#ifndef FUSE_INPUT_H
#define FUSE_INPUT_H

typedef enum input_event_type {

  INPUT_EVENT_KEYPRESS,
  INPUT_EVENT_KEYRELEASE,

  INPUT_EVENT_JOYSTICK_PRESS,
  INPUT_EVENT_JOYSTICK_RELEASE,

} input_event_type;

typedef enum input_key {

  INPUT_KEY_NONE = 0x00,

  INPUT_KEY_Tab = 0x09,

  INPUT_KEY_Return = 0x0d,

  INPUT_KEY_Escape = 0x1b,

  INPUT_KEY_space = 0x20,
  INPUT_KEY_exclam,
  INPUT_KEY_quotedbl,
  INPUT_KEY_numbersign,
  INPUT_KEY_dollar,
  INPUT_KEY_percent,
  INPUT_KEY_ampersand,
  INPUT_KEY_apostrophe,
  INPUT_KEY_parenleft,
  INPUT_KEY_parenright,
  INPUT_KEY_asterisk,
  INPUT_KEY_plus,
  INPUT_KEY_comma = 0x2c,
  INPUT_KEY_minus = 0x2d,
  INPUT_KEY_period = 0x2e,
  INPUT_KEY_slash = 0x2f,

  INPUT_KEY_0 = 0x30,
  INPUT_KEY_1,
  INPUT_KEY_2,
  INPUT_KEY_3,
  INPUT_KEY_4,
  INPUT_KEY_5,
  INPUT_KEY_6,
  INPUT_KEY_7,
  INPUT_KEY_8,
  INPUT_KEY_9,

  INPUT_KEY_colon = 0x3a,
  INPUT_KEY_semicolon,
  INPUT_KEY_less,
  INPUT_KEY_equal,
  INPUT_KEY_greater,
  INPUT_KEY_question,
  INPUT_KEY_at,

  INPUT_KEY_A = 0x41,
  INPUT_KEY_B,
  INPUT_KEY_C,
  INPUT_KEY_D,
  INPUT_KEY_E,
  INPUT_KEY_F,
  INPUT_KEY_G,
  INPUT_KEY_H,
  INPUT_KEY_I,
  INPUT_KEY_J,
  INPUT_KEY_K,
  INPUT_KEY_L,
  INPUT_KEY_M,
  INPUT_KEY_N,
  INPUT_KEY_O,
  INPUT_KEY_P,
  INPUT_KEY_Q,
  INPUT_KEY_R,
  INPUT_KEY_S,
  INPUT_KEY_T,
  INPUT_KEY_U,
  INPUT_KEY_V,
  INPUT_KEY_W,
  INPUT_KEY_X,
  INPUT_KEY_Y,
  INPUT_KEY_Z,
  INPUT_KEY_bracketleft,
  INPUT_KEY_backslash,
  INPUT_KEY_bracketright,
  INPUT_KEY_asciicircum = 0x5e,
  INPUT_KEY_dead_circumflex = 0x5e,
  INPUT_KEY_underscore = 0x5f,

  INPUT_KEY_a = 0x61,
  INPUT_KEY_b,
  INPUT_KEY_c,
  INPUT_KEY_d,
  INPUT_KEY_e,
  INPUT_KEY_f,
  INPUT_KEY_g,
  INPUT_KEY_h,
  INPUT_KEY_i,
  INPUT_KEY_j,
  INPUT_KEY_k,
  INPUT_KEY_l,
  INPUT_KEY_m,
  INPUT_KEY_n,
  INPUT_KEY_o,
  INPUT_KEY_p,
  INPUT_KEY_q,
  INPUT_KEY_r,
  INPUT_KEY_s,
  INPUT_KEY_t,
  INPUT_KEY_u,
  INPUT_KEY_v,
  INPUT_KEY_w,
  INPUT_KEY_x,
  INPUT_KEY_y,
  INPUT_KEY_z,
  INPUT_KEY_braceleft,
  INPUT_KEY_bar,
  INPUT_KEY_braceright,
  INPUT_KEY_asciitilde,
  INPUT_KEY_BackSpace = 0x7f,

  INPUT_KEY_KP_Enter = 0x8d,

  INPUT_KEY_Up = 0x100,
  INPUT_KEY_Down,
  INPUT_KEY_Left,
  INPUT_KEY_Right,

  INPUT_KEY_Insert,
  INPUT_KEY_Delete,
  INPUT_KEY_Home,
  INPUT_KEY_End,
  INPUT_KEY_Page_Up,
  INPUT_KEY_Page_Down,

  INPUT_KEY_Caps_Lock,

  INPUT_KEY_F1,
  INPUT_KEY_F2,
  INPUT_KEY_F3,
  INPUT_KEY_F4,
  INPUT_KEY_F5,
  INPUT_KEY_F6,
  INPUT_KEY_F7,
  INPUT_KEY_F8,
  INPUT_KEY_F9,
  INPUT_KEY_F10,
  INPUT_KEY_F11,
  INPUT_KEY_F12,

  INPUT_KEY_Shift_L = 0x1000,
  INPUT_KEY_Shift_R,
  INPUT_KEY_Control_L,
  INPUT_KEY_Control_R,
  INPUT_KEY_Alt_L,
  INPUT_KEY_Alt_R,
  INPUT_KEY_Meta_L,
  INPUT_KEY_Meta_R,
  INPUT_KEY_Super_L,
  INPUT_KEY_Super_R,
  INPUT_KEY_Hyper_L,
  INPUT_KEY_Hyper_R,
  INPUT_KEY_Mode_switch,

  INPUT_JOYSTICK_UP = 0x1100,
  INPUT_JOYSTICK_DOWN,
  INPUT_JOYSTICK_LEFT,
  INPUT_JOYSTICK_RIGHT,

  INPUT_JOYSTICK_FIRE_1,
  INPUT_JOYSTICK_FIRE_2,
  INPUT_JOYSTICK_FIRE_3,
  INPUT_JOYSTICK_FIRE_4,
  INPUT_JOYSTICK_FIRE_5,
  INPUT_JOYSTICK_FIRE_6,
  INPUT_JOYSTICK_FIRE_7,
  INPUT_JOYSTICK_FIRE_8,
  INPUT_JOYSTICK_FIRE_9,
  INPUT_JOYSTICK_FIRE_10,
  INPUT_JOYSTICK_FIRE_11,
  INPUT_JOYSTICK_FIRE_12,
  INPUT_JOYSTICK_FIRE_13,
  INPUT_JOYSTICK_FIRE_14,
  INPUT_JOYSTICK_FIRE_15,

} input_key;

typedef struct input_event_key_t {

  input_key native_key;
  input_key spectrum_key;

} input_event_key_t;

typedef struct input_event_joystick_t {

  int which;
  input_key button;

} input_event_joystick_t;

typedef struct input_event_t {

  input_event_type type;

  union {

    input_event_key_t key;
    input_event_joystick_t joystick;

  } types;

} input_event_t;

int input_event( const input_event_t *event );

#endif			/* #ifndef FUSE_INPUT_H */
