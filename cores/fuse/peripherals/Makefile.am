## Process this file with automake to produce Makefile.in
## Copyright (c) 2011 Philip Kendall
## Copyright (c) 2015 Stuart Brady
## Copyright (c) 2015 Sergio Baldoví
## Copyright (c) 2017 Fredrick Meunier

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
                peripherals/ay.c \
                peripherals/covox.c \
                peripherals/dck.c \
                peripherals/fuller.c \
                peripherals/if1.c \
                peripherals/if2.c \
                peripherals/joystick.c \
                peripherals/kempmouse.c \
                peripherals/melodik.c \
                peripherals/multiface.c \
                peripherals/printer.c \
                peripherals/scld.c \
                peripherals/speccyboot.c \
                peripherals/specdrum.c \
                peripherals/spectranet.c \
                peripherals/ula.c \
                peripherals/usource.c \
                peripherals/disk/beta.c \
                peripherals/disk/crc.c \
                peripherals/disk/didaktik.c \
                peripherals/disk/disciple.c \
                peripherals/disk/disk.c \
                peripherals/disk/fdd.c \
                peripherals/disk/opus.c \
                peripherals/disk/plusd.c \
                peripherals/disk/trdos.c \
                peripherals/disk/upd_fdc.c \
                peripherals/disk/wd_fdc.c \
                peripherals/ide/divide.c \
                peripherals/ide/divmmc.c \
                peripherals/ide/divxxx.c \
                peripherals/ide/ide.c \
                peripherals/ide/simpleide.c \
                peripherals/ide/zxatasp.c \
                peripherals/ide/zxcf.c \
                peripherals/ide/zxmmc.c

if BUILD_SPECCYBOOT
fuse_SOURCES += peripherals/nic/enc28j60.c
endif

if BUILD_SPECTRANET
fuse_SOURCES += \
                peripherals/flash/am29f010.c \
                peripherals/nic/w5100.c \
                peripherals/nic/w5100_socket.c
endif

noinst_HEADERS += \
                  peripherals/ay.h \
                  peripherals/covox.h \
                  peripherals/dck.h \
                  peripherals/fuller.h \
                  peripherals/if1.h \
                  peripherals/if2.h \
                  peripherals/joystick.h \
                  peripherals/kempmouse.h \
                  peripherals/melodik.h \
                  peripherals/multiface.h \
                  peripherals/printer.h \
                  peripherals/scld.h \
                  peripherals/speccyboot.h \
                  peripherals/specdrum.h \
                  peripherals/spectranet.h \
                  peripherals/ula.h \
                  peripherals/usource.h \
                  peripherals/disk/beta.h \
                  peripherals/disk/crc.h \
                  peripherals/disk/didaktik.h \
                  peripherals/disk/disciple.h \
                  peripherals/disk/disk.h \
                  peripherals/disk/fdd.h \
                  peripherals/disk/opus.h \
                  peripherals/disk/plusd.h \
                  peripherals/disk/trdos.h \
                  peripherals/disk/upd_fdc.h \
                  peripherals/disk/wd_fdc.h \
                  peripherals/ide/divide.h \
                  peripherals/ide/divmmc.h \
                  peripherals/ide/divxxx.h \
                  peripherals/ide/ide.h \
                  peripherals/ide/simpleide.h \
                  peripherals/ide/zxatasp.h \
                  peripherals/ide/zxcf.h \
                  peripherals/ide/zxmmc.h \
                  peripherals/flash/am29f010.h \
                  peripherals/nic/enc28j60.h \
                  peripherals/nic/w5100.h \
                  peripherals/nic/w5100_internals.h
