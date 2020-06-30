/* keyboard.h: Routines for dealing with the Spectrum's keyboard
   Copyright (c) 1999-2016 Philip Kendall

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

#ifndef FUSE_KEYBOARD_H
#define FUSE_KEYBOARD_H

#include <libspectrum.h>

#include "input.h"

extern libspectrum_byte keyboard_default_value;
extern libspectrum_byte keyboard_return_values[8];

/* A numeric identifier for each Spectrum key. Chosen to map to ASCII in
   most cases */
typedef enum keyboard_key_name {

  KEYBOARD_NONE = 0x00,		/* No key */

  KEYBOARD_space = 0x20,

  KEYBOARD_0 = 0x30,
  KEYBOARD_1,
  KEYBOARD_2,
  KEYBOARD_3,
  KEYBOARD_4,
  KEYBOARD_5,
  KEYBOARD_6,
  KEYBOARD_7,
  KEYBOARD_8,
  KEYBOARD_9,

  KEYBOARD_a = 0x61,
  KEYBOARD_b,
  KEYBOARD_c,
  KEYBOARD_d,
  KEYBOARD_e,
  KEYBOARD_f,
  KEYBOARD_g,
  KEYBOARD_h,
  KEYBOARD_i,
  KEYBOARD_j,
  KEYBOARD_k,
  KEYBOARD_l,
  KEYBOARD_m,
  KEYBOARD_n,
  KEYBOARD_o,
  KEYBOARD_p,
  KEYBOARD_q,
  KEYBOARD_r,
  KEYBOARD_s,
  KEYBOARD_t,
  KEYBOARD_u,
  KEYBOARD_v,
  KEYBOARD_w,
  KEYBOARD_x,
  KEYBOARD_y,
  KEYBOARD_z,

  KEYBOARD_Enter = 0x100,
  KEYBOARD_Caps,
  KEYBOARD_Symbol,

  /* Used by the configuration code to signify that a real joystick fire
     button should map to the emulated joystick fire button */

  KEYBOARD_JOYSTICK_FIRE = 0x1000,

} keyboard_key_name;

void keyboard_register_startup( void );
libspectrum_byte keyboard_read( libspectrum_byte porth );
void keyboard_press(keyboard_key_name key);
void keyboard_release(keyboard_key_name key);
int keyboard_release_all( void );

/* Which Spectrum keys should be emulated as pressed when each input
   layer key is pressed */

typedef struct keyboard_spectrum_keys_t {

  keyboard_key_name key1,key2;

} keyboard_spectrum_keys_t;

const keyboard_spectrum_keys_t* keyboard_get_spectrum_keys( input_key keysym );

/* The mapping from UI layer keysyms to Fuse input layer keysyms */

typedef struct keysyms_map_t {

  /* FIXME: this should really be the UI specific type used for keysyms */
  libspectrum_dword ui;

  input_key fuse;

} keysyms_map_t;

extern keysyms_map_t keysyms_map[];

input_key keysyms_remap( libspectrum_dword ui_keysym );

const char* keyboard_key_text( keyboard_key_name key );

/* Simulate what would be returned by the ULA if it is read from with
   the high byte of the port as "porth" and "key" is pressed */
libspectrum_byte
keyboard_simulate_keypress( libspectrum_byte porth, keyboard_key_name key );

#endif			/* #ifndef FUSE_KEYBOARD_H */
