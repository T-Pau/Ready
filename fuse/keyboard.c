/* keyboard.c: Routines for dealing with the Spectrum's keyboard
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

#include <config.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "infrastructure/startup_manager.h"
#include "keyboard.h"
#include "ui/ui.h"

/* Bit masks for each of the eight keyboard half-rows; `AND' the selected
   ones of these to get the value to return
*/
libspectrum_byte keyboard_return_values[8];

/* The hash used for storing the UI -> Fuse input layer key mappings */
static GHashTable *keysyms_hash;

struct spectrum_keys_wrapper {
  input_key input;
  keyboard_spectrum_keys_t spectrum;
};

/* Which Spectrum keys should be pressed for each key passed from the
   Fuse input layer */
static struct spectrum_keys_wrapper spectrum_keys_table[] = {

  { INPUT_KEY_Escape,      { KEYBOARD_1,     KEYBOARD_Caps   } },
  { INPUT_KEY_1,           { KEYBOARD_1,     KEYBOARD_NONE   } },
  { INPUT_KEY_2,           { KEYBOARD_2,     KEYBOARD_NONE   } },
  { INPUT_KEY_3,           { KEYBOARD_3,     KEYBOARD_NONE   } },
  { INPUT_KEY_4,           { KEYBOARD_4,     KEYBOARD_NONE   } },
  { INPUT_KEY_5,           { KEYBOARD_5,     KEYBOARD_NONE   } },
  { INPUT_KEY_6,           { KEYBOARD_6,     KEYBOARD_NONE   } },
  { INPUT_KEY_7,           { KEYBOARD_7,     KEYBOARD_NONE   } },
  { INPUT_KEY_8,           { KEYBOARD_8,     KEYBOARD_NONE   } },
  { INPUT_KEY_9,           { KEYBOARD_9,     KEYBOARD_NONE   } },
  { INPUT_KEY_0,           { KEYBOARD_0,     KEYBOARD_NONE   } },
  { INPUT_KEY_minus,       { KEYBOARD_j,     KEYBOARD_Symbol } },
  { INPUT_KEY_equal,       { KEYBOARD_l,     KEYBOARD_Symbol } },
  { INPUT_KEY_BackSpace,   { KEYBOARD_0,     KEYBOARD_Caps   } },

  { INPUT_KEY_Tab,         { KEYBOARD_Caps,  KEYBOARD_Symbol } },
  { INPUT_KEY_q,           { KEYBOARD_q,     KEYBOARD_NONE   } },
  { INPUT_KEY_w,           { KEYBOARD_w,     KEYBOARD_NONE   } },
  { INPUT_KEY_e,           { KEYBOARD_e,     KEYBOARD_NONE   } },
  { INPUT_KEY_r,           { KEYBOARD_r,     KEYBOARD_NONE   } },
  { INPUT_KEY_t,           { KEYBOARD_t,     KEYBOARD_NONE   } },
  { INPUT_KEY_y,           { KEYBOARD_y,     KEYBOARD_NONE   } },
  { INPUT_KEY_u,           { KEYBOARD_u,     KEYBOARD_NONE   } },
  { INPUT_KEY_i,           { KEYBOARD_i,     KEYBOARD_NONE   } },
  { INPUT_KEY_o,           { KEYBOARD_o,     KEYBOARD_NONE   } },
  { INPUT_KEY_p,           { KEYBOARD_p,     KEYBOARD_NONE   } },

  { INPUT_KEY_Caps_Lock,   { KEYBOARD_2,     KEYBOARD_Caps   } },
  { INPUT_KEY_a,           { KEYBOARD_a,     KEYBOARD_NONE   } },
  { INPUT_KEY_s,           { KEYBOARD_s,     KEYBOARD_NONE   } },
  { INPUT_KEY_d,           { KEYBOARD_d,     KEYBOARD_NONE   } },
  { INPUT_KEY_f,           { KEYBOARD_f,     KEYBOARD_NONE   } },
  { INPUT_KEY_g,           { KEYBOARD_g,     KEYBOARD_NONE   } },
  { INPUT_KEY_h,           { KEYBOARD_h,     KEYBOARD_NONE   } },
  { INPUT_KEY_j,           { KEYBOARD_j,     KEYBOARD_NONE   } },
  { INPUT_KEY_k,           { KEYBOARD_k,     KEYBOARD_NONE   } },
  { INPUT_KEY_l,           { KEYBOARD_l,     KEYBOARD_NONE   } },
  { INPUT_KEY_semicolon,   { KEYBOARD_o,     KEYBOARD_Symbol } },
  { INPUT_KEY_apostrophe,  { KEYBOARD_7,     KEYBOARD_Symbol } },
  { INPUT_KEY_numbersign,  { KEYBOARD_3,     KEYBOARD_Symbol } },
  { INPUT_KEY_Return,      { KEYBOARD_Enter, KEYBOARD_NONE   } },

