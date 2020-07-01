/* iossound.c: Sound Routines for Integration with Readh
   Copyright (c) 2020 Dieter Baron

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

*/

#include <config.h>

#include "settings.h"
#include "sound.h"
#include "sfifo.h"
#include "ui/ui.h"

sfifo_t sound_fifo;

int sound_lowlevel_init(const char *device, int *freqptr, int *stereoptr) {
	return 0;
}

void sound_lowlevel_end(void) {
}

void sound_lowlevel_frame(libspectrum_signed_word *data, int len) {
}
