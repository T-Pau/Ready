/* uijoystick.h: Joystick emulation support
   Copyright (c) 2001-2003 Russell Marks, Philip Kendall, Darren Salt,
			   Fredrick Meunier
   Copyright (c) 2015 UB880D
   Copyright (c) 2015 Sergio Baldoví

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

/* ui_joystick_* are called from joystick_*.
 * Do not use these directly; use the joystick_* wrappers.
 * UI-specific implementations are in ui/<ui>/<ui>joystick.c.
 * (If the UI cannot provide its own, it must use the default implementation;
 *  see uijoystick.c for details.)
 */

#ifndef FUSE_UI_UIJOYSTICK_H
#define FUSE_UI_UIJOYSTICK_H

#define NUM_JOY_BUTTONS 15

int ui_joystick_init( void ); /* returns no. of joysticks initialised */
void ui_joystick_end( void );

/* Poll the joysticks for any changes */
void ui_joystick_poll( void );

#endif			/* #ifndef FUSE_UI_UIJOYSTICK_H */
