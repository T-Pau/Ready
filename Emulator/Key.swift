/*
 Key.swift -- Keys Found on any of the Emulated Keyboards
 Copyright (C) 2019-2020 Dieter Baron
 
 This file is part of Ready, a Home Computer emulator for iPad.
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
    case Break
    case Caps
    case ClearHome
    case Commodore
    case CommodoreLock
    case Control
    case CursorDown
    case CursorLeft
    case CursorLeftRight
    case CursorRight
    case CursorUp
    case CursorUpDown
    case Delete
    case Edit
    case Escape
    case ExtendedMode
    case F1
    case F2
    case F3
    case F4
    case F5
    case F6
    case F7
    case F8
    case F9
    case F10
    case Graphics
    case Help
    case InsertDelete
    case InverseVideo
    case Option
    case Reset
    case Restore
    case Return
    case RunStop
    case Select
    case Shift
    case ShiftLeft
    case ShiftLock
    case ShiftRight
    case Start
    case SymbolShift
    case Tab
    case TrueVideo
    
    public var isShift: Bool {
        switch self {
        case .Shift, .ShiftLeft, .ShiftLock, .ShiftRight:
            return true
        default:
            return false
        }
    }
}
