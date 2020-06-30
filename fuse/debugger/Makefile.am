## Process this file with automake to produce Makefile.in
## Copyright (c) 2001-2016 Philip Kendall
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

fuse_SOURCES += \
                debugger/breakpoint.c \
                debugger/command.c \
                debugger/commandl.l \
                debugger/commandy.y \
                debugger/debugger.c \
                debugger/disassemble.c \
                debugger/event.c \
                debugger/expression.c \
                debugger/system_variable.c \
                debugger/variable.c

debugger/commandl.c: debugger/commandy.c
debugger/commandy.h: debugger/commandy.c

noinst_HEADERS += \
                  debugger/breakpoint.h \
                  debugger/commandy.h \
                  debugger/debugger.h \
                  debugger/debugger_internals.h

CLEANFILES += \
              debugger/commandl.c \
              debugger/commandy.c \
              debugger/commandy.h
