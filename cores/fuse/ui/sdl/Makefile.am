## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2004 Philip Kendall
## Copyright (c) 2015 Sergio Baldoví

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

if UI_SDL

fuse_SOURCES += $(ui_sdl_files)

BUILT_SOURCES += $(ui_sdl_built)

endif


CLEANFILES += $(ui_sdl_built)

ui_sdl_files = \
               ui/sdl/sdldisplay.c \
               ui/sdl/sdldisplay.h \
               ui/sdl/sdljoystick.c \
               ui/sdl/sdljoystick.h \
               ui/sdl/sdlkeyboard.c \
               ui/sdl/sdlkeyboard.h \
               ui/sdl/sdlui.c \
               ui/sdl/keysyms.c

ui_sdl_built = ui/sdl/keysyms.c

ui/sdl/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/sdl
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl sdl $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@
