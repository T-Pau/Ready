## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2003 Philip Kendall
## Copyright (c) 2015-2016 Sergio Baldoví

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

fuse_SOURCES += \
                z80/z80.c \
                z80/z80_debugger_variables.c \
                z80/z80_ops.c

BUILT_SOURCES += \
                 z80/opcodes_base.c \
                 z80/z80_cb.c \
                 z80/z80_ddfd.c \
                 z80/z80_ddfdcb.c \
                 z80/z80_ed.c

z80/opcodes_base.c: $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_base.dat
	@$(MKDIR_P) z80
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_base.dat > $@.tmp && mv $@.tmp $@

z80/z80_cb.c: $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_cb.dat
	@$(MKDIR_P) z80
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_cb.dat > $@.tmp && mv $@.tmp $@

z80/z80_ddfd.c: $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ddfd.dat
	@$(MKDIR_P) z80
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ddfd.dat > $@.tmp && mv $@.tmp $@

z80/z80_ddfdcb.c: $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ddfd.dat
	@$(MKDIR_P) z80
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ddfdcb.dat > $@.tmp && mv $@.tmp $@

z80/z80_ed.c: $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ed.dat
	@$(MKDIR_P) z80
	$(AM_V_GEN)$(PERL) -I$(srcdir)/perl $(srcdir)/z80/z80.pl $(srcdir)/z80/opcodes_ed.dat > $@.tmp && mv $@.tmp $@

noinst_HEADERS += \
                  z80/z80.h \
                  z80/z80_checks.h \
                  z80/z80_internals.h \
                  z80/z80_macros.h

EXTRA_DIST += \
              z80/tests/README \
              z80/tests/tests.expected \
              z80/tests/tests.in \
              z80/opcodes_base.c \
              z80/opcodes_base.dat \
              z80/opcodes_cb.dat \
              z80/opcodes_ddfd.dat \
              z80/opcodes_ddfdcb.dat \
              z80/opcodes_ed.dat \
              z80/z80.pl \
              z80/z80_cb.c \
              z80/z80_ddfd.c \
              z80/z80_ddfdcb.c \
              z80/z80_ed.c

## The core tester

noinst_PROGRAMS += z80/coretest

z80_coretest_SOURCES = z80/coretest.c z80/z80.c
z80_coretest_LDADD = z80/z80_coretest.o $(GLIB_LIBS) $(LIBSPECTRUM_LIBS)
z80_coretest_CPPFLAGS = $(GLIB_CFLAGS) $(LIBSPECTRUM_CFLAGS) -DCORETEST

z80/coretest.o: $(srcdir)/z80/coretest.c
	$(AM_V_CC)$(COMPILE) -DCORETEST -c $(srcdir)/z80/coretest.c -o $@

z80/z80_coretest.o: z80/z80_ops.c
	$(AM_V_CC)$(COMPILE) -DCORETEST -c $(srcdir)/z80/z80_ops.c -o $@

test: z80/coretest
	z80/coretest $(srcdir)/z80/tests/tests.in > z80/tests.actual
	cmp z80/tests.actual $(srcdir)/z80/tests/tests.expected

CLEANFILES += \
              z80/opcodes_base.c \
              z80/tests.actual \
              z80/z80_cb.c \
              z80/z80_coretest.o \
              z80/z80_ddfd.c \
              z80/z80_ddfdcb.c \
              z80/z80_ed.c
