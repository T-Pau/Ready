## Process this file with automake to produce Makefile.in
## Copyright (c) 2003-2013 Philip Kendall
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

noinst_HEADERS += compat/getopt.h

if COMPAT_DIRNAME
fuse_SOURCES += compat/dirname.c
endif

if COMPAT_GETOPT
fuse_SOURCES += compat/getopt.c compat/getopt1.c
endif

## Amiga routines
if COMPAT_AMIGA
fuse_SOURCES += \
                compat/unix/dir.c \
                compat/unix/file.c \
                compat/amiga/osname.c \
                compat/amiga/paths.c \
                compat/unix/timer.c
endif

## Linux routines
if COMPAT_LINUX
fuse_SOURCES += \
                compat/unix/dir.c \
                compat/unix/file.c \
                compat/unix/osname.c \
                compat/linux/relative_paths.c \
                compat/unix/paths.c \
                compat/unix/timer.c

if HAVE_SOCKETS
fuse_SOURCES += compat/unix/socket.c
endif
endif

## Morphos routines
if COMPAT_MORPHOS
fuse_SOURCES += \
                compat/unix/dir.c \
                compat/unix/file.c \
                compat/morphos/osname.c \
                compat/amiga/paths.c \
                compat/unix/timer.c
endif

## Unix routines
if COMPAT_UNIX
fuse_SOURCES += \
                compat/unix/dir.c \
                compat/unix/file.c \
                compat/unix/osname.c \
                compat/unix/relative_paths.c \
                compat/unix/paths.c \
                compat/unix/timer.c

if HAVE_SOCKETS
fuse_SOURCES += compat/unix/socket.c
endif
endif

## Wii routines
if COMPAT_WII
fuse_SOURCES += \
                compat/wii/dir.c \
                compat/unix/file.c \
                compat/wii/osname.c \
                compat/wii/paths.c \
                compat/wii/timer.c
endif

## Windows routines
if COMPAT_WIN32
fuse_SOURCES += \
                compat/unix/dir.c \
                compat/unix/file.c \
                compat/win32/osname.c \
                compat/win32/paths.c \
                compat/win32/timer.c

if HAVE_SOCKETS
fuse_SOURCES += compat/win32/socket.c
endif
endif

## SpeccyBoot routines
if HAVE_TUNTAP
fuse_SOURCES += compat/unix/tuntap.c
endif
