/*
 mousedrv.m -- Mosue Driver
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

#include "mousedrv.h"

#include "ViceThreadC.h"

static mouse_func_t *mouse_funcs;

void mouse_button_press(int button, int pressed) {
    switch (button) {
        case 1:
            mouse_funcs->mbl(pressed);
            break;
            
        case 2:
            mouse_funcs->mbr(pressed);
            break;

        default:
            break;
    }
}

/** \brief Register and parse mouse-related command-line options.
 *  \return Zero on success, nonzero on failure. */
int mousedrv_cmdline_options_init(void) {
    return 0;
}


/** \brief Returns the last time the mouse position changed.
 *
 *  \note A button press or release is not a change.
 *
 *  \return The current X value, in the range 0-65536.
 */
unsigned long mousedrv_get_timestamp(void) {
    return viceThread.mouseTimestamp;
}

/** \brief Returns the current mouse X value.
 *
 *  This is a running total of mouse movements and does not
 *  necessarily correspond to any particular screen position.
 *
 *  \return The current X value, in the range 0-65536.
 */
int mousedrv_get_x(void) {
    return viceThread.mouseX;
}


/** \brief Returns the current mouse Y value.
 *
 *  This is a running total of mouse movements and does not
 *  necessarily correspond to any particular screen position.
 *
 *  \return The current Y value, in the range 0-65536.
 */
int mousedrv_get_y(void) {
    return viceThread.mouseY;
}


/** \brief Called by the emulation core to announce the mouse has been
 *         enabled or disabled. */
void mousedrv_mouse_changed(void) {
}


/** \brief Initialize the mouse-handling subsystem. */
void mousedrv_init(void) {
}


/** \brief Register callbacks for mouse button presses.
 *  \param funcs The callbacks to register.
 *  \return Zero on success, nonzero on failure. */
int mousedrv_resources_init(mouse_func_t *funcs) {
    mouse_funcs = funcs;
    return 0;
}


