/*
 mousedrv.m -- Mosue Driver
 Copyright (C) 2019 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mousedrv.h"

#include "ViceThread.h"

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


