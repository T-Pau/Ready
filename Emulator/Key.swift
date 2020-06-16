/*
 Key.swift -- C64 Keys
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

import Foundation


public enum Key : Equatable, Hashable {
    case Char(Character)
    case ArrowLeft
    case ArrowUp
    case ClearHome
    case Commodore
    case Control
    case CursorUpDown
    case CursorLeftRight
    case F1
    case F3
    case F5
    case F7
    case InsertDelete
    case Restore
    case Return
    case RunStop
    case ShiftLeft
    case ShiftLock
    case ShiftRight
    
 
    public var isShift: Bool {
        switch self {
        case .ShiftLeft, .ShiftLock, .ShiftRight:
            return true
        default:
            return false
        }
    }
    
    public static func ==(_ lhs: Key, rhs: Key) -> Bool {
        switch (lhs, rhs) {
        case (.ArrowLeft, .ArrowLeft),
             (.ArrowUp, .ArrowUp),
             (.ClearHome, .ClearHome),
             (.Commodore, .Commodore),
             (.Control, .Control),
             (.CursorUpDown, .CursorUpDown),
             (.CursorLeftRight, .CursorLeftRight),
             (.F1, .F1),
             (.F3, .F3),
             (.F5, .F5),
             (.F7, .F7),
             (.InsertDelete, .InsertDelete),
             (.Restore, .Restore),
             (.Return, .Return),
             (.RunStop, .RunStop),
             (.ShiftLeft, .ShiftLeft),
             (.ShiftLock, .ShiftLock),
             (.ShiftRight, .ShiftRight):
            return true
        case (.Char(let l), .Char(let r)):
            return l == r
        default:
            return false
        }
    }
}
