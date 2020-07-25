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

if UI_GTK

fuse_SOURCES += $(ui_gtk_files)

BUILT_SOURCES += $(ui_gtk_built)

pkgdata_DATA += ui/gtk/menu_data.ui

endif


CLEANFILES += $(ui_gtk_built)

ui_gtk_files = \
               ui/gtk/binary.c \
               ui/gtk/browse.c \
               ui/gtk/confirm.c \
               ui/gtk/debugger.c \
               ui/gtk/fileselector.c \
               ui/gtk/gtkcompat.h \
               ui/gtk/gtkdisplay.c \
               ui/gtk/gtkinternals.h \
               ui/gtk/gtkjoystick.c \
               ui/gtk/gtkkeyboard.c \
               ui/gtk/gtkmouse.c \
               ui/gtk/gtkui.c \
               ui/gtk/keysyms.c \
               ui/gtk/menu_data.c \
               ui/gtk/options.c \
               ui/gtk/picture.c \
               ui/gtk/pixmaps.c \
               ui/gtk/pokefinder.c \
               ui/gtk/pokemem.c \
               ui/gtk/rollback.c \
               ui/gtk/roms.c \
               ui/gtk/statusbar.c \
               ui/gtk/stock.c

if HAVE_GTK2

ui_gtk_files += ui/gtk/gtkcompat.c \
                ui/gtk/memory_gtk2.c

else

ui_gtk_files += ui/gtk/memory.c

endif

ui_gtk_built = \
               ui/gtk/keysyms.c \
               ui/gtk/menu_data.c \
               ui/gtk/menu_data.ui \
               ui/gtk/options.c \
               ui/gtk/options_internals.h

EXTRA_DIST += \
              ui/gtk/options.pl \
              ui/gtk/options-header.pl

ui/gtk/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/gtk
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl gtk $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@

ui/gtk/menu_data.c: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.pl $(srcdir)/menu_data.dat config.h
	@$(MKDIR_P) ui/gtk
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl gtk c > $@.tmp && mv $@.tmp $@

ui/gtk/menu_data.ui: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.pl $(srcdir)/menu_data.dat config.h
	@$(MKDIR_P) ui/gtk
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl gtk ui > $@.tmp && mv $@.tmp $@

ui/gtk/options.c: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/gtk/options.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/gtk
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/gtk/options.pl - > $@.tmp && mv $@.tmp $@

ui/gtk/options_internals.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/gtk/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/gtk
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/gtk/options-header.pl - > $@.tmp && mv $@.tmp $@
