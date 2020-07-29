/*
 uimon.c -- Monitor UI (Stubs)
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

#include <stdlib.h>
#include <string.h>

#include "uimon.h"


int console_close_all(void) {
    return 0;
}


int console_init(void) {
    return 0;
}


char *uimon_get_in(char **ppchCommandLine, const char *prompt) {
    return strdup("x");
}



void uimon_notify_change(void) {
}

int uimon_out(const char *buffer) {
    return 0;
}


void uimon_set_interface(struct monitor_interface_s **interf, int i) {
}


void uimon_window_close(void) {
}

struct console_s *uimon_window_open(void) {
    return NULL;
}

struct console_s *uimon_window_resume(void) {
    return NULL;
}

void uimon_window_suspend(void) {
}
