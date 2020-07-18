/*
 Ide64Cartridge -- IDE64 Cartridge
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

    /*
    public var ports = [
        Port(name: "CF Card", key: .driveDrive, connectorTypes: [.media(.compactFlashIde64)]),
        Port(name: "Master", key: .driveDrive2, connectorTypes: [.media(.harddiskIdeIde64), .ide])
        Port(name: "Slave", key: .driveDrive3, connectorTypes: [.media(.harddiskIdeIde64)], .ide])
    ]
     */
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
    
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] {
        return [
            .IDE64version: .Int(version.rawValue),
            .IDE64RTCSave: .Bool(true),
            .CartridgeType: .Int(Int32(CartridgeImage.CrtType.ide64.rawValue)),
            .CartridgeFile: .String(Defaults.viceDataURL.appendingPathComponent("C64/" + version.romName).path)
        ]
    }
}
