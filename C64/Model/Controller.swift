/*
 Controller.swift -- Controller Harware Part
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

struct Controller: MachinePart {
    enum ViceType: Int32 {
        case none = 0
        case joystick = 1
        case paddles = 2
        case mouse1351 = 3
        case mouseNeos = 4
        case mouseAmiga = 5
        case mouseCx22 = 6
        case mouseSt = 7
        case mouseSmart = 8
        case mouseMicromys = 9
        case koalapad = 10
        case lightpenU = 11
        case lightpenL = 12
        case lightpenDatel = 13
        case lightgunY = 14
        case lightgunL = 15
        case lightpenInkwell = 16
        case sampler2bit = 17
        case sampler4bit = 18
        case bbrtc = 19
        case paperclip64 = 20
        case coplinKeypad = 21
        case cardcoKeypad = 22
        case cx85Keypad = 23
        case rushwareKeypad = 24
        case cx21Keypad = 25
        case script64Dongle = 26
        case vizawrite64Dongle = 27
        case waasoftDongle = 28
        case snespad = 29
        
        var inputType: InputType {
            switch self {
            case .bbrtc, .none, .paperclip64, .script64Dongle, .vizawrite64Dongle, .waasoftDongle:
                return .none
            case .joystick:
                return .joystick
            case .paddles:
                return .paddle
            case .koalapad, .mouse1351, .mouseAmiga, .mouseCx22, .mouseMicromys, .mouseNeos, .mouseSmart, .mouseSt:
                return .mouse
            case .lightgunL, .lightgunY:
                return .lightGun
            case .lightpenDatel, .lightpenInkwell, .lightpenL, .lightpenU:
                return .lightPen
            case .cardcoKeypad, .coplinKeypad, .cx21Keypad, .cx85Keypad, .rushwareKeypad, .sampler2bit, .sampler4bit, .snespad:
                return .none // TODO
            }
        }
        
        var isUserPortCompatible: Bool {
            switch self {
            case .bbrtc, .coplinKeypad, .joystick, .mouseAmiga, .mouseCx22, .mouseNeos, .mouseSt, .none, .paperclip64, .rushwareKeypad, .sampler2bit, .sampler4bit:
                return true
        case .cx21Keypad, .cx85Keypad, .cardcoKeypad, .koalapad, .lightgunL, .lightgunY, .lightpenDatel, .lightpenInkwell, .lightpenL, .lightpenU, .mouse1351, .mouseMicromys, .mouseSmart, .paddles, .script64Dongle, .snespad, .vizawrite64Dongle, .waasoftDongle:
                return false
            }
        }
    }
    
    enum InputType: CustomStringConvertible {
        case joystick
        case lightGun
        case lightPen
        case mouse
        case none
        case paddle
        
        var description: String {
            switch self {
            case .joystick:
                return "Joystick"
            case .lightGun:
                return "Light Gun"
            case .lightPen:
                return "Light Pen"
            case .mouse:
                return "Mouse"
            case .none:
                return "None"
            case .paddle:
                return "Paddle"
            }
        }
    }

    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage?
    var priority: Int
    
    var viceType: ViceType
    var inputType: InputType { return viceType.inputType }

    init(identifier: String, name: String, fullName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, viceType: ViceType) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        else {
            self.icon = nil
        }
        self.priority = priority
        self.viceType = viceType
    }
    
    static let none = Controller(identifier: "none", name: "None", iconName: nil, priority: 0, viceType: .none)

    static let controllers = [
        none,
        Controller(identifier: "Competition Pro", name: "Comp. Pro", fullName: "Competition Pro", iconName: "Competition Pro", priority: MachinePartHighPriority, viceType: .joystick),
        Controller(identifier: "QuickShot II", name: "QuickShot", fullName: "Spectravideo QuickShot II", iconName: "Spectravideo QuickShot II", viceType: .joystick),
        Controller(identifier: "1311", name: "1311", fullName: "Commodore Joystick 1311", iconName: "Commodore Joystick 1311", viceType: .joystick),
        //Controller(identifier: "Annihilator", name: "Annihilator", fullName: "Cheetah Annihilator", iconName: "Cheetah Annihilator", viceType: .joystick), // has two buttons
        Controller(identifier: "1351", name: "1351", fullName: "Commodore Mouse 1351", iconName: "Commodore 1351", viceType: .mouse1351),
        Controller(identifier: "STM1", name: "Atari STM1", fullName: "Atari Mouse STM1", iconName: "Atari Mouse STM1", viceType: .mouseSt)
        //Controller(identifier: "Sinclair Magnum", name: "Magnum", fullName: "Sinclair Magnum Light Phaser", iconName: "Sinclair Magnum Light Phaser", inputType: .lightGun)
    ]
    
    static var userPortControllers: [Controller] {
        return controllers.filter({ $0.viceType.isUserPortCompatible })
    }
    
    static private var byIdentifier = [String : Controller]()
    
    static func controller(identifier: String) -> Controller? {
        if byIdentifier.isEmpty {
            for controller in controllers {
                byIdentifier[controller.identifier] = controller
            }
        }
        
        return byIdentifier[identifier]
    }
}

extension Controller: Equatable {
    
}
