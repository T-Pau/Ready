// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _LOADSAVE_H_
#define _LOADSAVE_H_

#define KERNAL_OPEN 0xFFC0
#define KERNAL_CLOSE 0xFFC3
#define KERNAL_CHKIN 0xFFC6
#define KERNAL_CHKOUT 0xFFC9
#define KERNAL_CLRCHN 0xFFCC
#define KERNAL_CHRIN 0xFFCF
#define KERNAL_CHROUT 0xFFD2
#define KERNAL_LOAD 0xFFD5
#define KERNAL_SAVE 0xFFD8

void io_init();

bool IO_CALL();

#endif
