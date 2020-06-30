libspectrum 1.4.4
=================

libspectrum is a library which is designed to make the input and
output of ZX Spectrum emulator files slightly easier than it would be
otherwise. It should hopefully compile and run on Unix-based systems,
Win32 and Mac OS X.

Currently supported are:

* Snapshots: .z80, .szx, .sna (all read/write), .zxs, .sp., .snp and
  +D snapshots (read only).
* Tape images: .tzx, .tap, .spc, .sta and .ltp (read/write) and
  .pzx, Warajevo .tap, Z80Em and CSW version 1 (read only).
* Input recordings: .rzx (read/write).
* Disk images: .dsk (both plain and extended), .d40, .d80, .fdi, .img,
  .mgt, .opd, .sad, .scl, .td0, .trd and .udi (identification only).
* Timex cartridges: .dck (read only).
* IDE hard disk images: .hdf (read/write).
* Microdrive cartridge images: .mdr (read/write).

On Unix and Mac OS X, compiling libspectrum should just be as easy as

$ ./configure
$ make

and then `make install' should install it in `/usr/local' ready for
use on your system. If you want to install it somewhere other than
`/usr/local', give the `--prefix=DIR' option to `configure' to install
it under `DIR'. Once installed, some work will probably be necessary
to ensure that the linker can find libspectrum. Either read the
instructions printed when you do `make install' or ask your local
guru.

For details on the functions available, see `doc/libspectrum.txt'.

libspectrum is made available under the GNU General Public License,
which means that it is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See `COPYING' for
the full license.

For updates for libspectrum, its homepage is at:

http://fuse-emulator.sourceforge.net/libspectrum.php

Compiling from Git
------------------

If you're using version of libspectrum from Git rather than one
of the released tarballs, you'll need to run `autogen.sh' before
running 'configure' for the first time.

Compiling for the Wii
---------------------

To compile for the Wii, first make sure the dev tools are in your path
(export PATH=$PATH:$DEVKITPPC/bin). Then, use this configure line:

./configure --target=powerpc-gekko --host=powerpc-gekko \
            --prefix=$DEVKITPPC \
            --without-libgcrypt --with-fake-glib --without-libaudiofile

That is assuming you don't have libgcrypt, glib and libaudiofile for the
Wii. At the time of writing, those haven't been ported yet.

Then, type "make" and if everything went well, "make install".

Philip Kendall <philip-fuse@shadowmagic.org.uk>
1st July, 2018
