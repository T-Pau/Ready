/*
 Key.swift -- Keys Found on any of the Emulated Keyboards
 Copyright (C) 2019-2020 Dieter Baron
 
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

import Foundation


public enum Key : Equatable, Hashable {
    case Char(Character)
    case Alt
    case AltLeft
    case AltRight
    case ArrowLeft
    case ArrowUp
    case Backspace
    case Break
    case Caps
    case CapsLock
    case ClearHome
    case CommanderLeft
    case CommanderRight
    case Commodore
    case CommodoreLock
    case Control
    case ControlLeft
    case ControlRight
    case CursorDown
    case CursorLeft
    case CursorLeftRight
    case CursorRight
    case CursorUp
    case CursorUpDown
    case Delete
    case Display4080
    case Edit
    case End
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
    case F11
    case F12
    case Graphics
    case Help
    case Insert
    case InsertDelete
    case InverseVideo
    case Keypad0
    case Keypad1
    case Keypad2
    case Keypad3
    case Keypad4
    case Keypad5
    case Keypad6
    case Keypad7
    case Keypad8
    case Keypad9
    case KeypadEnter
    case KeypadMinus
    case KeypadPeriod
    case KeypadPlus
    case LineFeed
    case Menu
    case Option
    case PageDown
    case PageUp
    case Reset
    case Restore
    case Return
    case RunStop
    case ScrollLock
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
