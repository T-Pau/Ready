/*
 Computer.swift -- Computer Harware Part
 Copyright (C) 2019 Dieter Baron
 
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

import UIKit
import RetroMedia

public struct Computer: MachinePart {
    public enum ComputerType {
        case atari8Bit
        case c64
        case c128
        case plus4
        case spectrum
        case vic
        case x16
        
        var dataDirectory: String {
            switch self {
            case .atari8Bit:
                return "atari800"
            case .c64, .c128, .plus4: // TODO: Plus4 actually uses two directories
                return "vice/C64"
            case .spectrum:
                return "fuse"
            case .vic:
                return "vice/VIC20"
            case .x16:
                return "x16-emulator"
            }
        }
        
        var supportedMediaTypes: Set<MediaType> {
            switch self {
            case .atari8Bit:
                return [] // TODO
            case .c64:
                return [
                    .cartridgeCommodore64,
                    .cassetteCommodore,
                    .compactFlashIde64,
                    .cd,
                    .disk3_5DoubleDensityCmd,
                    .disk3_5DoubleDensityCommodore,
                    .disk3_5HighDensityCmd,
                    .disk3_5HighDensityCmd,
                    .disk5_25SingleDensitySingleSidedCommodore,
                    .disk5_25SingleDensityDoubleSidedCommodore,
                    .disk5_25DoubleDensitySingleSidedCommodore,
                    .harddiskIdeIde64
                ]

            case .c128:
                return [
                    .cartridgeCommodore64,
                    .cassetteCommodore,
                    .compactFlashIde64,
                    .cd,
                    .disk3_5DoubleDensityCmd,
                    .disk3_5DoubleDensityCommodore,
                    .disk3_5HighDensityCmd,
                    .disk3_5HighDensityCmd,
                    .disk5_25SingleDensitySingleSidedCommodore,
                    .disk5_25SingleDensityDoubleSidedCommodore,
                    .disk5_25DoubleDensitySingleSidedCommodore,
                    .harddiskIdeIde64
                ] // TODO

            case .plus4:
                return [
                    .cartridgeCommodorePlus4,
                    .cassetteCommodore,
                    .disk3_5DoubleDensityCmd,
                    .disk3_5DoubleDensityCommodore,
                    .disk3_5HighDensityCmd,
                    .disk3_5HighDensityCmd,
                    .disk5_25SingleDensitySingleSidedCommodore,
                    .disk5_25SingleDensityDoubleSidedCommodore,
                    .disk5_25DoubleDensitySingleSidedCommodore,
                ]

            case .spectrum:
                return [
                    .cassetteSpectrum
                ]

            case .vic:
                return [
                    .cartridgeCommodoreVIC20,
                    .cassetteCommodore,
                    .disk3_5DoubleDensityCmd,
                    .disk3_5DoubleDensityCommodore,
                    .disk3_5HighDensityCmd,
                    .disk3_5HighDensityCmd,
                    .disk5_25SingleDensitySingleSidedCommodore,
                    .disk5_25SingleDensityDoubleSidedCommodore,
                    .disk5_25DoubleDensitySingleSidedCommodore,
                ]
                
            case .x16:
                return [] // TODO
            }
        }
        
        var ports: [Port] {
            switch self {
            case .atari8Bit:
                return [] // TODO
            case .c64:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], supportsHotSwap: true, iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.commodoreTapePort]),
                    Port(name: "Cartridge", key: .expansionPort, connectorTypes: [.c64ExpansionPort, .media(.cartridgeCommodore64)]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Control Port 1", key: .controlPort1, connectorTypes: [.atariJoystick, .atariJoystickAnalog, .c64JoystickLightpen], supportsHotSwap: true),
                    Port(name: "Control Port 2", key: .controlPort2, connectorTypes: [.atariJoystick, .atariJoystickAnalog], supportsHotSwap: true)
                ]

            case .c128:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], supportsHotSwap: true, iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.commodoreTapePort]),
                    Port(name: "Cartridge", key: .expansionPort, connectorTypes: [.c64ExpansionPort, .media(.cartridgeCommodore64)]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Control Port 1", key: .controlPort1, connectorTypes: [.atariJoystick, .atariJoystickAnalog, .c64JoystickLightpen], supportsHotSwap: true),
                    Port(name: "Control Port 2", key: .controlPort2, connectorTypes: [.atariJoystick, .atariJoystickAnalog], supportsHotSwap: true)
                ]

            case .plus4:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], supportsHotSwap: true, iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.plus4TapePort]),
                    Port(name: "Memory Expansion", key: .expansionPort, connectorTypes: [.plus4ExpansionPort, .media(.cartridgeCommodorePlus4)]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2), // TODO: 1551
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Joy 0", key: .controlPort1, connectorTypes: [.atariJoystick], supportsHotSwap: true),
                    Port(name: "Joy 1", key: .controlPort2, connectorTypes: [.atariJoystick], supportsHotSwap: true)
                ]
                
            case .spectrum:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], supportsHotSwap: true, iconWidth: 2, iconHeight: 2),
                    Port(name: "Mic/Ear", key: .cassetteDrive, connectorTypes: [.audioJackTape]),
                    Port(name: "Expansion", key: .expansionPort, connectorTypes: [.spectrumExpansionPort]),
                ]

            case .vic:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoComponent], supportsHotSwap: true, iconWidth: 2, iconHeight: 2),
                    Port(name: "User Port", key: .userPort, connectorTypes: [.c64UserPort]),
                    Port(name: "Cassette", key: .cassetteDrive, connectorTypes: [.commodoreTapePort]),
                    Port(name: "Cartridge", key: .expansionPort, connectorTypes: [.vic20ExpansionPort, .media(.cartridgeCommodoreVIC20)]),
                    Port(name: "Disk Drive 8", key: .diskDrive8, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 9", key: .diskDrive9, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 10", key: .diskDrive10, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Disk Drive 11", key: .diskDrive11, connectorTypes: [.commodoreIEC], iconWidth: 2),
                    Port(name: "Control Port", key: .controlPort1, connectorTypes: [.atariJoystick, .atariJoystickAnalog], supportsHotSwap: true) // TODO: lightpen
                ]
                
            case .x16:
                return [
                    Port(name: "Screen", key: .screen, connectorTypes: [.videoVGA], supportsHotSwap: true, iconWidth: 2, iconHeight:  2),
                    Port(name: "Joystick 1", key: .controlPort1, connectorTypes: [.supernesJoystick], supportsHotSwap: true),
                    Port(name: "Joystick 2", key: .controlPort2, connectorTypes: [.supernesJoystick], supportsHotSwap: true),
                ] // TODO
            }
        }
    }
    
    public enum Model: String {
        case atari600Xl
        case atari800Xl
        case c16Pal
        case c16Ntsc
        case c64cNtsc
        case c64cPal
        case c64Japanese
        case c64Gs
        case c64Ntsc
        case c64OldNtsc
        case c64OldPal
        case c64Pal
        case c64PalN
        case c128DcrPal
        case c128DcrNtsc
        case c128Pal
        case c128Ntsc
        case c232Ntsc
        case pet64Ntsc
        case pet64Pal
        case plus4Pal
        case plus4Ntsc
        case sx64Pal
        case sx64Ntsc
        case ultimax
        case v364Ntsc
        case vic1001
        case vic20Pal
        case vic20Ntsc
        case x16
        case zxSpectrum16k
        case zxSpectrum48k
        case zxSpectrum48kNtsc
        case zxSpectrum128k
        case zxSpectrumPlus2

        public var type: ComputerType {
            switch self {
            case .atari600Xl, .atari800Xl:
                return .atari8Bit
            case .c64cNtsc, .c64cPal, .c64Japanese, .c64Gs, .c64Ntsc, .c64OldNtsc, .c64OldPal, .c64Pal, .c64PalN, .pet64Ntsc, .pet64Pal, .sx64Ntsc, .sx64Pal, .ultimax:
                return .c64
            case .c128DcrPal, .c128DcrNtsc, .c128Pal, .c128Ntsc:
                return .c128
            case .c16Pal, .c16Ntsc, .c232Ntsc, .plus4Pal, .plus4Ntsc, .v364Ntsc:
                return .plus4
            case .vic1001, .vic20Pal, .vic20Ntsc:
                return .vic
            case .x16:
                return .x16
            case .zxSpectrum16k, .zxSpectrum48k, .zxSpectrum48kNtsc, .zxSpectrum128k, .zxSpectrumPlus2:
                return .spectrum
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

    public var model: Model
    public var caseColor: UIColor
    public var keyboard: Keyboard?
    public var drive8: DiskDrive?
    public var chargenName: String
    
    private var emulatorInfo: EmulatorInfo?
    
    public func emulatorInfo(for type: ComputerType) -> EmulatorInfo? {
        guard type == model.type else { return nil }
        return emulatorInfo
    }
    
    public var chargenUppercase: Chargen? {
        return Chargen(named: chargenName, subdirectory: model.type.dataDirectory, bank: 0)
    }
    public var chargenLowercase: Chargen? {
        return Chargen(named: chargenName, subdirectory: model.type.dataDirectory, bank: 1)
    }

    public init(identifier: String, name: String? = nil, fullName: String, variantName: String? = nil, iconName: String, priority: Int = MachinePartNormalPriority, viceMachineModel: Model, keyboardName: String?, caseColorName: String, drive8: DiskDrive? = nil, missingPorts: Set<MachineConfig.Key> = [], additionalPorts: [Port]? = nil, emulatorInfo: EmulatorInfo? = nil, chargenName: String = "chargen") {
        self.identifier = identifier
        if let name = name {
            self.name = name
        }
        else {
            if let variantName = variantName {
                self.name = "fullName (\(variantName))"
            }
            else {
                self.name = fullName
            }
        }
        self.fullName = fullName
        self.variantName = variantName
        self.icon = UIImage(named: iconName)
        self.priority = priority
        self.model = viceMachineModel
        self.caseColor = UIColor(named: caseColorName) ?? UIColor.white
        if let keyboardName = keyboardName {
            self.keyboard = Keyboard.keyboard(named: keyboardName)
        }
        self.ports = viceMachineModel.type.ports

        self.drive8 = drive8
        if drive8 != nil {
            ports.removeAll(where: { $0.key == .diskDrive8 })
        }
        for port in missingPorts {
            ports.removeAll(where: { $0.key == port })
        }
        if let additionalPorts = additionalPorts {
            ports.append(contentsOf: additionalPorts)
        }
        self.emulatorInfo = emulatorInfo
        self.chargenName = chargenName
    }
    
    public static let c64Pal = Computer(identifier: "C64 PAL",
                                 name: "Commodore 64 (PAL)",
                                 fullName: "Commodore 64",
                                 variantName: "PAL",
                                 iconName: "Commodore 64 Original",
                                 viceMachineModel: .c64Pal,
                                 keyboardName: "C64",
                                 caseColorName: "C64 Case")
    
    public static let computers = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: []),
        
        MachinePartSection(title: "Commodore 64", parts: [
            c64Pal,
            
            Computer(identifier: "C64 NTSC",
                     fullName: "Commodore 64",
                     variantName: "NTSC",
                     iconName: "Commodore 64 Original",
                     viceMachineModel: .c64Ntsc,
                     keyboardName: "C64",
                     caseColorName: "C64 Case"),
            
            Computer(identifier: "C64 Japanese",
                     fullName: "Commodore 64",
                     variantName: "Japanese",
                     iconName: "Commodore 64 Original",
                     viceMachineModel: .c64Japanese,
                     keyboardName: "C64 Japanese",
                     caseColorName: "C64 Case",
                     chargenName: "jpchrgen"),
            
            Computer(identifier: "C64 VIC20 PAL",
                     fullName: "Commodore 64",
                     variantName: "VIC-20 Style Keyboard, PAL",
                     iconName: "Commodore 64 VIC-20",
                     viceMachineModel: .c64Pal,
                     keyboardName: "VIC-20",
                     caseColorName: "C64 Case"),
            
            Computer(identifier: "C64 VIC20 NTSC",
                     fullName: "Commodore 64",
                     variantName: "VIC-20 Style Keyboard, NTSC",
                     iconName: "Commodore 64 VIC-20",
                     viceMachineModel: .c64Ntsc,
                     keyboardName: "VIC-20",
                     caseColorName: "C64 Case"),
            
            Computer(identifier: "C64 Silver PAL",
                     fullName: "Commodore 64 Silver",
                     variantName: "PAL",
                     iconName: "Commodore 64 Silver",
                     viceMachineModel: .c64OldPal,
                     keyboardName: "PET Style",
                     caseColorName: "VIC-20 Case"),
            
            Computer(identifier: "C64 Silver NTSC",
                     fullName: "Commodore 64 Silver",
                     variantName: "NTSC",
                     iconName: "Commodore 64 Silver",
                     viceMachineModel: .c64OldNtsc,
                     keyboardName: "PET Style",
                     caseColorName: "VIC-20 Case"),
        ]),
        
        MachinePartSection(title: "Commdore 64 C", parts: [
            Computer(identifier: "C64C PAL",
                     fullName: "Commodore 64C",
                     variantName: "Old Keyboard, PAL",
                     iconName: "Commodore 64 C",
                     viceMachineModel: .c64Pal,
                     keyboardName: "C64C",
                     caseColorName: "C64C Case"),
            
            Computer(identifier: "C64C NTSC",
                     fullName: "Commodore 64C",
                     variantName: "Old Keyboard, NTSC",
                     iconName: "Commodore 64 C",
                     viceMachineModel: .c64Ntsc,
                     keyboardName: "C64C",
                     caseColorName: "C64C Case"),
            
            Computer(identifier: "C64C New PAL",
                     fullName: "Commodore 64C",
                     variantName: "New Keyboard, PAL",
                     iconName: "Commodore 64 C",
                     viceMachineModel: .c64cPal,
                     keyboardName: "C64C New",
                     caseColorName: "C64C Case"),
            
            Computer(identifier: "C64C New NTSC",
                     fullName: "Commodore 64C",
                     variantName: "New Keyboard, NTSC",
                     iconName: "Commodore 64 C",
                     viceMachineModel: .c64cNtsc,
                     keyboardName: "C64C New",
                     caseColorName: "C64C Case"),
            
            Computer(identifier: "C64 Drean",
                     fullName: "Commodore 64 Drean",
                     iconName: "Commodore 64 Drean",
                     viceMachineModel: .c64PalN,
                     keyboardName: "C64C",
                     caseColorName: "C64C Case"),
        ]),
        
        MachinePartSection(title: "Other Commodore 64 Variants", parts: [
            Computer(identifier: "SX64 PAL",
                     fullName: "Commodore SX-64",
                     variantName: "PAL",
                     iconName: "Commodore SX64",
                     viceMachineModel: .sx64Pal,
                     keyboardName: "SX64",
                     caseColorName: "SX64 Case",
                     drive8: DiskDrive.sx64,
                     missingPorts: [ .cassetteDrive ]),
            
            Computer(identifier: "SX64 NTSC",
                     fullName: "Commodore SX-64",
                     variantName: "NTSC",
                     iconName: "Commodore SX64",
                     viceMachineModel: .sx64Ntsc,
                     keyboardName: "SX64",
                     caseColorName: "SX64 Case",
                     drive8: DiskDrive.sx64,
                     missingPorts: [ .cassetteDrive ]),
            
            Computer(identifier: "Educator 64 PAL",
                     fullName: "Commodore Educator 64",
                     variantName: "PAL",
                     iconName: "Commodore Educator 64",
                     viceMachineModel: .pet64Pal,
                     keyboardName: "C64",
                     caseColorName: "Educator 64 Case"),
            
            Computer(identifier: "Educator 64 NTSC",
                     fullName: "Commodore Educator 64",
                     variantName: "NTSC",
                     iconName: "Commodore Educator 64",
                     viceMachineModel: .pet64Ntsc,
                     keyboardName: "C64",
                     caseColorName: "Educator 64 Case"),
            
            Computer(identifier: "Max",
                     fullName: "Commodore Ultimax",
                     iconName: "Commodore Ultimax",
                     viceMachineModel: .ultimax,
                     keyboardName: "Max",
                     caseColorName: "Max Case",
                     missingPorts: [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .userPort ]),
            
            Computer(identifier: "C64 GS",
                     fullName: "Commodore 64 Game System",
                     iconName: "Commodore 64 Game System",
                     viceMachineModel: .c64Gs,
                     keyboardName: nil,
                     caseColorName: "C64 GS Case",
                     missingPorts: [ .cassetteDrive, .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .userPort ])
            ]),
        
/*
        MachinePartSection(title: "Commodore 128", parts: [
            Computer(identifier: "C128 DE PAL",
                     fullName: "Commodore 128",
                     variantName: "German Keyboard, PAL",
                     iconName: "Commodore 128",
                     viceMachineModel: .c128Pal,
                     keyboardName: "C128 DE",
                     caseColorName: "C64C Case"),
        ]),
*/

        MachinePartSection(title: "Commodore 16, Plus/4", parts: [
            Computer(identifier: "C16 PAL",
                     fullName: "Commodore 16",
                     variantName: "PAL",
                     iconName: "Commodore C16",
                     viceMachineModel: .c16Pal,
                     keyboardName: "C16",
                     caseColorName: "C16 Case"),
            
            Computer(identifier: "C16 NTSC",
                     fullName: "Commodore 16",
                     variantName: "NTSC",
                     iconName: "Commodore C16",
                     viceMachineModel: .c16Ntsc,
                     keyboardName: "C16",
                     caseColorName: "C16 Case"),
            
            Computer(identifier: "Plus/4 PAL",
                     fullName: "Commodore Plus/4",
                     variantName: "PAL",
                     iconName: "Commodore Plus 4",
                     viceMachineModel: .plus4Pal,
                     keyboardName: "Plus/4",
                     caseColorName: "C16 Case"),

            Computer(identifier: "Plus/4 NTSC",
                     fullName: "Commodore Plus/4",
                     variantName: "NTSC",
                     iconName: "Commodore Plus 4",
                     viceMachineModel: .plus4Ntsc,
                     keyboardName: "Plus/4",
                     caseColorName: "C16 Case")
        ]),
        
        MachinePartSection(title: "Commodore VIC-20", parts: [
            Computer(identifier: "VIC-20 PAL",
                     fullName: "Commodore VIC-20",
                     variantName: "PAL",
                     iconName: "Commodore VIC-20",
                     viceMachineModel: .vic20Pal,
                     keyboardName: "VIC-20",
                     caseColorName: "VIC-20 Case"),
            
            Computer(identifier: "VIC-20 NTSC",
                     fullName: "Commodore VIC-20",
                     variantName: "NTSC",
                     iconName: "Commodore VIC-20",
                     viceMachineModel: .vic20Ntsc,
                     keyboardName: "VIC-20",
                     caseColorName: "VIC-20 Case"),
            
            Computer(identifier: "VIC-1001",
                     name: "Commodore VIC-1001",
                     fullName: "Commodore VIC-1001",
                     variantName: "Japanese",
                     iconName: "Commodore VIC-1001",
                     viceMachineModel: .vic1001,
                     keyboardName: "VIC-1001",
                     caseColorName: "VIC-20 Case",
                     chargenName: "jpchrgen"),
            
            Computer(identifier: "VIC-20 PET PAL",
                     fullName: "Commodore VIC-20",
                     variantName: "PET Style Keyboard, PAL",
                     iconName: "Commodore VIC-20",
                     viceMachineModel: .vic20Pal,
                     keyboardName: "PET Style",
                     caseColorName: "VIC-20 Case"),
            
            Computer(identifier: "VIC-20 PET NTSC",
                     fullName: "Commodore VIC-20",
                     variantName: "PET Style Keyboard, NTSC",
                     iconName: "Commodore VIC-20",
                     viceMachineModel: .vic20Ntsc,
                     keyboardName: "PET Style",
                     caseColorName: "VIC-20 Case"),

            Computer(identifier: "VIC-20 C64 PAL",
                     fullName: "Commodore VIC-20",
                     variantName: "C64 Style Keyboard, PAL",
                     iconName: "Commodore VIC-20 C64",
                     viceMachineModel: .vic20Pal,
                     keyboardName: "C64",
                     caseColorName: "VIC-20 Case"),

            Computer(identifier: "VIC-20 C64 NTSC",
                     fullName: "Commodore VIC-20",
                     variantName: "C64 Style Keyboard, NTSC",
                     iconName: "Commodore VIC-20 C64",
                     viceMachineModel: .vic20Ntsc,
                     keyboardName: "C64",
                     caseColorName: "VIC-20 Case")
        ]),
        
        MachinePartSection(title: "Sinclair ZX Spectrum", parts: [
            Computer(identifier: "ZX 16k",
                     fullName: "Sinclair ZX Spectrum",
                     variantName: "16k, PAL",
                     iconName: "Sinclair ZX Spectrum",
                     viceMachineModel: .zxSpectrum16k,
                     keyboardName: "ZX Spectrum",
                     caseColorName: "ZX Case"),
            
            Computer(identifier: "ZX 48k",
                     fullName: "Sinclair ZX Spectrum",
                     variantName: "48k, PAL",
                     iconName: "Sinclair ZX Spectrum",
                     viceMachineModel: .zxSpectrum48k,
                     keyboardName: "ZX Spectrum",
                     caseColorName: "ZX Case"),

            Computer(identifier: "ZX 48k NTSC",
                     fullName: "Sinclair ZX Spectrum",
                     variantName: "48k, NTSC",
                     iconName: "Sinclair ZX Spectrum",
                     viceMachineModel: .zxSpectrum48kNtsc,
                     keyboardName: "ZX Spectrum",
                     caseColorName: "ZX Case"),
            
            Computer(identifier: "ZX+",
                     fullName: "Sinclair ZX Spectrum+",
                     variantName: "PAL",
                     iconName: "Sinclair ZX Spectrum Plus",
                     viceMachineModel: .zxSpectrum48k,
                     keyboardName: "ZX Spectrum+",
                     caseColorName: "ZX Plus Case"),

            Computer(identifier: "ZX 128k",
                     fullName: "Sinclair ZX Spectrum 128k",
                     variantName: "PAL",
                     iconName: "Sinclair ZX Spectrum 128k",
                     viceMachineModel: .zxSpectrum128k,
                     keyboardName: "ZX Spectrum+",
                     caseColorName: "ZX Plus Case"),

            Computer(identifier: "ZX +2",
                     fullName: "Sinclair ZX Spectrum +2",
                     variantName: "PAL",
                     iconName: "Sinclair ZX Spectrum +2",
                     viceMachineModel: .zxSpectrumPlus2,
                     keyboardName: "ZX Spectrum +2",
                     caseColorName: "ZX +2 Case",
                     missingPorts: [.cassetteDrive],
                     additionalPorts: [
//                        Port(name: "Cassete", key: .cassette, connectorTypes: [.media(.cassetteSpectrum)], supportsHotSwap: true),
                        Port(name: "Joystick 1", key: .controlPort1, connectorTypes: [.atariJoystick], supportsHotSwap: true),
                        Port(name: "Joystick 2", key: .controlPort2, connectorTypes: [.atariJoystick], supportsHotSwap: true),
            ]),

        ]),

        MachinePartSection(title: "Atari 8-Bit Computers", parts: [
            Computer(identifier: "Atari 600XL",
                     fullName: "Atari 600XL",
                     variantName: "PAL",
                     iconName: "Atari 600XL",
                     viceMachineModel: .atari600Xl,
                     keyboardName: "Atari XL",
                     caseColorName: "Atari XL Case",
                     emulatorInfo: Atari800EmulatorInfo(xlOS: "Atari 600XL OS rev. 1", basicRom: "Atari BASIC rev. B"),
                     chargenName: ""),

            Computer(identifier: "Atari 600XL NTSC",
                     fullName: "Atari 600XL",
                     variantName: "NTSC",
                     iconName: "Atari 600XL",
                     viceMachineModel: .atari600Xl,
                     keyboardName: "Atari XL",
                     caseColorName: "Atari XL Case",
                     emulatorInfo: Atari800EmulatorInfo(xlOS: "Atari 600XL OS rev. 1", basicRom: "Atari BASIC rev. B", ntsc: true),
                     chargenName: ""),

            Computer(identifier: "Atari 800XL",
                     fullName: "Atari 800XL",
                     variantName: "PAL",
                     iconName: "Atari 800XL",
                     priority: MachinePartHighPriority,
                     viceMachineModel: .atari800Xl,
                     keyboardName: "Atari XL",
                     caseColorName: "Atari XL Case",
                     emulatorInfo: Atari800EmulatorInfo(xlOS: "Atari XL-XE OS rev. 2", basicRom: "Atari BASIC rev. B"),
                     chargenName: ""),
            
            Computer(identifier: "Atari 800XL NTSC",
                     name: "Atari 800XL",
                     fullName: "Atari 800XL",
                     variantName: "NTSC",
                     iconName: "Atari 800XL",
                     viceMachineModel: .atari800Xl,
                     keyboardName: "Atari XL",
                     caseColorName: "Atari XL Case",
                     emulatorInfo: Atari800EmulatorInfo(xlOS: "Atari XL-XE OS rev. 2", basicRom: "Atari BASIC rev. B", ntsc: true),
                     chargenName: "")
        ]),
        
        MachinePartSection(title: "Commander X16", parts: [
            Computer(identifier: "X16",
                     fullName: "Commander X16",
                     iconName: "Commander X16",
                     viceMachineModel: .x16,
                     keyboardName: "X16",
                     caseColorName: "X16 Case",
                     chargenName: "")
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
