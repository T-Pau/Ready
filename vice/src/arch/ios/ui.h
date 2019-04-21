/*
 ui.h -- UI Prototypes
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

#ifndef ui_h
#define ui_h

#include "vice.h"


#include "videoarch.h"
#include "palette.h"

void ui_set_handle_dropped_files_func(int (*func)(const char *));
void ui_set_create_window_func(void (*func)(video_canvas_t *));
void ui_set_identify_canvas_func(int (*func)(video_canvas_t *));

void ui_create_main_window(video_canvas_t *canvas);
void ui_display_main_window(int index);
void ui_destroy_main_window(int index);

void ui_dispatch_events(void);
void ui_exit(void);
void ui_show_text(const char *title, const char *text, int width, int height);

void ui_display_paused(int flag);
void ui_pause_emulation(int flag);
int  ui_emulation_is_paused(void);

#endif /* ui_h */
