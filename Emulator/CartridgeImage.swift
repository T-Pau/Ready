/*
 CartridgeImage.swift -- Access Cartridge Images
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

public struct CartridgeImage {
    public enum Format {
        case crt(type: CrtType)
        case c64
        case gmod2(eepromUrl: URL?)
        case vic20(address: Int)
        
        var connectorType: ConnectorType {
            switch self {
            case .crt(_), .c64, .gmod2(_):
                return .c64ExpansionPort
            case .vic20(_):
                return .vic20ExpansionPort
            }
        }
    }
    
    public enum CrtType: Int {
        case generic
        case actionReplay5 = 1
        case kcsPower = 2
        case finalCartridge3 = 3
        case simonsBasic = 4
        case oceanType1 = 5
        case expertCartridge = 6
        case funPlay = 7
        case superGames = 8
        case atomicPower = 9
        case epyxFastload = 10
        case westermannLearning = 11
        case rexUtility = 12
        case finalCartridge1 = 13
        case magicFormel = 14
        case gameSystem = 15
        case warpSpeed = 16
        case dinamic = 17
        case zaxxon = 18
        case magicDesk = 19
        case superSnapshot5 = 20
        case comal80 = 21
        case structuredBasic = 22
        case ross = 23 // TODO
        case delaEp64 = 24 // TODO
        case delaEp7x8 = 25 // TODO
        case delaEp256 = 26 // TODO
        case rexEp256 = 27 // TODO
        case mikroAssembler = 28
        case finalCartridgePlus = 29
        case actionReplay4 = 30
        case starDos = 31
        case easyFlash = 32
        case easyFlashXbank = 33
        case capture = 34
        case actionReplay3 = 35
        case retroReplay = 36
        case mmc64 = 37 // TODO
        case mmcReplay = 38 // TODO
        case ide64 = 39
        case superSnapshot4 = 40
        case ieee488 = 41
        case gameKiller = 42
        case prophet64 = 43
        case exos = 44 // TODO
        case freezeFrame = 45
        case freezeMachine = 46
        case snapshot64 = 47
        case superExplide5 = 48
        case magicVoice = 49
        case actionReplay2 = 50
        case mach5 = 51
        case diashowMaker = 52
        case pagefox = 53
        case kingsoft = 54 // TODO
        case silverrock128 = 55
        case formel64 = 56
        case rgcd = 57 // TOOD
        case easyCalc = 58
        case gmod2 = 60
        case maxBasic = 61
        
        public var hasFreeze: Bool {
            switch self {
            case .actionReplay5, .actionReplay2, .actionReplay3, .actionReplay4, .atomicPower, .capture, .diashowMaker, .expertCartridge, .finalCartridge1, .finalCartridge3, .finalCartridgePlus, .freezeFrame, .freezeMachine, .gameKiller, .kcsPower, .magicFormel, .mmcReplay, .retroReplay, .snapshot64, .superSnapshot4, .superSnapshot5:
                return true
                
            default:
                return false
            }
            
        }
    }
    public var bytes: Data
    public var romSize: Int
    
    public var format: Format
    public var title: String?
    public var url: URL?
    
    public var eepromUrl: URL? {
        switch format {
        case .gmod2(let eepromUrl):
            return eepromUrl
            
        default:
            return nil
        }
    }
    
    public var hasFreeze: Bool {
        switch format {
        case .crt(type: let type):
            return type.hasFreeze
            
        default:
            return false
        }
    }
    
    public init?(directory: URL, file: String?, eeprom: String? = nil) {
        guard let file = file else { return nil }
        let fileUrl = directory.appendingPathComponent(file)
        
        self.init(url: fileUrl)

        if let eeprom = eeprom {
            format = .gmod2(eepromUrl: directory.appendingPathComponent(eeprom))
        }
    }

    public init?(url: URL) {
        guard let bytes = FileManager.default.contents(atPath: url.path) else { return nil }
        self.init(bytes: bytes)
        self.url = url
    }
    
    public init?(bytes: Data) {
        romSize = bytes.count
        title = nil
                
        if romSize > 0x40 && String(bytes: bytes[0...15], encoding: .ascii) == "C64 CARTRIDGE   " {
            title = String(bytes: bytes[0x20...0x3f], encoding: .ascii)
            if title?.isEmpty ?? false {
                title = nil
            }
            format = .crt(type: CrtType(rawValue: Int(bytes[0x16]) << 8 | Int(bytes[0x17])) ?? .generic)
            // TODO: romSize is incorrect, but currently unused for crt
        }
        else if let vic20Format = CartridgeImage.matchVic20(bytes: bytes) {
            if romSize % 0x1000 == 2 {
                romSize -= 2 // Account for laod address, if present.
            }
            format = vic20Format
        }
        else {
            // TODO: Check for C64 signature.
            format = .c64
        }
        
        self.bytes = bytes
        url = nil
    }
    
    private struct Signature {
        var address: Int
        var bytes: [UInt8]
        
        func matches(rom: Data, loadAddress: Int, offset: Int) -> Bool {
            let endAdress = loadAddress + rom.count - offset
            if loadAddress > address || endAdress < address {
                return false
            }
            
            for i in 0 ..< bytes.count {
                if rom[offset + address - loadAddress + i] != bytes[i] {
                    return false
                }
            }
            
            return true
        }
    }
    private static let vic20Signature = Signature(address: 0xa004, bytes: [0x41, 0x30, 0xc3, 0xc2, 0xcd])
    private static let c64Signature = Signature(address: 0xa003, bytes: [0xc3, 0xc2, 0xcd, 0x38, 0x30])

    private static func matchVic20(bytes: Data) -> Format? {
        var loadAddress = 0xa000
        var romSize = bytes.count
        var offset = 0
        
        if romSize % 0x1000 == 2 {
            loadAddress = Int(bytes[0]) | Int(bytes[1]) << 8
            romSize -= 2
            offset = 2
        }
        
        var matches = false
        
        switch loadAddress {
        case 0x2000, 0x4000, 0x6000:
            if romSize == 0x4000 {
                /* The second half of 16k cartridges is mapped at 0xa000. */
                if vic20Signature.matches(rom: bytes, loadAddress: 0xa000, offset: offset + 0x2000) {
                    matches = true
                }
            }
            else {
                /* This might be a cartrigde that doesn't auto boot; we ignore this for now. */
            }
        case 0xa000:
            if romSize <= 0x2000 {
                if vic20Signature.matches(rom: bytes, loadAddress: 0xa000, offset: offset) {
                    matches = true
                }
            }
            else {
                /* There isn't room for 16k starting at 0xa000. */
            }
        default:
            break
        }
        
        if matches {
            return .vic20(address: loadAddress)
        }
        else {
            return nil
        }
    }

}

