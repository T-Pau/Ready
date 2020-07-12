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

import Atari800C

@objc public class Atari800: Emulator {
    public override init() {
        atari800Thread = Atari800Thread()
        super.init()
        atari800Thread?.delegate = self
    }
    
    private var controlPressed = false
    private var leftShiftPressed = false
    private var rightShiftPressed = false
    private var pressedKeyes = [Int32]()
    
    @objc public func ramSize() -> Int32 {
        switch machine.specification.computer.model {
        case .atari600Xl:
            return 16
        case .atari800Xl:
            return 64
            
        default:
            return 64
        }
    }

    private func keyCode(for key: Key) -> Int32? {
        switch key {
        case .Escape:
            return AKEY_ESCAPE
        case .Char("1"):
            return AKEY_1
        case .Char("2"):
            return AKEY_2
        case .Char("3"):
            return AKEY_3
        case .Char("4"):
            return AKEY_4
        case .Char("5"):
            return AKEY_5
        case .Char("6"):
            return AKEY_6
        case .Char("7"):
            return AKEY_7
        case .Char("8"):
            return AKEY_8
        case .Char("9"):
            return AKEY_9
        case .Char("0"):
            return AKEY_0
        case .Char("<"):
            return AKEY_LESS
        case .Char(">"):
            return AKEY_GREATER
        case .Delete:
            return AKEY_BACKSPACE
        case .Break:
            return AKEY_BREAK
            
        case .Tab:
            return AKEY_TAB
        case .Char("q"):
            return AKEY_q
        case .Char("w"):
            return AKEY_w
        case .Char("e"):
            return AKEY_e
        case .Char("r"):
            return AKEY_r
        case .Char("t"):
            return AKEY_t
        case .Char("y"):
            return AKEY_y
        case .Char("u"):
            return AKEY_u
        case .Char("i"):
            return AKEY_i
        case .Char("o"):
            return AKEY_o
        case .Char("p"):
            return AKEY_p
        case .Char("-"):
            return AKEY_MINUS
        case .Char("="):
            return AKEY_EQUAL
        case .Return:
            return AKEY_RETURN

        case .Char("a"):
            return AKEY_a
        case .Char("s"):
            return AKEY_s
        case .Char("d"):
            return AKEY_d
        case .Char("f"):
            return AKEY_f
        case .Char("g"):
            return AKEY_g
        case .Char("h"):
            return AKEY_h
        case .Char("j"):
            return AKEY_j
        case .Char("k"):
            return AKEY_k
        case .Char("l"):
            return AKEY_l
        case .Char(";"):
            return AKEY_SEMICOLON
        case .Char("+"):
            return AKEY_PLUS
        case .Char("*"):
            return AKEY_ASTERISK
        case .Caps:
            return AKEY_CAPSTOGGLE

        case .Char("z"):
            return AKEY_z
        case .Char("x"):
            return AKEY_x
        case .Char("c"):
            return AKEY_c
        case .Char("v"):
            return AKEY_v
        case .Char("b"):
            return AKEY_b
        case .Char("n"):
            return AKEY_n
        case .Char("m"):
            return AKEY_m
        case .Char(","):
            return AKEY_COMMA
        case .Char("."):
            return AKEY_FULLSTOP
        case .Char("/"):
            return AKEY_SLASH
        case .InverseVideo:
            return AKEY_ATARI
            
        case .Char(" "):
            return AKEY_SPACE
            
        case .Help:
            return AKEY_HELP
        case .Reset:
            return AKEY_WARMSTART
            
        default:
            return nil
        }
    }
    private func joystickValue(for buttons: JoystickButtons) -> Int32 {
        var value = Int32(0x0f)
        
        if buttons.down {
            value &= INPUT_STICK_BACK
        }
        if buttons.left {
            value &= INPUT_STICK_LEFT
        }
        if buttons.right {
            value &= INPUT_STICK_RIGHT
        }
        if buttons.up {
            value &= INPUT_STICK_FORWARD
        }
        
        return value
    }
        
    override public func handle(event: Event) -> Bool {
        switch event {
        case let .joystick(port: port, buttons: buttons, _):
            atari800Thread?.updateJoystick(Int32(port - 1), directions: joystickValue(for: buttons), fire: buttons.fire)

        case .key(let key, let pressed):
            switch (key) {
            case .Control:
                controlPressed = pressed

            case .ShiftLeft:
                leftShiftPressed = pressed

            case .ShiftRight:
                rightShiftPressed = pressed

            case .Option:
                if pressed {
                    INPUT_key_consol &= ~INPUT_CONSOL_OPTION
                }
                else {
                    INPUT_key_consol |= INPUT_CONSOL_OPTION
                }
            case .Select:
                if pressed {
                    INPUT_key_consol &= ~INPUT_CONSOL_SELECT
                }
                else {
                    INPUT_key_consol |= INPUT_CONSOL_SELECT
                }
            case .Start:
                if pressed {
                    INPUT_key_consol &= ~INPUT_CONSOL_START
                }
                else {
                    INPUT_key_consol |= INPUT_CONSOL_START
                }

            default:
                if let keyCode = keyCode(for: key) {
                    if pressed {
                        pressedKeyes.append(keyCode)
                    }
                    else {
                        pressedKeyes.removeAll(where: { $0 == keyCode })
                    }
                }
            }
            
            INPUT_key_shift = leftShiftPressed || rightShiftPressed ? 1 : 0
            INPUT_key_code = pressedKeyes.last ?? AKEY_NONE
            if (INPUT_key_code >= 0) {
                if (leftShiftPressed || rightShiftPressed) {
                    INPUT_key_code |= AKEY_SHFT;
                }
                if (controlPressed) {
                    INPUT_key_code |= AKEY_CTRL
                }
            }
            
        case .quit:
            atari800Thread?.running = false
            
        case .reset:
            Atari800_Warmstart(); // TODO: cold start?
            
        default:
            break
        }
        
        return true
    }
    
    override public func start() {
        var args = [
            "atari800"
        ]
        
        let dataDir = Bundle.main.resourceURL?.appendingPathComponent("atari800").path ?? "."
        
        if let emulatorInfo = machine.specification.computer.emulatorInfo(for: .atari8Bit) as? Atari800EmulatorInfo {
            args.append(contentsOf: emulatorInfo.arguments.map({ $0.replacingOccurrences(of: "@DATADIR@", with: dataDir) }))
        }
        atari800Thread?.args = args
        atari800Thread?.start()
    }
    
    public override func set(borderMode: MachineConfigOld.BorderMode) {
        atari800Thread?.newBorderMode = borderMode.cValue
    }
}

