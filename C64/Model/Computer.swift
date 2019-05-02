/*
 Computer.swift -- Computer Harware Part
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

struct Computer: MachinePart {
    enum ViceModel: String {
        case c64Pal
        case c64cPal
        case c64OldPal
        case c64Ntsc
        case c64cNtsc
        case c64OldNtsc
        case c64PalN
        case sx64Pal
        case sx64Ntsc
        case c64Japanese
        case c64Gs
        case pet64Pal
        case pet64Ntsc
        case ultimax

        var int32Value: Int32 {
            switch self {
            case .c64Pal:
                return 0
            case .c64cPal:
                return 1
            case .c64OldPal:
                return 2
            case .c64Ntsc:
                return 3
            case .c64cNtsc:
                return 4
            case .c64OldNtsc:
                return 5
            case .c64PalN:
                return 6
            case .sx64Pal:
                return 7
            case .sx64Ntsc:
                return 8
            case .c64Japanese:
                return 9
            case .c64Gs:
                return 10
            case .pet64Pal:
                return 11
            case .pet64Ntsc:
                return 12
            case .ultimax:
                return 13
            }
        }
    }
    
    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage? = nil
    var priority: Int
    
    var viceMachineModel: ViceModel
    var caseColor: UIColor
    var keyboard: Keyboard?
    var ports: Set<MachineConfig.Key>
    var drive8: DiskDrive?
    var chargenName: String
    
    var chargenUppercase: Chargen? {
        return Chargen(named: chargenName, bank: 0)
    }
    var chargenLowercase: Chargen? {
        return Chargen(named: chargenName, bank: 1)
    }

    init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String, priority: Int = MachinePartNormalPriority, viceMachineModel: ViceModel, keyboardName: String?, caseColorName: String, drive8: DiskDrive? = nil, missingPorts: Set<MachineConfig.Key> = [], chargenName: String = "chargen") {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        self.icon = UIImage(named: iconName)
        self.priority = priority
        self.viceMachineModel = viceMachineModel
        self.caseColor = UIColor(named: caseColorName) ?? UIColor.white
        if let keyboardName = keyboardName {
            self.keyboard = Keyboard.keyboard(named: keyboardName)
        }
        self.ports = [ .cassetteDrive, .controlPort1, .controlPort2, .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .userPort ]
        self.drive8 = drive8
        if drive8 != nil {
            self.ports.remove(.diskDrive8)
        }
        self.ports.subtract(missingPorts)
        self.chargenName = chargenName
    }
    
    static let c64Pal = Computer(identifier: "C64 PAL",
                                 name: "Commodore 64 (PAL)",
                                 fullName: "Commodore 64",
                                 variantName: "PAL",
                                 iconName: "Commodore 64 Original",
                                 viceMachineModel: .c64Pal,
                                 keyboardName: "C64 Keyboard",
                                 caseColorName: "C64 Case")
    
    static let computers = [
        c64Pal,
        
        Computer(identifier: "C64 NTSC",
                 name: "Commodore 64 (NTSC)",
                 fullName: "Commodore 64",
                 variantName: "NTSC",
                 iconName: "Commodore 64 Original",
                 viceMachineModel: .c64Ntsc,
                 keyboardName: "C64 Keyboard",
                 caseColorName: "C64 Case"),
        
        Computer(identifier: "C64 Japanese",
                 name: "Commodore 64 (Japanese)",
                 fullName: "Commodore 64",
                 variantName: "Japanese, NTSC",
                 iconName: "Commodore 64 Original",
                 viceMachineModel: .c64Japanese,
                 keyboardName: "C64 Keyboard Japanese",
                 caseColorName: "C64 Case",
                 chargenName: "jpchrgen"),
        
        Computer(identifier: "C64 VIC20 PAL",
                 name: "Commodore 64 (VIC-20 Style Keyboard, PAL)",
                 fullName: "Commodore 64",
                 variantName: "VIC-20 Style Keyboard, PAL",
                 iconName: "Commodore 64 VIC-20",
                 viceMachineModel: .c64Pal,
                 keyboardName: "VIC-20 Keyboard",
                 caseColorName: "C64 Case"),
        
        Computer(identifier: "C64 VIC20 NTSC",
                 name: "Commodore 64 (VIC-20 Style Keyboard, NTSC)",
                 fullName: "Commodore 64",
                 variantName: "VIC-20 Style Keyboard, NTSC",
                 iconName: "Commodore 64 VIC-20",
                 viceMachineModel: .c64Ntsc,
                 keyboardName: "VIC-20 Keyboard",
                 caseColorName: "C64 Case"),
        
        Computer(identifier: "C64 Silver PAL",
                 name: "Commodore 64 Silver (PAL)",
                 fullName: "Commodore 64 Silver",
                 variantName: "PAL",
                 iconName: "Commodore 64 Silver",
                 viceMachineModel: .c64OldPal,
                 keyboardName: "PET Style Keyboard",
                 caseColorName: "C64 Silver Case"),

        Computer(identifier: "C64 Silver NTSC",
                 name: "Commodore 64 Silver (NTSC)",
                 fullName: "Commodore 64 Silver",
                 variantName: "NTSC",
                 iconName: "Commodore 64 Silver",
                 viceMachineModel: .c64OldNtsc,
                 keyboardName: "PET Style Keyboard",
                 caseColorName: "C64 Silver Case"),
        
        Computer(identifier: "C64C PAL",
                 name: "Commodore 64C (Old Keyboard, PAL)",
                 fullName: "Commodore 64C",
                 variantName: "Old Keyboard, PAL",
                 iconName: "Commodore 64 C",
                 viceMachineModel: .c64Pal,
                 keyboardName: "C64C Keyboard",
                 caseColorName: "C64C Case"),
        
        Computer(identifier: "C64C NTSC",
                 name: "Commodore 64C (Old Keyboard, NTSC)",
                 fullName: "Commodore 64C",
                 variantName: "Old Keyboard, NTSC",
                 iconName: "Commodore 64 C",
                 viceMachineModel: .c64Ntsc,
                 keyboardName: "C64C Keyboard",
                 caseColorName: "C64C Case"),
        
        Computer(identifier: "C64C New PAL",
                 name: "Commodore 64C (New Keyboard, PAL)",
                 fullName: "Commodore 64C",
                 variantName: "New Keyboard, PAL",
                 iconName: "Commodore 64 C",
                 viceMachineModel: .c64cPal,
                 keyboardName: "C64C New Keyboard",
                 caseColorName: "C64C Case"),
        
        Computer(identifier: "C64C New NTSC",
                 name: "Commodore 64C (New Keyboard, NTSC)",
                 fullName: "Commodore 64C",
                 variantName: "New Keyboard, NTSC",
                 iconName: "Commodore 64 C",
                 viceMachineModel: .c64cNtsc,
                 keyboardName: "C64C New Keyboard",
                 caseColorName: "C64C Case"),

        Computer(identifier: "C64 Drean",
                 name: "Commodore 64 Drean",
                 fullName: "Commodore 64 Drean",
                 iconName: "Commodore 64 Drean",
                 viceMachineModel: .c64PalN,
                 keyboardName: "C64C Keyboard",
                 caseColorName: "C64C Case"),

        Computer(identifier: "SX64 PAL",
                 name: "CommodoreSX-64 (PAL)",
                 fullName: "Commodore SX-64",
                 variantName: "PAL",
                 iconName: "Commodore SX64",
                 viceMachineModel: .sx64Pal,
                 keyboardName: "SX64 Keyboard",
                 caseColorName: "SX64 Case",
                 drive8: DiskDrive.sx64,
                 missingPorts: [ .cassetteDrive ]),
        
        Computer(identifier: "SX64 NTSC",
                 name: "Commodore SX-64 (NTSC)",
                 fullName: "Commodore SX-64",
                 variantName: "NTSC",
                 iconName: "Commodore SX64",
                 viceMachineModel: .sx64Ntsc,
                 keyboardName: "SX64 Keyboard",
                 caseColorName: "SX64 Case",
                 drive8: DiskDrive.sx64,
                 missingPorts: [ .cassetteDrive ]),
        
        Computer(identifier: "Educator 64 PAL",
                 name: "Commodore Educator 64 (PAL)",
                 fullName: "Commodore Educator 64",
                 variantName: "PAL",
                 iconName: "Commodore Educator 64",
                 viceMachineModel: .pet64Pal,
                 keyboardName: "C64 Keyboard",
                 caseColorName: "Educator 64 Case"),
        
        Computer(identifier: "Educator 64 NTSC",
                 name: "Commodore Educator 64 (NTSC)",
                 fullName: "Commodore Educator 64",
                 variantName: "NTSC",
                 iconName: "Commodore Educator 64",
                 viceMachineModel: .pet64Ntsc,
                 keyboardName: "C64 Keyboard",
                 caseColorName: "Educator 64 Case"),
        
        Computer(identifier: "Max",
                 name: "Commodore Ultimax",
                 fullName: "Commodore Ultimax",
                 iconName: "Commodore Ultimax",
                 viceMachineModel: .ultimax,
                 keyboardName: "Max Keyboard",
                 caseColorName: "Max Case",
                 missingPorts: [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .userPort ]),
        
        Computer(identifier: "C64 GS",
                 name: "Commodore 64 Game System",
                 fullName: "Commodore 64 Game System",
                 iconName: "Commodore 64 Game System",
                 viceMachineModel: .c64Gs,
                 keyboardName: nil,
                 caseColorName: "C64 GS Case",
                 missingPorts: [ .cassetteDrive, .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .userPort ])
    ]
    
    static private var byIdentifier = [String: Computer]()
    
    static func computer(identifier: String) -> Computer? {
        if byIdentifier.isEmpty {
            for computer in computers {
                byIdentifier[computer.identifier] = computer
            }
        }
        
        return byIdentifier[identifier]
    }
}
