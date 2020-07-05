/*
 RamExpansionUnit.swift -- REU Cartridge
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

public struct RamExpansionUnit {
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var connector: ConnectorType
    public var url: URL?
    
    public var size: Int
    
    public static let ramExpansionUnits = [
        3: RamExpansionUnit(identifier: "VIC-1211A",
                            name: "VIC-1211A",
                            fullName: "Commodore VIC-1211A",
                            iconName: "Commodore VIC-111x",
                            size: 3,
                            connector: .vic20ExpansionPort),

        8: RamExpansionUnit(identifier: "VIC-1110",
                            name: "VIC-1110",
                            fullName: "Commodore VIC-1110",
                            iconName: "Commodore VIC-111x",
                            size: 8,
                            connector: .vic20ExpansionPort),

        16: RamExpansionUnit(identifier: "VIC-1111",
                             name: "VIC-1111",
                             fullName: "Commodore VIC-1111",
                             iconName: "Commodore VIC-111x",
                             size: 16,
                             connector: .vic20ExpansionPort),

        128: RamExpansionUnit(identifier: "1700",
                              name: "1700",
                              fullName: "Commodore 1700 REU",
                              iconName: "Commodore RAM Expansion",
                              size: 128),
        
        256: RamExpansionUnit(identifier: "1764",
                              name: "1764",
                              fullName: "Commodore 1764 REU",
                              iconName: "Commodore RAM Expansion",
                              size: 256),
        
        512: RamExpansionUnit(identifier: "1750",
                              name: "1750",
                              fullName: "Commodore 1750 REU",
                              iconName: "Commodore RAM Expansion",
                              size: 512),
        
        2048: RamExpansionUnit(identifier: "1750XL",
                               name: "1750XL",
                               fullName: "CMD 1750XL REU",
                               iconName: "CMD 1750XL REU",
                               size: 2048),
        
        4096: RamExpansionUnit(identifier: "4MB REU",
                               name: "4MB REU",
                               fullName: "4MB REU (emulated by Ultimate II)",
                               iconName: "Ultimate II",
                               size: 4096),
        
        8192: RamExpansionUnit(identifier: "8MB REU",
                               name: "8MB REU",
                               fullName: "8MB REU (emulated by Ultimate II)",
                               iconName: "Ultimate II",
                               size: 8192),
        
        16384: RamExpansionUnit(identifier: "16MB REU",
                                name: "16MB REU",
                                fullName: "16MB REU (emulated by Ultimate II)",
                                iconName: "Ultimate II",
                                size: 16384)
    ]

    public init(identifier: String, name: String, fullName: String? = nil, iconName: String?, size: Int, connector: ConnectorType = .c64ExpansionPort) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.size = size

        if size > 1024 && size % 1024 == 0 {
            variantName = "\(size / 1024) megabytes"
        }
        else {
            variantName = "\(size) kilobytes"
        }
        self.connector = connector
    }
    
    public init?(size: Int) {
        guard size % 1024 == 0 else { return nil }
        guard let reu = RamExpansionUnit.ramExpansionUnits[size / 1024] else { return nil }
        self = reu
    }
    
    public init?(url: URL) {
        do {
            let data = try Data(contentsOf: url)
            guard let reu = RamExpansionUnit(size: data.count) else { return nil }
            self = reu
            
            identifier = url.path
            self.url = url
            if let variant = variantName {
                variantName = "\(identifier) (\(variant))"
            }
            else {
                variantName = identifier
            }
        }
        catch {
            return nil
        }
    }
}

extension RamExpansionUnit: Cartridge {
    public var cartridgeType: CartridgeType {
        return .io
    }
    
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] {
        var resources = [MachineOld.ResourceName: MachineOld.ResourceValue]()
            
        switch connector {
        case .c64ExpansionPort:
            resources[.REU] = .Bool(true)
            resources[.REUsize] = .Int(Int32(size))
            if let fileName = url?.path {
                resources[.REUfilename] = .String(fileName)
            }

        case .vic20ExpansionPort:
            switch size {
            case 3:
                resources[.RAMBlock0] = .Bool(true)
                
            case 8:
                resources[.RAMBlock1] = .Bool(true)

            case 16:
                resources[.RAMBlock1] = .Bool(true)
                resources[.RAMBlock2] = .Bool(true)

            default:
                break
            }
            // TODO
            
        default:
            break
        }
        
        return resources
    }
}

extension RamExpansionUnit: MachinePart {
    public var priority: Int {
        return MachinePartNormalPriority
    }
}
