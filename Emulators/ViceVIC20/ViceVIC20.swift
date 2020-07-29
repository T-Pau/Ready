/*
 ViceVIC20.swift -- VIC-20 Specifics
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

import ViceX64sc

import Emulator

typealias ViceThread = ViceThreadVIC20

/*
 VIC-20 keyboard matrix:
 
       +-----+-----+-----+-----+-----+-----+-----+-----+
       |Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 0| 1 ! |  <- | CTRL| R/S |SPACE|  C= |  Q  | 2 " |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 1| 3 # |  W  |  A  | S_L |  Z  |  S  |  E  | 4 $ |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 2| 5 % |  R  |  D  |  X  |  C  |  F  |  T  | 6 & |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 3| 7 ' |  Y  |  G  |  V  |  B  |  H  |  U  | 8 ( |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 4| 9 ) |  I  |  J  |  N  |  M  |  K  |  O  |  0  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 5|  +  |  P  |  L  | , < | . > | : [ |  @  |  -  |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 6|POUND|  *  | ; ] | / ? | S_R |  =  |  ^  | HOME|
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 |Bit 7| DEL | RET |C_L/R|C_U/D|  F1 |  F3 |  F5 |  F7 |
 +-----+-----+-----+-----+-----+-----+-----+-----+-----+
*/

let viceVariant = ViceVariant(keyboardMatrix: [
        [.Char("1"), .ArrowLeft, .Control, .RunStop, .Char(" "), .Commodore, .Char("q"), .Char("2")],
        [.Char("3"), .Char("w"), .Char("a"), .ShiftLeft, .Char("z"), .Char("s"), .Char("e"), .Char("4")],
        [.Char("5"), .Char("r"), .Char("d"), .Char("x"), .Char("c"), .Char("f"), .Char("t"), .Char("6")],
        [.Char("7"), .Char("y"), .Char("g"), .Char("v"), .Char("b"), .Char("h"), .Char("u"), .Char("8")],
        [.Char("9"), .Char("i"), .Char("j"), .Char("n"), .Char("m"), .Char("k"), .Char("o"), .Char("0")],
        [.Char("+"), .Char("p"), .Char("l"), .Char(","), .Char("."), .Char(":"), .Char("@"), .Char("-")],
        [.Char("£"), .Char("*"), .Char(";"), .Char("/"), .ShiftRight, .Char("="), .ArrowUp, .ClearHome],
        [.InsertDelete, .Return, .CursorLeftRight, .CursorUpDown, .F1, .F3, .F5, .F7]
    ], aliases: [
        .ShiftLeft: [.ShiftLock]
    ], viceModel: [
        .vic20Pal: 0,
        .vic20Ntsc: 1,
        .vic1001: 3
    ])


