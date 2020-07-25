## This file does not need automake. Include in the final Makefile.
## Copyright (c) 2013-2016 Sergio Baldoví

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
## E-mail: serbalgi@gmail.com

package_win32=$(PACKAGE)-$(PACKAGE_VERSION)-$(UI)
top_win32dir=$(top_builddir)/$(package_win32)

install-win32: all
	test -n "$(DESTDIR)" || { echo "ERROR: set DESTDIR path"; exit 1; }
	$(MKDIR_P) $(DESTDIR)/roms/ || exit 1
	$(MKDIR_P) $(DESTDIR)/lib/ || exit 1
	test "$(UI)" != "sdl" || $(MKDIR_P) $(DESTDIR)/ui/widget/ || exit 1
	cp $(top_srcdir)/roms/*.rom $(DESTDIR)/roms
	cp $(top_srcdir)/roms/README.copyright $(DESTDIR)/roms
	cp $(top_srcdir)/lib/*.bmp $(DESTDIR)/lib
	cp $(top_srcdir)/lib/*.scr $(DESTDIR)/lib
	test "$(UI)" != "sdl" || cp $(top_builddir)/ui/widget/fuse.font $(DESTDIR)/ui/widget
#	Copy fuse executable (we should manually copy the required libraries)
	cp $(top_builddir)/.libs/fuse$(EXEEXT) $(DESTDIR) || \
	cp $(top_builddir)/fuse$(EXEEXT) $(DESTDIR)
#	Get text files
	for file in AUTHORS ChangeLog COPYING README; \
	  do cp "$(top_srcdir)/$$file" "$(DESTDIR)/$$file.txt"; \
	done
#	Get manuals
	if test -n "$(GROFF)"; then \
	  sed ':a;N;$$!ba;s/\.PP\n\.TS/\.bp\n&/g' $(top_srcdir)/man/fuse.1 | \
	  $(GROFF) -t -Thtml -man -P -I -P manual > $(DESTDIR)/fuse.html; \
	elif test -n "$(MAN2HTML)"; then \
	  $(MAN2HTML) -r $(top_srcdir)/man/fuse.1 | sed '1d' > $(DESTDIR)/fuse.html; \
	fi
	-mv manual*.png $(DESTDIR);
#	Convert to DOS line endings
	test -z "$(UNIX2DOS)" || find $(DESTDIR) -type f \( -name "*.txt" -or -name "*.html" -or -name "*.copyright" \) -exec $(UNIX2DOS) {} \;

install-win32-strip: install-win32
	test -z "$(STRIP)" || $(STRIP) $(DESTDIR)/fuse$(EXEEXT)

dist-win32-dir:
	$(MAKE) DESTDIR="$(top_win32dir)" install-win32-strip

dist-win32-zip: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	zip -q -9 -r $(abs_top_builddir)/$(package_win32).zip .
	-sha1sum $(top_builddir)/$(package_win32).zip > $(top_builddir)/$(package_win32).zip.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).zip.sha1; }

dist-win32-7z: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -bd $(abs_top_builddir)/$(package_win32).7z .
	-sha1sum $(top_builddir)/$(package_win32).7z > $(top_builddir)/$(package_win32).7z.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).7z.sha1; }

dist-win32-exe: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
#	Locate NSIS in system path, MSYS drive or Cygwin drive
	@NSISFILE="$(abs_top_builddir)/data/win32/installer.nsi"; \
	if makensis -VERSION > /dev/null 2>&1; then \
	  MAKENSIS="makensis"; \
	elif [ -x "/c/Program Files/NSIS/makensis" ]; then \
	  MAKENSIS="/c/Program\ Files/NSIS/makensis"; \
	elif [ -x "/cygdrive/c/Program Files/NSIS/makensis" ]; then \
	  MAKENSIS="/cygdrive/c/Program\ Files/NSIS/makensis"; \
	else \
	  echo 'ERROR: cannot locate makensis tool'; exit 1; \
	fi; \
	case "`uname -s`" in \
	  CYGWIN*) NSISFILE=`cygpath -m $$NSISFILE`;; \
	esac; \
	cd $(top_win32dir); \
	eval "$$MAKENSIS -V2 -NOCD $$NSISFILE"
	mv $(top_win32dir)/$(package_win32)-setup.exe $(top_builddir)
	-sha1sum $(top_builddir)/$(package_win32)-setup.exe > $(top_builddir)/$(package_win32)-setup.exe.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32)-setup.exe.sha1; }

dist-win32: dist-win32-zip dist-win32-7z dist-win32-exe

distclean-win32:
	if test -d "$(top_builddir)/$(package_win32)"; then \
	  rm -rf -- "$(top_builddir)/$(package_win32)"; \
	fi
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe.sha1

.PHONY: install-win32 install-win32-strip dist-win32 dist-win32-dir \
	dist-win32-zip dist-win32-7z dist-win32-exe distclean-win32
