## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2008 Philip Kendall
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

if UI_WIN32

fuse_SOURCES += $(ui_win32_files)

BUILT_SOURCES += $(ui_win32_built)

ui_win32_res = $(ui_win32_rc_files)

endif


CLEANFILES += $(ui_win32_built)

ui_win32_files = \
                 ui/win32/about.c \
                 ui/win32/about.h \
                 ui/win32/binary.c \
                 ui/win32/binary.h \
                 ui/win32/browse.c \
                 ui/win32/browse.h \
                 ui/win32/confirm.c \
                 ui/win32/debugger.c \
                 ui/win32/debugger.h \
                 ui/win32/fileselector.c \
                 ui/win32/hyperlinks.c \
                 ui/win32/hyperlinks.h \
                 ui/win32/keysyms.c \
                 ui/win32/memorybrowser.c \
                 ui/win32/memorybrowser.h \
                 ui/win32/menu_data.c \
                 ui/win32/options.c \
                 ui/win32/picture.c \
                 ui/win32/picture.h \
                 ui/win32/pokefinder.c \
                 ui/win32/pokefinder.h \
                 ui/win32/pokemem.c \
                 ui/win32/pokemem.h \
                 ui/win32/rollback.c \
                 ui/win32/rollback.h \
                 ui/win32/roms.c \
                 ui/win32/roms.h \
                 ui/win32/select_template.h \
                 ui/win32/statusbar.c \
                 ui/win32/win32display.c \
                 ui/win32/win32internals.h \
                 ui/win32/win32joystick.c \
                 ui/win32/win32joystick.h \
                 ui/win32/win32keyboard.c \
                 ui/win32/win32mouse.c \
                 ui/win32/win32ui.c

ui_win32_rc_files = \
                 ui/win32/about.rc \
                 ui/win32/binary.rc \
                 ui/win32/browse.rc \
                 ui/win32/debugger.rc \
                 ui/win32/memorybrowser.rc \
                 ui/win32/menu_data.rc \
                 ui/win32/options.rc \
                 ui/win32/picture.rc \
                 ui/win32/pokefinder.rc \
                 ui/win32/pokemem.rc \
                 ui/win32/rollback.rc \
                 ui/win32/roms.rc \
                 ui/win32/select_template.rc \
                 ui/win32/statusbar.rc \
                 ui/win32/win32joystick.rc

ui_win32_built = \
                 ui/win32/keysyms.c \
                 ui/win32/menu_data.c \
                 ui/win32/menu_data.h \
                 ui/win32/menu_data.rc \
                 ui/win32/options_internals.h \
                 ui/win32/options.c \
                 ui/win32/options.rc

EXTRA_DIST += \
              ui/win32/icons/disk_active.bmp \
              ui/win32/icons/disk_inactive.bmp \
              ui/win32/icons/mdr_active.bmp \
              ui/win32/icons/mdr_inactive.bmp \
              ui/win32/icons/mouse_active.bmp \
              ui/win32/icons/mouse_inactive.bmp \
              ui/win32/icons/pause_active.bmp \
              ui/win32/icons/pause_inactive.bmp \
              ui/win32/icons/tape_active.bmp \
              ui/win32/icons/tape_inactive.bmp \
              ui/win32/icons/tape_marker.bmp \
              ui/win32/icons/tape_marker_mask.bmp \
              ui/win32/options.pl \
              ui/win32/options-header.pl \
              ui/win32/options-resource.pl \
              $(ui_win32_rc_files)

ui/win32/keysyms.c: $(srcdir)/keysyms.pl $(srcdir)/keysyms.dat
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/keysyms.pl win32 $(srcdir)/keysyms.dat > $@.tmp && mv $@.tmp $@

ui/win32/menu_data.c: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.pl $(srcdir)/menu_data.dat config.h
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl win32 c > $@.tmp && mv $@.tmp $@

ui/win32/menu_data.h: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.pl $(srcdir)/menu_data.dat config.h
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl win32 h > $@.tmp && mv $@.tmp $@

ui/win32/menu_data.rc: $(srcdir)/perl/cpp-perl.pl $(srcdir)/menu_data.pl $(srcdir)/menu_data.dat config.h
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/menu_data.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/menu_data.pl win32 rc > $@.tmp && mv $@.tmp $@

ui/win32/options.c: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/win32/options.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/win32/options.pl - > $@.tmp && mv $@.tmp $@

ui/win32/options_internals.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/win32/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/win32/options-header.pl - > $@.tmp && mv $@.tmp $@

ui/win32/options.rc: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/win32/options-resource.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	@$(MKDIR_P) ui/win32
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/win32/options-resource.pl - > $@.tmp && mv $@.tmp $@
