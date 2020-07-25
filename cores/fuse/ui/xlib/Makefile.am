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

if UI_X

fuse_SOURCES += $(ui_xlib_files)

BUILT_SOURCES += $(ui_xlib_built)

endif


EXTRA_fuse_SOURCES += $(ui_xlib_files)

CLEANFILES += $(ui_xlib_built)

ui_xlib_files = \
                ui/xlib/keysyms.c \
                ui/xlib/xdisplay.c \
                ui/xlib/xdisplay.h \
                ui/xlib/xerror.c \
                ui/xlib/xjoystick.c \
                ui/xlib/xkeyboard.c \
                ui/xlib/xkeyboard.h \
                ui/xlib/xui.c \
                ui/xlib/xui.h

ui_xlib_built = \
                 ui/xlib/keysyms.c \
                 ui/xlib/xpixmaps.c

EXTRA_DIST += \
              ui/xlib/xpixmaps.c \
              ui/xlib/xstatusbar.pl

ui/xlib/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/xlib
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl x $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@

ui/xlib/xpixmaps.c: ui/xlib/xstatusbar.pl $(srcdir)/ui/gtk/pixmaps.c
	@$(MKDIR_P) ui/xlib
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/ui/xlib/xstatusbar.pl $(srcdir)/ui/gtk/pixmaps.c > $@.tmp && mv $@.tmp $@
