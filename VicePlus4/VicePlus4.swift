/*
 VicePlus4.swift -- Plus/4 Specifics
 Copyright (C) 2020 Dieter Baron
 
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

struct KeyboardMatrix {
    static func column(for key: Key) -> Int? {
        switch key {
        case .InsertDelete, .Char("3"), .Char("5"), .Char("7"), .Char("9"), .CursorDown, .CursorLeft, .Char("1"):
            return 0
        case  .Return, .Char("w"), .Char("r"), .Char("y"), .Char("i"), .Char("p"), .Char("*"), .ClearHome:
            return 1
        case .Char("£"), .Char("a"), .Char("d"), .Char("g"), .Char("j"), .Char("l"), .Char(";"), .Control:
            return 2
        case .Help, .Char("4"), .Char("6"), .Char("8"), .Char("0"), .CursorUp, .CursorRight, .Char("2"):
            return 3
        case .F1, .Char("z"), .Char("c"), .Char("b"), .Char("m"), .Char("."), .Escape, .Char(" "):
            return 4
        case .F2, .Char("s"), .Char("f"), .Char("h"), .Char("k"), .Char(":"), .Char("="), .Commodore:
            return 5
        case .F3, .Char("e"), .Char("t"), .Char("u"), .Char("o"), .Char("-"), .Char("+"), .Char("q"):
            return 6
        case .Char("@"), .Shift, .ShiftLock, .Char("x"), .Char("v"), .Char("n"), .Char(","), .Char("/"), .RunStop:
            return 7

        default:
            return nil
        }
    }
    
    static func row(for key: Key) -> Int? {
        switch key {
        case .InsertDelete, .Return, .Char("£"), .Help, .F1, .F2, .F3, .Char("@"):
            return 0
        case .Char("3"), .Char("w"), .Char("a"), .Char("4"), .Char("z"), .Char("s"), .Char("e"), .Shift, .ShiftLock:
            return 1
        case .Char("5"), .Char("r"), .Char("d"), .Char("6"), .Char("c"), .Char("f"), .Char("t"), .Char("x"):
            return 2
        case .Char("7"), .Char("y"), .Char("g"), .Char("8"), .Char("b"), .Char("h"), .Char("u"), .Char("v"):
            return 3
        case .Char("9"), .Char("i"), .Char("j"), .Char("0"), .Char("m"), .Char("k"), .Char("o"), .Char("n"):
            return 4
        case .CursorDown, .Char("p"), .Char("l"), .CursorUp, .Char("."), .Char(":"), .Char("-"), .Char(","):
            return 5
        case .CursorLeft, .Char("*"), .Char(";"), .CursorRight, .Escape, .Char("="), .Char("+"), .Char("/"):
            return 6
        case .Char("1"), .ClearHome, .Control, .Char("2"), .Char(" "), .Commodore, .Char("q"), .RunStop:
            return 7

        default:
            return nil
        }
    }
}