  { INPUT_KEY_Shift_L,     { KEYBOARD_NONE,  KEYBOARD_Caps   } },
  { INPUT_KEY_z,           { KEYBOARD_z,     KEYBOARD_NONE   } },
  { INPUT_KEY_x,           { KEYBOARD_x,     KEYBOARD_NONE   } },
  { INPUT_KEY_c,           { KEYBOARD_c,     KEYBOARD_NONE   } },
  { INPUT_KEY_v,           { KEYBOARD_v,     KEYBOARD_NONE   } },
  { INPUT_KEY_b,           { KEYBOARD_b,     KEYBOARD_NONE   } },
  { INPUT_KEY_n,           { KEYBOARD_n,     KEYBOARD_NONE   } },
  { INPUT_KEY_m,           { KEYBOARD_m,     KEYBOARD_NONE   } },
  { INPUT_KEY_comma,       { KEYBOARD_n,     KEYBOARD_Symbol } },
  { INPUT_KEY_period,      { KEYBOARD_m,     KEYBOARD_Symbol } },
  { INPUT_KEY_slash,       { KEYBOARD_v,     KEYBOARD_Symbol } },
  { INPUT_KEY_Shift_R,     { KEYBOARD_NONE,  KEYBOARD_Caps   } },

  { INPUT_KEY_asterisk,    { KEYBOARD_b,     KEYBOARD_Symbol } },
  { INPUT_KEY_dollar,      { KEYBOARD_4,     KEYBOARD_Symbol } },
  { INPUT_KEY_exclam,      { KEYBOARD_1,     KEYBOARD_Symbol } },
  { INPUT_KEY_less,        { KEYBOARD_r,     KEYBOARD_Symbol } },
  { INPUT_KEY_parenright,  { KEYBOARD_9,     KEYBOARD_Symbol } },
  { INPUT_KEY_colon,       { KEYBOARD_z,     KEYBOARD_Symbol } },
  { INPUT_KEY_plus,        { KEYBOARD_k,     KEYBOARD_Symbol } },

