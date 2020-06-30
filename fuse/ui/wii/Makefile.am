## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2009 Philip Kendall, Bjoern Giesler
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
## E-mail: bjoern@giesler.de

if UI_WII

fuse_SOURCES += $(ui_wii_files)

BUILT_SOURCES += $(ui_wii_built)

endif


CLEANFILES += $(ui_wii_built)

ui_wii_files = \
                ui/wii/keysyms.c \
                ui/wii/wiidisplay.c \
                ui/wii/wiidisplay.h \
                ui/wii/wiijoystick.c \
                ui/wii/wiikeyboard.c \
                ui/wii/wiikeyboard.h \
                ui/wii/wiikeysyms.h \
                ui/wii/wiimouse.c \
                ui/wii/wiimouse.h \
                ui/wii/wiiui.c

ui_wii_built = ui/wii/keysyms.c

ui/wii/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/wii
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl wii $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@
