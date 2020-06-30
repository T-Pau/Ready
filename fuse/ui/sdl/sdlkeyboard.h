/* sdlkeyboard.h: routines for dealing with the SDL keyboard
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier
   Copyright (c) 2005 Fredrick Meunier

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

#ifndef FUSE_SDLKEYBOARD_H
#define FUSE_SDLKEYBOARD_H

void sdlkeyboard_init(void);
void sdlkeyboard_end(void);
void sdlkeyboard_keypress(SDL_KeyboardEvent *keyevent);
void sdlkeyboard_keyrelease(SDL_KeyboardEvent *keyevent);

#endif			/* #ifndef FUSE_SDLKEYBOARD_H */
