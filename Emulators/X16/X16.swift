/*
 Atari800.swift -- High Level Interface to atari800
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

import UIKit
import Emulator

import X16C

@objc public class X16: Emulator {
    public override init() {
        x16Thread = X16Thread()
        super.init()
        x16Thread?.delegate = self
    }
    
        
    override public func handle(event: Event) -> Bool {
        switch event {
        case let .joystick(port: port, buttons: buttons, _):
            // TODO
            break

        case .key(let key, let pressed):
            if let scancode = scancode[key] {
                handle_keyboard(pressed, scancode)
            }

        case .quit:
            return false;
            
        case .reset:
            // TODO
            break
            
        default:
            break
        }
        
        return true
    }
    
    override public func start() {
        var args = [
            "x16emu",
            "-ram", "2048"
        ]
        
        if let file = machine.programFile?.url?.path {
            args.append(contentsOf: ["-prg", file, "-run"])
        }
        
        x16Thread?.dataDir = Bundle.main.resourceURL?.appendingPathComponent("x16-emulator").path ?? "."
        
        if let emulatorInfo = machine.specification.computer.emulatorInfo(for: .x16) as? X16EmulatorInfo {
            args.append(contentsOf: emulatorInfo.arguments);
        }
        x16Thread?.args = args
        x16Thread?.start()
    }
    
    private var scancode: [Key: SDL_Scancode] = [
        .Escape: SDL_SCANCODE_ESCAPE,
        .F1: SDL_SCANCODE_F1,
        .F2: SDL_SCANCODE_F2,
        .F3: SDL_SCANCODE_F3,
        .F4: SDL_SCANCODE_F4,
        .F5: SDL_SCANCODE_F5,
        .F6: SDL_SCANCODE_F6,
        .F7: SDL_SCANCODE_F7,
        .F8: SDL_SCANCODE_F8,
        .F9: SDL_SCANCODE_F9,
        .F10: SDL_SCANCODE_F10,
        .F11: SDL_SCANCODE_F11,
        .F12: SDL_SCANCODE_F12,
        
        .ArrowLeft: SDL_SCANCODE_GRAVE,
        .Char("1"): SDL_SCANCODE_1,
        .Char("2"): SDL_SCANCODE_2,
        .Char("3"): SDL_SCANCODE_3,
        .Char("4"): SDL_SCANCODE_4,
        .Char("5"): SDL_SCANCODE_5,
        .Char("6"): SDL_SCANCODE_6,
        .Char("7"): SDL_SCANCODE_7,
        .Char("8"): SDL_SCANCODE_8,
        .Char("9"): SDL_SCANCODE_9,
        .Char("0"): SDL_SCANCODE_0,
        .Char("-"): SDL_SCANCODE_MINUS,
        .Char("="): SDL_SCANCODE_EQUALS,
        .Backspace: SDL_SCANCODE_BACKSPACE,
        
        .Tab: SDL_SCANCODE_TAB,
        .Char("q"): SDL_SCANCODE_Q,
        .Char("w"): SDL_SCANCODE_W,
        .Char("e"): SDL_SCANCODE_E,
        .Char("r"): SDL_SCANCODE_R,
        .Char("t"): SDL_SCANCODE_T,
        .Char("y"): SDL_SCANCODE_Y,
        .Char("u"): SDL_SCANCODE_U,
        .Char("i"): SDL_SCANCODE_I,
        .Char("o"): SDL_SCANCODE_O,
        .Char("p"): SDL_SCANCODE_P,
        .Char("["): SDL_SCANCODE_LEFTBRACKET,
        .Char("]"): SDL_SCANCODE_RIGHTBRACKET,
        .Char("£"): SDL_SCANCODE_BACKSLASH,

        // TODO: .ShiftLock
        .Char("a"): SDL_SCANCODE_A,
        .Char("s"): SDL_SCANCODE_S,
        .Char("d"): SDL_SCANCODE_D,
        .Char("f"): SDL_SCANCODE_F,
        .Char("g"): SDL_SCANCODE_G,
        .Char("h"): SDL_SCANCODE_H,
        .Char("j"): SDL_SCANCODE_J,
        .Char("k"): SDL_SCANCODE_K,
        .Char("l"): SDL_SCANCODE_L,
        .Char(";"): SDL_SCANCODE_SEMICOLON,
        .Char("'"): SDL_SCANCODE_APOSTROPHE,
        .Return: SDL_SCANCODE_RETURN,

        .ShiftLeft: SDL_SCANCODE_LSHIFT,
        .Char("z"): SDL_SCANCODE_Z,
        .Char("x"): SDL_SCANCODE_X,
        .Char("c"): SDL_SCANCODE_C,
        .Char("v"): SDL_SCANCODE_V,
        .Char("b"): SDL_SCANCODE_B,
        .Char("n"): SDL_SCANCODE_N,
        .Char("m"): SDL_SCANCODE_M,
        .Char(","): SDL_SCANCODE_COMMA,
        .Char("."): SDL_SCANCODE_PERIOD,
        .Char("/"): SDL_SCANCODE_SLASH,
        .ShiftRight: SDL_SCANCODE_RSHIFT,

        .ControlLeft: SDL_SCANCODE_LCTRL,
        // TODO: .CommanderLeft
        .AltLeft: SDL_SCANCODE_LALT,
        .Char(" "): SDL_SCANCODE_SPACE,
        .AltRight: SDL_SCANCODE_RALT,
        // TODO: .CommanderRight
        .Menu: SDL_SCANCODE_MENU,
        .ControlRight: SDL_SCANCODE_RCTRL,

        // TODO: .Restore
        // TODO: .Display4080
        // TODO: .RunStop

        .Insert: SDL_SCANCODE_INSERT,
        .ClearHome: SDL_SCANCODE_CLEAR,
        .PageUp: SDL_SCANCODE_PAGEUP,
        .Delete: SDL_SCANCODE_DELETE,
        .End: SDL_SCANCODE_END,
        .PageDown: SDL_SCANCODE_PAGEDOWN,
        
        .CursorUp: SDL_SCANCODE_UP,
        .CursorLeft: SDL_SCANCODE_LEFT,
        .CursorDown: SDL_SCANCODE_DOWN,
        .CursorRight: SDL_SCANCODE_RIGHT
    ]
}

