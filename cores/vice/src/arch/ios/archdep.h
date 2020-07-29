/*
 archdep.h
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

#ifndef archdep_h
#define archdep_h

#define VICE_ARCHAPI_PRIVATE_API
#include "archapi.h"
#undef VICE_ARCHAPI_PRIVATE_API

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\n"


/* Filesystem dependant operators.  */
#define FSDEVICE_DEFAULT_DIR "."
#define FSDEV_DIR_SEP_STR    "/"
#define FSDEV_DIR_SEP_CHR    '/'
#define FSDEV_EXT_SEP_STR    "."
#define FSDEV_EXT_SEP_CHR    '.'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ':'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ":"

/* Modes for fopen().  */
#define MODE_READ              "r"
#define MODE_READ_TEXT         "r"
#define MODE_READ_WRITE        "r+"
#define MODE_WRITE             "w"
#define MODE_WRITE_TEXT        "w"
#define MODE_APPEND            "a"
#define MODE_APPEND_READ_WRITE "a+"

/* Access types */
#define ARCHDEP_R_OK R_OK
#define ARCHDEP_W_OK W_OK
#define ARCHDEP_X_OK X_OK
#define ARCHDEP_F_OK F_OK

/* what to use to return an error when a socket error happens */
#define ARCHDEP_SOCKET_ERROR errno

/* Keyword to use for a static prototype */
#define STATIC_PROTOTYPE static

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1
#define ARCHDEP_VICII_DSCAN   1
#define ARCHDEP_VICII_HWSCALE 1
#define ARCHDEP_VDC_DSIZE     1
#define ARCHDEP_VDC_DSCAN     1
#define ARCHDEP_VDC_HWSCALE   1
#define ARCHDEP_VIC_DSIZE     1
#define ARCHDEP_VIC_DSCAN     1
#define ARCHDEP_VIC_HWSCALE   1
#define ARCHDEP_CRTC_DSIZE    1
#define ARCHDEP_CRTC_DSCAN    1
#define ARCHDEP_CRTC_HWSCALE  1
#define ARCHDEP_TED_DSIZE     1
#define ARCHDEP_TED_DSCAN     1
#define ARCHDEP_TED_HWSCALE   1

/* Video chip double buffering.  */
#define ARCHDEP_VICII_DBUF 0
#define ARCHDEP_VDC_DBUF   0
#define ARCHDEP_VIC_DBUF   0
#define ARCHDEP_CRTC_DBUF  0
#define ARCHDEP_TED_DBUF   0

/* No key symcode.  */
#define ARCHDEP_KEYBOARD_SYM_NONE 0

/* Default sound fragment size */
#define ARCHDEP_SOUND_FRAGMENT_SIZE SOUND_FRAGMENT_SMALL

/* Default sound output mode */
#define ARCHDEP_SOUND_OUTPUT_MODE SOUND_OUTPUT_SYSTEM

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "viceprnt.out"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "LPT1:"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "LPT2:"

extern char *bundle_directory;

void archdep_set_resources(void);
int archdep_late_init(void);

#endif /* archdep_h */
