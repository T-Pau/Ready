/*
 vice_c64.h -- import C definition to Swift
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

#ifndef HAD_VICE_C64_H
#define HAD_VICE_C64_H

#import "ViceThread.h"

#define AUTOSTART_PRG_MODE_INJECT   1

// datasette.h
#define DATASETTE_CONTROL_STOP    0
#define DATASETTE_CONTROL_START   1
#define DATASETTE_CONTROL_FORWARD 2
#define DATASETTE_CONTROL_REWIND  3
#define DATASETTE_CONTROL_RECORD  4
#define DATASETTE_CONTROL_RESET   5
#define DATASETTE_CONTROL_RESET_COUNTER   6

/* Drive idling methods.  */
#define DRIVE_IDLE_NO_IDLE     0
#define DRIVE_IDLE_SKIP_CYCLES 1
#define DRIVE_IDLE_TRAP_IDLE   2

/* values to be used with IDE64Version resource */
#define IDE64_VERSION_3 0
#define IDE64_VERSION_4_1 1
#define IDE64_VERSION_4_2 2

#define LP_HOST_BUTTON_1    1
#define LP_HOST_BUTTON_2    4

#define MACHINE_RESET_MODE_SOFT 0
#define MACHINE_RESET_MODE_HARD 1

#define SID_MODEL_6581           0
#define SID_MODEL_8580           1

#define VICII_NORMAL_BORDERS 0
#define VICII_FULL_BORDERS   1
#define VICII_DEBUG_BORDERS  2
#define VICII_NO_BORDERS     3
#define VICII_TALL_BORDERS   4

extern int maincpu_running;

extern void cartridge_trigger_freeze(void);

extern void c64model_set(int model);

extern int file_system_attach_disk(unsigned int unit, const char *filename);

void joystick_set_value_absolute(unsigned int joyport, uint8_t value);
void joystick_set_value_or(unsigned int joyport, uint8_t value);
void joystick_set_value_and(unsigned int joyport, uint8_t value);

extern void machine_trigger_reset(const unsigned int reset_mode);

extern void mem_inject(uint32_t addr, uint8_t value);

void mouse_button_press(int button, int pressed);

extern int resources_set_int(const char *name, int value);
extern int resources_set_string(const char *name, const char *value);

void keyboard_restore_pressed(void);
void keyboard_restore_released(void);

extern unsigned long vsyncarch_gettime(void);

void update_light_pen(int x, int y, int width, int height, int button_1, int button_2, int is_koala_pad);

const char *drive_get_status(int unit);
void drive_set_id(int unit, uint8_t id1, uint8_t id2);

#endif /* HAD_VICE_C64_H */
