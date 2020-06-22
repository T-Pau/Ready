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

import UIKit

public struct Controller: MachinePart {
    public struct DeviceConfig: Equatable {
        public var sensitivity = 1.0
        public var numberOfButtons = 1
    }

    public enum ViceType: Int32 {
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
        case koalaPad = 10
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
        
        public var inputType: InputType {
            switch self {
            case .bbrtc, .none, .paperclip64, .script64Dongle, .vizawrite64Dongle, .waasoftDongle:
                return .none
            case .joystick:
                return .joystick
            case .paddles:
                return .paddle
            case .mouse1351, .mouseAmiga, .mouseCx22, .mouseMicromys, .mouseNeos, .mouseSmart, .mouseSt:
                return .mouse
            case .lightgunL, .lightgunY:
                return .lightGun
            case .koalaPad, .lightpenDatel, .lightpenInkwell, .lightpenL, .lightpenU:
                return .lightPen
            case .cardcoKeypad, .coplinKeypad, .cx21Keypad, .cx85Keypad, .rushwareKeypad, .sampler2bit, .sampler4bit, .snespad:
                return .none // TODO
            }
        }
        
        public var connector: ConnectorType {
            switch self {
            case .bbrtc, .coplinKeypad, .joystick, .mouseAmiga, .mouseCx22, .mouseNeos, .mouseSt, .none, .paperclip64, .rushwareKeypad, .sampler2bit, .sampler4bit:
                return .atariJoystick
            case .cx21Keypad, .cx85Keypad, .cardcoKeypad, .koalaPad, .mouse1351, .mouseMicromys, .mouseSmart, .paddles, .script64Dongle, .snespad, .vizawrite64Dongle, .waasoftDongle:
                return .atariJoystickAnalog
            case  .lightgunL, .lightgunY, .lightpenDatel, .lightpenInkwell, .lightpenL, .lightpenU:
                return .c64JoystickLightpen
            }
        }
        
        public var isUserPortCompatible: Bool {
            return connector == .atariJoystick
        }
        
        public var isPort2Compatible: Bool {
            switch self {
            case .lightgunL, .lightgunY, .lightpenL, .lightpenU, .lightpenDatel, .lightpenInkwell:
                return false
            default:
                return true
            }
        }
        
        public var numberOfButtons: Int {
            switch self {
            case .joystick, .lightgunL, .lightgunY, .lightpenDatel, .lightpenL, .lightpenU, .mouseCx22:
                return 1
            case .koalaPad, .lightpenInkwell, .mouse1351, .mouseNeos, .mouseSmart, .mouseSt, .paddles:
                return 2
            case .mouseAmiga:
                return 3
            case .mouseMicromys:
                return 5
            default: // TODO
                return 0
            }
        }
        
        public var lightPenNeedsButtonOnTouch: Bool {
            switch self {
            case .lightpenDatel, .lightpenL, .lightpenU:
                return true
            default:
                return false
            }
        }
    }
    
    public enum InputType: CustomStringConvertible {
        case joystick
        case lightGun
        case lightPen
        case mouse
        case none
        case paddle
        
        public var description: String {
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

    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var portIcon: UIImage?
    public var priority: Int
    public var connector: ConnectorType { return viceType.connector }
    
    public var viceType: ViceType
    public var inputType: InputType { return viceType.inputType }
    public var deviceConfig = DeviceConfig()
    
    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, portIconName: String? = nil, priority: Int = MachinePartNormalPriority, viceType: ViceType, numberOfButtons: Int? = nil, sensitivity: Double = 1) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        else {
            self.icon = nil
        }
        if let portIconName = portIconName {
            self.portIcon = UIImage(named: portIconName)
        }
        else {
            self.portIcon = self.icon
        }
        self.priority = priority
        self.viceType = viceType
        if let numberOfButtons = numberOfButtons {
            deviceConfig.numberOfButtons = numberOfButtons
        }
        else {
            deviceConfig.numberOfButtons = viceType.numberOfButtons
        }
        deviceConfig.sensitivity = sensitivity
    }
    
    public static let none = Controller(identifier: "none", name: "None", iconName: nil, priority: 0, viceType: .none)

