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

import UIKit

public struct Computer: MachinePart {
    public enum ComputerType {
        case c64
        case vic
        
        var dataDirectory: String {
            switch self {
            case .c64:
                return "vice/C64"
            case .vic:
                return "vice/VIC20"
            }
        }
        
        var ports: [Port] {
            switch self {
            case .c64:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.commodoreTape]),
                    Port(name: "Cartridge", key: .expansionPort, connectorTypes: [.c64ExpansionPort]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Control Port 1", key: .controlPort1, connectorTypes: [.atariJoystick, .atariJoystickAnalog, .c64JoystickLightpen]),
                    Port(name: "Control Port 2", key: .controlPort2, connectorTypes: [.atariJoystick, .atariJoystickAnalog])
                ]
                
            case .vic:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.commodoreTape]),
                    Port(name: "Cartridge", key: .expansionPort, connectorTypes: [.vic20ExpansionPort]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Control Port", key: .controlPort1, connectorTypes: [.atariJoystick, .atariJoystickAnalog]) // TODO: lightpen?
                ]
            }
        }
    }
    
    public enum ViceModel: String {
        case c64cNtsc
        case c64cPal
        case c64Japanese
        case c64Gs
        case c64Ntsc
        case c64OldNtsc
        case c64OldPal
        case c64Pal
        case c64PalN
        case pet64Ntsc
        case pet64Pal
        case sx64Pal
        case sx64Ntsc
        case ultimax
        case vic1001
        case vic20Pal
        case vic20Ntsc

        public var viceMachine: ComputerType {
            switch self {
            case .c64cNtsc, .c64cPal, .c64Japanese, .c64Gs, .c64Ntsc, .c64OldNtsc, .c64OldPal, .c64Pal, .c64PalN, .pet64Ntsc, .pet64Pal, .sx64Ntsc, .sx64Pal, .ultimax:
                return .c64
            case .vic1001, .vic20Pal, .vic20Ntsc:
                return .vic
            }
        }
        public var int32Value: Int32 {
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
                
            case .vic20Pal:
                return 0
            case .vic20Ntsc:
                return 1
            case .vic1001:
                return 3
            }
        }
    }
    
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage? = nil
    public var priority: Int
    
    public var connector: ConnectorType { return .computer }
    public var ports: [Port]

    public var viceMachineModel: ViceModel
    public var caseColor: UIColor
    public var keyboard: Keyboard?
    public var drive8: DiskDrive?
    public var chargenName: String
    
    public var chargenUppercase: Chargen? {
        return Chargen(named: chargenName, subdirectory: viceMachineModel.viceMachine.dataDirectory, bank: 0)
    }
    public var chargenLowercase: Chargen? {
        return Chargen(named: chargenName, subdirectory: viceMachineModel.viceMachine.dataDirectory, bank: 1)
    }

    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String, priority: Int = MachinePartNormalPriority, viceMachineModel: ViceModel, keyboardName: String?, caseColorName: String, drive8: DiskDrive? = nil, missingPorts: Set<MachineConfig.Key> = [], chargenName: String = "chargen") {
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
        self.ports = viceMachineModel.viceMachine.ports

        self.drive8 = drive8
        if drive8 != nil {
            ports.removeAll(where: { $0.key == .diskDrive8 })
        }
        for port in missingPorts {
            ports.removeAll(where: { $0.key == port })
        }
        self.chargenName = chargenName
    }
    
    public static let c64Pal = Computer(identifier: "C64 PAL",
                                 name: "Commodore 64 (PAL)",
                                 fullName: "Commodore 64",
                                 variantName: "PAL",
                                 iconName: "Commodore 64 Original",
                                 viceMachineModel: .c64Pal,
                                 keyboardName: "C64 Keyboard",
                                 caseColorName: "C64 Case")
    
    public static let computers = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: []),
        
        MachinePartSection(title: "Commodore 64", parts: [
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
                     variantName: "Japanese",
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
        ]),
        MachinePartSection(title: "Commdore 64 C", parts: [
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
        ]),
        
        MachinePartSection(title: "Other Commodore 64 Variants", parts: [
            Computer(identifier: "SX64 PAL",
                     name: "Commodore SX-64 (PAL)",
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
            ]),
        
        MachinePartSection(title: "Commodore VIC-20", parts: [
            Computer(identifier: "VIC-20 PAL",
                     name: "Commodore VIC-20 (PAL)",
                     fullName: "Commodore VIC-20",
                     variantName: "PAL",
                     iconName: "Commodore 64 VIC-20", // TODO: own icon
                     viceMachineModel: .vic20Pal,
                     keyboardName: "VIC-20 Keyboard",
                     caseColorName: "C64 Silver Case"),
            
            Computer(identifier: "VIC-20 NTSC",
                     name: "Commodore VIC-20 (NTSC)",
                     fullName: "Commodore VIC-20",
                     variantName: "NTSC",
                     iconName: "Commodore 64 VIC-20", // TODO: own icon
                     viceMachineModel: .vic20Ntsc,
                     keyboardName: "VIC-20 Keyboard",
                     caseColorName: "C64 Silver Case"),
            
            Computer(identifier: "VIC-1001",
                     name: "Commodore VIC-1001",
                     variantName: "Japanese",
                     iconName: "VIC-1001",
                     viceMachineModel: .vic1001,
                     keyboardName: "VIC-1001 Keyboard",
                     caseColorName: "C64 Silver Case")
        ])
   ])
    
    static private var byIdentifier = [String: Computer]()
    
    public static func computer(identifier: String) -> Computer? {
        if byIdentifier.isEmpty {
            for computer in computers.parts {
                byIdentifier[computer.identifier] = computer as? Computer
            }
        }
        
        return byIdentifier[identifier]
    }
}
