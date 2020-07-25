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

if UI_SVGA

fuse_SOURCES += $(ui_svga_files)

BUILT_SOURCES += $(ui_svga_built)

endif


CLEANFILES += $(ui_svga_built)

ui_svga_files = \
                ui/svga/keysyms.c \
                ui/svga/svgadisplay.c \
                ui/svga/svgadisplay.h \
                ui/svga/svgajoystick.c \
                ui/svga/svgakeyboard.c \
                ui/svga/svgakeyboard.h \
                ui/svga/svgaui.c

ui_svga_built = ui/svga/keysyms.c

ui/svga/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/svga
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl svga $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@
