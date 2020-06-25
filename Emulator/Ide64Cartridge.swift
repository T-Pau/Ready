/*
 Ide64Cartridge -- IDE64 Cartridge
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

public struct Ide64Cartridge {
    public enum Version: Int32 {
        case version3 = 0
        case version4_1 = 1
        case version4_2 = 2
        
        public var name: String {
            switch self {
            case .version3:
                return "3.4"
            case .version4_1:
                return "4.1"
            case .version4_2:
                return "4.2"
            }
        }
        
        public var romName: String {
            switch self {
            case .version3:
                return "idedos20190819-v3-c64.rom"
            case .version4_1, .version4_2:
                return "idedos20190819-v4-c64.rom"
            }
            
        }
    }
    
    public var version: Version
    
    public static var standard = Ide64Cartridge(version: .version4_1)
}

extension Ide64Cartridge: MachinePart {
    public var identifier: String { "IDE64 \(version.name)" }
    public var name: String { "IDE64" }
    public var variantName: String? { version.name }
    public var icon: UIImage? { UIImage(named: identifier) }
    public var priority: Int { MachinePartNormalPriority }
    public var connector: ConnectorType { .c64ExpansionPort }
}

extension Ide64Cartridge: Cartridge {
    public var cartridgeType: CartridgeType {
        return .main
    }
    
    public var resources: [Machine.ResourceName: Machine.ResourceValue] {
        return [
            .IDE64version: .Int(version.rawValue),
            .IDE64RTCSave: .Bool(true),
            .CartridgeType: .Int(Int32(CartridgeImage.CrtType.ide64.rawValue)),
            .CartridgeFile: .String(Defaults.viceDataURL.appendingPathComponent("C64/" + version.romName).path)
        ]
    }
}
