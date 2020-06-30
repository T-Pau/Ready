## Process this file with automake to produce Makefile.in
## Copyright (c) 2001,2002 Philip Kendall
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

if USE_WIDGET

fuse_SOURCES += $(ui_widget_files)

BUILT_SOURCES += $(ui_widget_built)

pkgdata_DATA += ui/widget/fuse.font

endif


CLEANFILES += $(ui_widget_built)

ui_widget_files = \
                  ui/widget/about.c \
                  ui/widget/browse.c \
                  ui/widget/debugger.c \
                  ui/widget/error.c \
                  ui/widget/filesel.c \
                  ui/widget/memory.c \
                  ui/widget/menu.c \
                  ui/widget/menu_data.c \
                  ui/widget/options.c \
                  ui/widget/picture.c \
                  ui/widget/pokefinder.c \
                  ui/widget/pokemem.c \
                  ui/widget/query.c \
                  ui/widget/roms.c \
                  ui/widget/select.c \
                  ui/widget/text.c \
                  ui/widget/widget_internals.h \
                  ui/widget/widget.c \
                  ui/widget/widget.h

ui_widget_built = \
                  ui/widget/fuse.font \
                  ui/widget/menu_data.c \
                  ui/widget/options.c \
                  ui/widget/options_internals.h

EXTRA_DIST += \
              ui/widget/fuse.font.sbf \
              ui/widget/mkfusefont.pl \
              ui/widget/options.pl \
              ui/widget/options-header.pl

ui/widget/menu_data.c: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.dat $(srcdir)/menu_data.pl config.h
	@$(MKDIR_P) ui/widget
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl widget > $@.tmp && mv $@.tmp $@

ui/widget/options.c: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/widget/options.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/widget
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/widget/options.pl - > $@.tmp && mv $@.tmp $@

ui/widget/options_internals.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/widget/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/widget
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/widget/options-header.pl - > $@.tmp && mv $@.tmp $@

ui/widget/fuse.font: $(srcdir)/ui/widget/mkfusefont.pl $(srcdir)/ui/widget/fuse.font.sbf
	@$(MKDIR_P) ui/widget
	$(AM_V_GEN)$(PERL) $(srcdir)/ui/widget/mkfusefont.pl $(srcdir)/ui/widget/fuse.font.sbf > $@.tmp && mv $@.tmp $@
