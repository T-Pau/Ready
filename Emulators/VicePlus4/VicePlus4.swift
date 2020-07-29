/*
 VicePlus4.swift -- Plus/4 Specifics
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

typealias ViceThread = ViceThreadPlus4

/*
 Plus/4 keyboard matrix:
 
       +-----+-----+-----+-----+-----+-----+-----+-----+
       |Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 0| DEL | RET |POUND| HELP|  F1 |  F2 |  F3 |  @  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 1| 3 # |  W  |  A  | 4 $ |  Z  |  S  |  E  |SHIFT|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 2| 5 % |  R  |  D  | 6 & |  C  |  F  |  T  |  X  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 3| 7 ' |  Y  |  G  | 8 ( |  B  |  H  |  U  |  V  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 4| 9 ) |  I  |  J  | 0 ^ |  M  |  K  |  O  |  N  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 5| C_D |  P  |  L  | C_U | . > | : [ |  -  | , < |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 6| C_L |  *  | ; ] | C_R | ESC |  =  |  +  | / ? |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 7| 1 ! | HOME| CTRL| 2 " |SPACE|  C= |  Q  | R/S |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
*/
let viceVariant = ViceVariant(keyboardMatrix: [
        [.InsertDelete, .Return, .Char("£"), .Help, .F1, .F2, .F3, .Char("@")],
        [.Char("3"), .Char("w"), .Char("a"), .Char("4"), .Char("z"), .Char("s"), .Char("e"), .Shift],
        [.Char("5"), .Char("r"), .Char("d"), .Char("6"), .Char("c"), .Char("f"), .Char("t"), .Char("x")],
        [.Char("7"), .Char("y"), .Char("g"), .Char("8"), .Char("b"), .Char("h"), .Char("u"), .Char("v")],
        [.Char("9"), .Char("i"), .Char("j"), .Char("0"), .Char("m"), .Char("k"), .Char("o"), .Char("n")],
        [.CursorDown, .Char("p"), .Char("l"), .CursorUp, .Char("."), .Char(":"), .Char("-"), .Char(",")],
        [.CursorLeft, .Char("*"), .Char(";"), .CursorRight, .Escape, .Char("="), .Char("+"), .Char("/")],
        [.Char("1"), .ClearHome, .Control, .Char("2"), .Char(" "), .Commodore, .Char("q"), .RunStop]
    ], aliases: [
        .Shift: [.ShiftLock]
    ], viceModel: [
        .c16Pal: 0,
        .c16Ntsc: 1,
        .plus4Pal: 2,
        .plus4Ntsc: 3,
        .v364Ntsc: 4,
        .c232Ntsc: 5
    ])