    public static let controllers = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: [
            none,
        ]),
        
        MachinePartSection(title: "Joysticks", parts: [
            Controller(identifier: "Competition Pro",
                       name: "Comp. Pro",
                       fullName: "Competition Pro",
                       iconName: "Competition Pro",
                       priority: MachinePartHighPriority,
                       viceType: .joystick),
            
            Controller(identifier: "Competition Pro Blue",
                       name: "Comp. Pro",
                       fullName: "Competition Pro",
                       variantName: "Blue",
                       iconName: "Competition Pro Blue",
                       viceType: .joystick),
            
            Controller(identifier: "Competition Pro Cleaer",
                       name: "Comp. Pro",
                       fullName: "Competition Pro",
                       variantName: "Clear",
                       iconName: "Competition Pro Clear",
                       viceType: .joystick),
            
            Controller(identifier: "1311",
                       name: "1311",
                       fullName: "Commodore Joystick 1311",
                       iconName: "Commodore Joystick 1311",
                       viceType: .joystick),
            
            Controller(identifier: "QuickShot II",
                       name: "QuickShot",
                       fullName: "Spectravideo QuickShot II",
                       iconName: "Spectravideo QuickShot II",
                       viceType: .joystick),
            
            Controller(identifier: "QuickShot IX",
                       name: "QuickShot IX",
                       fullName: "Spectravideo QuickShot IX",
                       iconName: "Spectravideo QuickShot IX",
                       viceType: .joystick),
            
            Controller(identifier: "Annihilator",
                       name: "Annihilator",
                       fullName: "Cheetah Annihilator",
                       iconName: "Cheetah Annihilator",
                       viceType: .joystick,
                       numberOfButtons: 2),

            Controller(identifier: "Elite 2002 Joystick",
                       name: "Elite 2002",
                       fullName: "Elite Multi-Function 2002",
                       variantName: "Joystick Mode",
                       iconName: "Elite Multi-Function 2002 Joystick",
                       viceType: .joystick)
        ]),
        
        MachinePartSection(title: "Mice", parts: [
            Controller(identifier: "1351",
                       name: "1351",
                       fullName: "Commodore Mouse 1351",
                       iconName: "Commodore 1351",
                       viceType: .mouse1351),
            
            Controller(identifier: "Smart Mouse",
                       name: "Smart Mouse",
                       fullName: "CMD Smart Mouse",
                       iconName: "CMD Smart Mouse",
                       viceType: .mouseSmart),

            Controller(identifier: "Smart Track",
                       name: "Smart Track",
                       fullName: "CMD Smart Track",
                       iconName: "CMD Smart Track",
                       viceType: .mouseSmart),

            Controller(identifier: "Amiga Mouse",
                       name: "Amiga",
                       fullName: "Commodore Amiga Mouse",
                       iconName: "Commodore 1351",
                       viceType: .mouseAmiga,
                       numberOfButtons: 2),

            Controller(identifier: "STM1",
                       name: "Atari STM1",
                       fullName: "Atari Mouse STM1",
                       iconName: "Atari Mouse STM1",
                       viceType: .mouseSt),
            
            Controller(identifier: "Neos",
                       name: "Neos",
                       fullName: "Nihon Neos Mouse",
                       iconName: "Nihon Neos Mouse",
                       viceType: .mouseNeos),
            
            Controller(identifier: "CX22",
                       name: "CX22",
                       fullName: "Atari Track-Ball CX22",
                       iconName: "Atari Track-Ball CX22",
                       viceType: .mouseCx22),

            Controller(identifier: "KoalaPad",
                       name: "KoalaPad",
                       iconName: "KoalaPad",
                       viceType: .koalaPad)
        ]),
        
        MachinePartSection(title: "Paddles", parts: [
            Controller(identifier: "1312",
                       name: "1312",
                       fullName: "Commodore Paddles 1312",
                       iconName: "Commodore Paddles 1312",
                       portIconName: "Commodore Paddles 1312 Single",
                       viceType: .paddles,
                       sensitivity: 180 / 135),
            
            Controller(identifier: "VIC-20 Paddles",
                       name: "VIC-20",
                       fullName: "Commodore VIC-20 Paddles",
                       iconName: "Commodore VIC-20 Paddles",
                       portIconName: "Commodore VIC-20 Paddles Single",
                       viceType: .paddles,
                       sensitivity: 180 / 100),

            Controller(identifier: "CX30",
                       name: "CX30",
                       fullName: "Atari Paddles CX30",
                       iconName: "Atari Paddles CX30",
                       portIconName: "Atari Paddles CX30 Single",
                       viceType: .paddles,
                       sensitivity: 180 / 90),

            Controller(identifier: "Elite 2002 Paddles",
                       name: "Elite 2002",
                       fullName: "Elite Multi-Function 2002",
                       variantName: "Paddles Mode",
                       iconName: "Elite Multi-Function 2002 Paddles",
                       viceType: .paddles,
                       sensitivity: 180 / 185) // logical: 185°, pysical: 280°
        ]),
        
        MachinePartSection(title: "Light Pen", parts: [
/*            Controller(identifier: "Inkwell 170C",
                       name: "Inkwell 170C",
                       fullName: "Inkwell Light Pen 170C",
                       iconName: "Inkwell Light Pen 170C",
                       viceType: .lightpenInkwell,
                       numberOfButtons: 1), */

            Controller(identifier: "Inkwell 184C",
                       name: "Inkwell 184C",
                       fullName: "Inkwell Light Pen 184C",
                       iconName: "Inkwell Light Pen 184C",
                       viceType: .lightpenInkwell),

            Controller(identifier: "Rex 9631",
                       name: "Rex 9631",
                       fullName: "Rex Light Pen 9631",
                       iconName: "Rex Light Pen 9631",
                       viceType: .lightpenL,
                       numberOfButtons: 0)
        ]),
        
        MachinePartSection(title: "Light Guns", parts: [
            Controller(identifier: "Sinclair Magnum",
                       name: "Magnum",
                       fullName: "Sinclair Magnum Light Phaser",
                       iconName: "Sinclair Magnum Light Phaser",
                       viceType: .lightgunY)
        ])
    ])
    
    public static var port2Controllers: MachinePartList {
        return controllers.filter({
            ($0 as? Controller)?.viceType.isPort2Compatible ?? false
        })
    }
    
    public static var userPortControllers: MachinePartList {
        return controllers.filter({
            ($0 as? Controller)?.viceType.isUserPortCompatible ?? false
        })
    }
    
    static private var byIdentifier = [String : Controller]()
    
    public static func controller(identifier: String) -> Controller? {
        if byIdentifier.isEmpty {
            for controller in controllers.parts {
                byIdentifier[controller.identifier] = controller as? Controller
            }
        }
        
        return byIdentifier[identifier]
    }
}

extension Controller: Equatable {
    
}
