/*
 vice_vic20.c -- VIC-20 Specifics
 Copyright (C) 2020 Dieter Baron
 
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

#include <stdio.h>

// TODO: include correct file?
extern void vic20model_set(int model);

void model_set(int model) {
    vic20model_set(model);
}

void cartridge_trigger_freeze(void) {
}

uint32_t palette[] = {
    0x000000ff,
    0xffffffff,
    0x6d2327ff,
    0xa0fef8ff,
    0x8e3c97ff,
    0x7eda75ff,
    0x252390ff,
    0xffff86ff,
    0xa4643bff,
    0xffc8a1ff,
    0xf2a7abff,
    0xdbffffff,
    0xffb4ffff,
    0xd7ffceff,
    0x9d9affff,
    0xffffc9ff
};
