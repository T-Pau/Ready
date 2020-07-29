/*
 ViceC64.swift -- C64 Specifics
 Copyright (C) 2020 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import Emulator
import ViceX64sc

typealias ViceThread = ViceThreadC64

/*
 C64 keyboard matrix:
 
       +-----+-----+-----+-----+-----+-----+-----+-----+
       |Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 0| DEL |Retrn|C_L/R|  F7 |  F1 |  F3 |  F5 |C_U/D|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 1| 3 # |  W  |  A  | 4 $ |  Z  |  S  |  E  | S_L |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 2| 5 % |  R  |  D  | 6 & |  C  |  F  |  T  |  X  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 3| 7 ' |  Y  |  G  | 8 ( |  B  |  H  |  U  |  V  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 4| 9 ) |  I  |  J  |  0  |  M  |  K  |  O  |  N  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 5|  +  |  P  |  L  |  -  | . > | : [ |  @  | , < |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 6|POUND|  *  | ; ] | HOME| S_R |  =  |  ^  | / ? |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 7| 1 ! |  <- | CTRL| 2 " |SPACE|  C= |  Q  | R/S |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
*/

let viceVariant = ViceVariant(keyboardMatrix: [
        [.InsertDelete, .Return, .CursorLeftRight, .F7, .F1, .F3, .F5, .CursorUpDown],
        [.Char("3"), .Char("w"), .Char("a"), .Char("4"), .Char("z"), .Char("s"), .Char("e"), .ShiftLeft],
        [.Char("5"), .Char("r"), .Char("d"), .Char("6"), .Char("c"), .Char("f"), .Char("t"), .Char("x")],
        [.Char("7"), .Char("y"), .Char("g"), .Char("8"), .Char("b"), .Char("h"), .Char("u"), .Char("v")],
        [.Char("9"), .Char("i"), .Char("j"), .Char("0"), .Char("m"), .Char("k"), .Char("o"), .Char("n")],
        [.Char("+"), .Char("p"), .Char("l"), .Char("-"), .Char("."), .Char(":"), .Char("@"), .Char(",")],
        [.Char("£"), .Char("*"), .Char(";"), .ClearHome, .ShiftRight, .Char("="), .ArrowUp, .Char("/")],
        [.Char("1"), .ArrowLeft, .Control, .Char("2"), .Char(" "), .Commodore, .Char("q"), .RunStop]
    ], aliases: [
        .ShiftLeft: [.ShiftLock],
        .Commodore: [.CommodoreLock]
    ], viceModel: [
        .c64Pal: 0,
        .c64cPal: 1,
        .c64OldPal: 2,
        .c64Ntsc: 3,
        .c64cNtsc: 4,
        .c64OldNtsc: 5,
        .c64PalN: 6,
        .sx64Pal: 7,
        .sx64Ntsc: 8,
        .c64Japanese: 9,
        .c64Gs: 10,
        .pet64Pal: 11,
        .pet64Ntsc: 12,
        .ultimax: 13,
    ])