extension CartridgeImage: Cartridge {
    public var cartridgeType: CartridgeType {
        return .main
    }

    public var identifier: String {
        return url?.lastPathComponent ?? "<unknown>"
    }
    
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] {
        guard let fileName = url?.path else { return [:] }
        var resources = [MachineOld.ResourceName: MachineOld.ResourceValue]()

        switch format {
        case .c64, .crt(_):
            resources[.CartridgeFile] = .String(fileName)
            
        case .gmod2(let eepromUrl):
            resources[.CartridgeFile] = .String(fileName)
            if let eepromUrl = eepromUrl {
                resources[.CartridgeType] = .Int(Int32(CrtType.gmod2.rawValue))
                resources[.GMod2EEPROMImage] = .String(eepromUrl.path)
                resources[.GMod2EEPROMRW] = .Bool(true)
            }
            
        case .vic20(let address):
            if let resourceNane = MachineOld.ResourceName(vic20CartridgeAddress: address) {
                resources[.CartridgeType] = .Int(1)
                resources[resourceNane] = .String(fileName)
            }
        }
        return resources
    }
}

extension CartridgeImage: MachinePart {
    public var name: String {
        return title ?? "Cartridge"
    }
    
    public var icon: UIImage? {
        let name: String
        switch format {
        case .crt(let type):
            switch type {
            case .actionReplay2, .actionReplay3, .actionReplay4, .actionReplay5:
                name = "Action Replay"
            case .atomicPower:
                name = "Nordic Power"
            case .easyFlash, .easyFlashXbank:
                name = "Easy Flash Cartridge"
            case .finalCartridge1, .finalCartridge3, .finalCartridgePlus:
                name = "Final Cartridge III"
            case .gmod2:
                name = "GMod2 Cartridge"
            case .magicFormel:
                name = "Magic Formel"
            case .pagefox:
                name = "Scanntronics Pagefox"
            default:
                name = "Cartridge"
            }
            
        case .c64:
            name = "EPROM Cartridge"
            
        case .gmod2(_):
            name = "GMod2 Cartridge"
            
        case .vic20(_):
            name = "VIC-20 Cartridge"
        }
        return UIImage(named: name)
    }
    
    public var priority: Int {
        return MachinePartNormalPriority
    }
    
    public var connector: ConnectorType { format.connectorType }
}
