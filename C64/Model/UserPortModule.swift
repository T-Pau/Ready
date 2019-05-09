/*
 UserPortModule.swift -- User Port Module Harware Part
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

struct UserPortModule: MachinePart {
    enum ViceJoystickType: Int32 {
        case cga = 0
        case pet = 1
        case hummer = 2
        case oem = 3
        case hit = 4
        case kingsoft = 5
        case starbyte = 6
        
        var joystickPorts: Int {
            switch self {
            case .cga, .hit, .kingsoft, .pet, .starbyte:
                return 2
                
            case .hummer, .oem:
                return 1
            }
        }
    }

    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage?
    var priority: Int
    var viceJoystickType: ViceJoystickType?
    
    var joystickPorts: Int {
        if let type = viceJoystickType {
            return type.joystickPorts
        }
        else {
            return 0
        }
    }
    
    init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, viceJoystickType: ViceJoystickType? = nil) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.viceJoystickType = viceJoystickType
    }
    
    static let none = UserPortModule(identifier: "none", name: "None", iconName: nil)
    
    static let modules = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: [
            none
        ]),
        
        MachinePartSection(title: "Joystick Adapters", parts: [
            UserPortModule(identifier: "Protovision 4 Player Adapter",
                           name: "Protovision",
                           fullName: "Protovision 4 Player Adapter",
                           iconName: "Protovision 4 Player Adapter",
                           viceJoystickType: .cga),
            UserPortModule(identifier: "Singular 4 Player Adapter CGA",
                           name: "CGA",
                           fullName: "Singular Crew 4 Player Adapter",
                           variantName: "Classical Games / Protovision Mode",
                           iconName: "Singular Crew 4 Player Adapter (CGA)",
                           viceJoystickType: .cga),
            UserPortModule(identifier: "Singular 4 Player Adapter D&H",
                           name: "Hitmen",
                           fullName: "Singular Crew 4 Player Adapter",
                           variantName: "Digital Excess / Hitmen Mode",
                           iconName: "Singular Crew 4 Player Adapter (D&H)",
                           viceJoystickType: .hit),
            UserPortModule(identifier: "Singular 4 Player Adapter BBA",
                           name: "Kingsoft",
                           fullName: "Singular Crew 4 Player Adapter",
                           variantName: "Bug Bomber / Kingsoft Mode",
                           iconName: "Singular Crew 4 Player Adapter (BBA)",
                           viceJoystickType: .kingsoft)
        ])
    ])
    
    static private var byIdentifier = [String: UserPortModule]()
    
    static func module(identifier: String) -> UserPortModule? {
        if byIdentifier.isEmpty {
            for module in modules.parts {
                byIdentifier[module.identifier] = module as? UserPortModule
            }
        }
        
        return byIdentifier[identifier]
    }

}
