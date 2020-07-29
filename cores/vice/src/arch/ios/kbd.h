#ifndef kbd_h
#define kbd_h

/*
 kdb.h -- Keyboard Interface
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

void kbd_arch_init(void);
void kbd_arch_shutdown(void);
int kbd_arch_get_host_mapping(void);
void kbd_initialize_numpad_joykeys(int *joykeys);
void kbd_connect_handlers(void *xxx, void *data);

#define KBD_PORT_PREFIX "ios"

/* add more function prototypes as needed below */

signed long kbd_arch_keyname_to_keynum(char *keyname);
const char *kbd_arch_keynum_to_keyname(signed long keynum);

#endif /* kbd_h */
