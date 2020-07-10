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
        if !buttons.fire {
            value |= 0x10
        }
        
        return value
    }
        
    override public func handle(event: Event) -> Bool {
        switch event {
        case let .joystick(port: port, buttons: buttons, _):
            switch port {
            case 1:
                joystickPort.0 = joystickValue(for: buttons)

            case 2:
                joystickPort.1 = joystickValue(for: buttons)

            default:
                break
            }

        case .key(let key, let pressed):
            // TODO: implement
            break
            
        case .quit:
            atari800Thread?.running = false
            
        default:
            break
        }
        
        return true
    }
    
    override public func start() {
        let args = [
            "atari800"
        ]
        atari800Thread?.args = args
        atari800Thread?.start()
    }
    
    public override func set(borderMode: MachineConfigOld.BorderMode) {
        atari800Thread?.newBorderMode = borderMode.cValue
    }
}

