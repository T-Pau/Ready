/*
 kbd.c -- Keyboard Interface (Stubs)
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

#include "kbd.h"

#include <stdlib.h>


int kbd_arch_get_host_mapping(void) {
    return 0;
}


/** \brief  Initialize keyboard handling
 */
void kbd_arch_init(void)
{
}


signed long kbd_arch_keyname_to_keynum(char *keyname) {
    return 0;
}


const char *kbd_arch_keynum_to_keyname(signed long keynum) {
    return NULL;
}


void kbd_initialize_numpad_joykeys(int *joykeys) {
}
