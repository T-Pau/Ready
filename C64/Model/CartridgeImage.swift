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

import Foundation

struct CartridgeImage {
    enum ViceType: Int {
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
        
        var hasFreeze: Bool {
            switch self {
            case .actionReplay5, .actionReplay2, .actionReplay3, .actionReplay4, .atomicPower, .capture, .diashowMaker, .expertCartridge, .finalCartridge1, .finalCartridge3, .finalCartridgePlus, .freezeFrame, .freezeMachine, .gameKiller, .kcsPower, .magicFormel, .mmcReplay, .retroReplay, .snapshot64, .superSnapshot4, .superSnapshot5:
                return true
                
            default:
                return false
            }
            
        }
    }
    var bytes: Data
    
    var isCrt: Bool
    var type: ViceType
    var title: String?
    var url: URL?
    var eepromUrl: URL? {
        didSet {
            if eepromUrl == nil {
                if isCrt {
                    type = ViceType(rawValue: Int(bytes[0x16]) << 8 | Int(bytes[0x17])) ?? .generic
                }
                else {
                    type = .generic
                }
            }
            else {
                // TODO: only allow if bin or already gmod2?
                type = .gmod2
            }
        }
    }
    
    init?(directory: URL, file: String?, eeprom: String? = nil) {
        guard let file = file else { return nil }
        let fileUrl = directory.appendingPathComponent(file)
        
        self.init(url: fileUrl)

        if let eeprom = eeprom {
            type = .gmod2
            eepromUrl = directory.appendingPathComponent(eeprom)
        }
    }

    init?(url: URL) {
        guard let bytes = FileManager.default.contents(atPath: url.path) else { return nil }
        self.init(bytes: bytes)
        self.url = url
    }
    
    init?(bytes: Data) {
        guard bytes.count >= 0x40 else { return nil } // too short for header
        
        if String(bytes: bytes[0...15], encoding: .ascii) == "C64 CARTRIDGE   " {
            isCrt = true
            title = String(bytes: bytes[0x20...0x3f], encoding: .ascii)
            if title?.isEmpty ?? false {
                title = nil
            }
            type = ViceType(rawValue: Int(bytes[0x16]) << 8 | Int(bytes[0x17])) ?? .generic
        }
        else {
            isCrt = false
            type = .generic
            title = nil
        }
        
        self.bytes = bytes
        url = nil
    }
}

extension CartridgeImage: Cartridge {
    var cartridgeType: CartridgeType {
        return .main
    }

    var identifier: String {
        return url?.lastPathComponent ?? "<unknown>"
    }
    
    var resources: [Machine.ResourceName: Machine.ResourceValue] {
        guard let fileName = url?.path else { return [:] }
        return [
            .CartridgeFile: .String(fileName)
        ]
    }

}

extension CartridgeImage: MachinePart {
    var name: String {
        return title ?? "Cartridge"
    }
    
    var icon: UIImage? {
        let name: String
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
            name = isCrt ? "Cartridge" : "EPROM Cartridge"
        }
        return UIImage(named: name)
    }
    
    var priority: Int {
        return MachinePartNormalPriority
    }
}
