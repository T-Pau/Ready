## Process this file with automake to produce Makefile.in
## Copyright (c) 1999-2015 Philip Kendall

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

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = fuse

noinst_PROGRAMS =

fuse_SOURCES = display.c \
	event.c \
	fuse.c \
	input.c \
	keyboard.c \
	loader.c \
	machine.c \
	memory_pages.c \
	mempool.c \
	menu.c \
	movie.c \
	module.c \
	periph.c \
	phantom_typist.c \
	profile.c \
	psg.c \
	rectangle.c \
	rzx.c \
	screenshot.c \
	settings.c \
	slt.c \
	snapshot.c \
	sound.c \
	spectrum.c \
	svg.c \
	tape.c \
	ui.c \
	uidisplay.c \
	uimedia.c \
	utils.c

fuse_LDADD = \
             $(PTHREAD_LIBS) \
             $(LIBSPECTRUM_LIBS) \
             $(GTK_LIBS) \
             $(GLIB_LIBS) \
             $(PNG_LIBS) \
             $(SDL_LIBS) \
             $(X_LIBS) \
             $(XML_LIBS)

fuse_DEPENDENCIES =

EXTRA_fuse_SOURCES =

BUILT_SOURCES = options.h settings.c settings.h

settings.c: settings.pl settings.dat
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/settings.pl $(srcdir)/settings.dat > $@.tmp && mv $@.tmp $@

settings.h: settings-header.pl settings.dat
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/settings-header.pl $(srcdir)/settings.dat > $@.tmp && mv $@.tmp $@

options.h: $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/@OPTIONS_DIR@/options-header.pl $(srcdir)/ui/options.dat $(srcdir)/perl/Fuse.pm $(srcdir)/perl/Fuse/Dialog.pm
	$(AM_V_GEN)$(PERL) $(srcdir)/perl/cpp-perl.pl config.h $(srcdir)/ui/options.dat | $(PERL) -I$(srcdir)/perl $(srcdir)/ui/@OPTIONS_DIR@/options-header.pl - public > $@.tmp && mv $@.tmp $@

AM_CPPFLAGS = \
              $(GLIB_CFLAGS) \
              $(GTK_CFLAGS) \
              $(LIBSPECTRUM_CFLAGS) \
              $(XML_CFLAGS) \
              -DFUSEDATADIR="\"${pkgdatadir}\"" \
              $(SDL_CFLAGS) \
              $(PNG_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS) $(PTHREAD_CFLAGS)

noinst_HEADERS = bitmap.h \
	compat.h \
	display.h \
	event.h \
	fuse.h \
	input.h \
	keyboard.h \
	loader.h \
	machine.h \
	memory_pages.h \
	mempool.h \
	menu.h \
	movie.h \
	movie_tables.h \
	module.h \
	periph.h \
	phantom_typist.h \
	psg.h \
	rectangle.h \
	rzx.h \
	screenshot.h \
	settings.h \
	slt.h \
	snapshot.h \
	sound.h \
	spectrum.h \
	svg.h \
	tape.h \
	utils.h \
	options.h \
	profile.h

EXTRA_DIST = AUTHORS \
	     INSTALL \
	     PORTING \
	     README \
	     THANKS \
	     keysyms.dat \
	     keysyms.pl \
	     m4/ax_create_stdint_h.m4 \
	     m4/ax_pthread.m4 \
	     m4/ax_string_strcasecmp.m4 \
	     m4/gtk-2.0.m4 \
	     m4/pkg.m4 \
	     m4/sdl.m4 \
	     menu_data.dat \
	     menu_data.pl \
	     settings.dat \
	     settings.pl \
	     settings-header.pl

CLEANFILES = options.h \
	     settings.c \
	     settings.h

DISTCLEANFILES =

pkgdata_DATA =


## Resources for Windows executables
if COMPAT_WIN32

fuse_SOURCES += windres.rc
fuse_LDADD += windres.o
fuse_DEPENDENCIES += windres.o

windres.o: windres.rc data/win32/winfuse.ico data/win32/fuse.manifest $(ui_win32_res)
	$(AM_V_GEN)$(WINDRES) -I$(srcdir) -I. $(srcdir)/windres.rc $(LIBSPECTRUM_CFLAGS) $(CPPFLAGS) windres.o

endif


include compat/Makefile.am
include data/Makefile.am
include debugger/Makefile.am
include hacking/Makefile.am
include infrastructure/Makefile.am
include lib/Makefile.am
include machines/Makefile.am
include man/Makefile.am
include peripherals/Makefile.am
include perl/Makefile.am
include pokefinder/Makefile.am
include roms/Makefile.am
include sound/Makefile.am
include timer/Makefile.am
include ui/Makefile.am
include ui/fb/Makefile.am
include ui/gtk/Makefile.am
include ui/null/Makefile.am
include ui/scaler/Makefile.am
include ui/sdl/Makefile.am
include ui/svga/Makefile.am
include ui/widget/Makefile.am
include ui/wii/Makefile.am
include ui/win32/Makefile.am
include ui/xlib/Makefile.am
include unittests/Makefile.am
include z80/Makefile.am