  { INPUT_KEY_Control_L,   { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Alt_L,       { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Meta_L,      { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Super_L,     { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Hyper_L,     { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_space,       { KEYBOARD_space, KEYBOARD_NONE   } },
  { INPUT_KEY_Hyper_R,     { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Super_R,     { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Meta_R,      { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Alt_R,       { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Control_R,   { KEYBOARD_NONE,  KEYBOARD_Symbol } },
  { INPUT_KEY_Mode_switch, { KEYBOARD_NONE,  KEYBOARD_Symbol } },

  { INPUT_KEY_Left,        { KEYBOARD_5,     KEYBOARD_NONE   } },
  { INPUT_KEY_Down,        { KEYBOARD_6,     KEYBOARD_NONE   } },
  { INPUT_KEY_Up,          { KEYBOARD_7,     KEYBOARD_NONE   } },
  { INPUT_KEY_Right,       { KEYBOARD_8,     KEYBOARD_NONE   } },

  { INPUT_KEY_KP_Enter,    { KEYBOARD_Enter, KEYBOARD_NONE   } },

  { INPUT_KEY_NONE, { KEYBOARD_NONE, KEYBOARD_NONE } } /* End marker */

};

static GHashTable *spectrum_keys;

/* When each Spectrum key is pressed, twiddle this {port,bit} pair
   in `keyboard_return_values'. 
*/
struct key_bit {
  int port;
  libspectrum_byte bit;
};

struct key_info {
  keyboard_key_name key;
  struct key_bit bit;
};

static struct key_info keyboard_data_table[] = {

  { KEYBOARD_1,      { 3, 0x01 } },
  { KEYBOARD_2,      { 3, 0x02 } },
  { KEYBOARD_3,      { 3, 0x04 } },
  { KEYBOARD_4,      { 3, 0x08 } },
  { KEYBOARD_5,      { 3, 0x10 } },
  { KEYBOARD_6,      { 4, 0x10 } },
  { KEYBOARD_7,      { 4, 0x08 } },
  { KEYBOARD_8,      { 4, 0x04 } },
  { KEYBOARD_9,      { 4, 0x02 } },
  { KEYBOARD_0,      { 4, 0x01 } },

  { KEYBOARD_q,      { 2, 0x01 } },
  { KEYBOARD_w,      { 2, 0x02 } },
  { KEYBOARD_e,      { 2, 0x04 } },
  { KEYBOARD_r,      { 2, 0x08 } },
  { KEYBOARD_t,      { 2, 0x10 } },
  { KEYBOARD_y,      { 5, 0x10 } },
  { KEYBOARD_u,      { 5, 0x08 } },
  { KEYBOARD_i,      { 5, 0x04 } },
  { KEYBOARD_o,      { 5, 0x02 } },
  { KEYBOARD_p,      { 5, 0x01 } },

  { KEYBOARD_a,      { 1, 0x01 } },
  { KEYBOARD_s,      { 1, 0x02 } },
  { KEYBOARD_d,      { 1, 0x04 } },
  { KEYBOARD_f,      { 1, 0x08 } },
  { KEYBOARD_g,      { 1, 0x10 } },
  { KEYBOARD_h,      { 6, 0x10 } },
  { KEYBOARD_j,      { 6, 0x08 } },
  { KEYBOARD_k,      { 6, 0x04 } },
  { KEYBOARD_l,      { 6, 0x02 } },
  { KEYBOARD_Enter,  { 6, 0x01 } },

  { KEYBOARD_Caps,   { 0, 0x01 } },
  { KEYBOARD_z,      { 0, 0x02 } },
  { KEYBOARD_x,      { 0, 0x04 } },
  { KEYBOARD_c,      { 0, 0x08 } },
  { KEYBOARD_v,      { 0, 0x10 } },
  { KEYBOARD_b,      { 7, 0x10 } },
  { KEYBOARD_n,      { 7, 0x08 } },
  { KEYBOARD_m,      { 7, 0x04 } },
  { KEYBOARD_Symbol, { 7, 0x02 } },
  { KEYBOARD_space,  { 7, 0x01 } },

  { KEYBOARD_NONE,   { 0, 0x00 } }	/* End marker */

};

/* The hash used for storing keyboard_data_table */
static GHashTable *keyboard_data;

/* A textual name for each key */
struct key_text_t {
  keyboard_key_name key;
  const char *text;
};

struct key_text_t key_text_table[] = {

  { KEYBOARD_NONE, "Nothing" },

  { KEYBOARD_space, "Space" },

  { KEYBOARD_0, "0" },
  { KEYBOARD_1, "1" },
  { KEYBOARD_2, "2" },
  { KEYBOARD_3, "3" },
  { KEYBOARD_4, "4" },
  { KEYBOARD_5, "5" },
  { KEYBOARD_6, "6" },
  { KEYBOARD_7, "7" },
  { KEYBOARD_8, "8" },
  { KEYBOARD_9, "9" },

  { KEYBOARD_a, "A" },
  { KEYBOARD_b, "B" },
  { KEYBOARD_c, "C" },
  { KEYBOARD_d, "D" },
  { KEYBOARD_e, "E" },
  { KEYBOARD_f, "F" },
  { KEYBOARD_g, "G" },
  { KEYBOARD_h, "H" },
  { KEYBOARD_i, "I" },
  { KEYBOARD_j, "J" },
  { KEYBOARD_k, "K" },
  { KEYBOARD_l, "L" },
  { KEYBOARD_m, "M" },
  { KEYBOARD_n, "N" },
  { KEYBOARD_o, "O" },
  { KEYBOARD_p, "P" },
  { KEYBOARD_q, "Q" },
  { KEYBOARD_r, "R" },
  { KEYBOARD_s, "S" },
  { KEYBOARD_t, "T" },
  { KEYBOARD_u, "U" },
  { KEYBOARD_v, "V" },
  { KEYBOARD_w, "W" },
  { KEYBOARD_x, "X" },
  { KEYBOARD_y, "Y" },
  { KEYBOARD_z, "Z" },

  { KEYBOARD_Enter, "Enter" },
  { KEYBOARD_Caps, "Caps Shift" },
  { KEYBOARD_Symbol, "Symbol Shift" },

  { KEYBOARD_JOYSTICK_FIRE, "Joystick Fire" },

  { KEYBOARD_NONE, NULL },		/* End marker */

};

static GHashTable *key_text;

static int
keyboard_init( void *context )
{
  struct key_info *ptr;
  struct spectrum_keys_wrapper *ptr2;
  keysyms_map_t *ptr3;
  struct key_text_t *ptr4;

  keyboard_release_all();

  keyboard_data = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr = keyboard_data_table; ptr->key != KEYBOARD_NONE; ptr++ )
    g_hash_table_insert( keyboard_data, &( ptr->key ), &( ptr->bit ) );

  spectrum_keys = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr2 = spectrum_keys_table; ptr2->input != INPUT_KEY_NONE; ptr2++ )
    g_hash_table_insert( spectrum_keys,
			 &( ptr2->input ), &( ptr2->spectrum ) );

  keysyms_hash = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr3 = keysyms_map; ptr3->ui; ptr3++ )
    g_hash_table_insert( keysyms_hash, &( ptr3->ui ), &( ptr3->fuse ) );

  key_text = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr4 = key_text_table; ptr4->text != NULL; ptr4++ )
    g_hash_table_insert( key_text, &( ptr4->key ), &( ptr4->text ) );

  return 0;
}

static void
keyboard_end( void )
{
  g_hash_table_destroy( keyboard_data );
  g_hash_table_destroy( spectrum_keys );
  g_hash_table_destroy( keysyms_hash );
  g_hash_table_destroy( key_text );
}

void
keyboard_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_LIBSPECTRUM,
    STARTUP_MANAGER_MODULE_SETUID
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_KEYBOARD, dependencies,
                            ARRAY_SIZE( dependencies ), keyboard_init,
                            keyboard_end, NULL );
}

libspectrum_byte
keyboard_read( libspectrum_byte porth )
{
  libspectrum_byte data = 0xff; int i;

  for( i=0; i<8; i++,porth>>=1 ) {
    if(! (porth&0x01) ) data &= keyboard_return_values[i];
  }

  return data;

}

void
keyboard_press( keyboard_key_name key )
{
  struct key_bit *ptr;

  ptr = g_hash_table_lookup( keyboard_data, &key );

  if( ptr ) keyboard_return_values[ ptr->port ] &= ~( ptr->bit );
}

void
keyboard_release( keyboard_key_name key )
{
  struct key_bit *ptr;

  ptr = g_hash_table_lookup( keyboard_data, &key );

  if( ptr ) keyboard_return_values[ ptr->port ] |= ptr->bit;
}

int keyboard_release_all( void )
{
  int i;

  for( i=0; i<8; i++ ) keyboard_return_values[i] = 0xff;

  return 0;
}

const keyboard_spectrum_keys_t*
keyboard_get_spectrum_keys( input_key keysym )
{
  return g_hash_table_lookup( spectrum_keys, &keysym );
}

input_key
keysyms_remap( libspectrum_dword ui_keysym )
{
  const input_key *ptr;

  ptr = g_hash_table_lookup( keysyms_hash, &ui_keysym );

  return ptr ? *ptr : INPUT_KEY_NONE;
}

const char*
keyboard_key_text( keyboard_key_name key )
{
  const char **ptr;

  ptr = g_hash_table_lookup( key_text, &key );

  return ptr ? *ptr : "[Unknown key]";
}

libspectrum_byte
keyboard_simulate_keypress( libspectrum_byte porth, keyboard_key_name key )
{
  libspectrum_byte r = 0xff;
  struct key_bit *data;

  data = g_hash_table_lookup( keyboard_data, &key );

  if( data ) {
    libspectrum_byte mask = (1 << data->port);
    if( !(porth & mask) ) {
      r &= ~data->bit;
    }
  }

  return r;
}
