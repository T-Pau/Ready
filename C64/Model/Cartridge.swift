/*
 Cartridge.swift -- Cartridge (without ROM Image) Harware Part
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

struct Cartridge: MachinePart {
    enum CartridgeType {
        case none
        case ramExpansionUnit(size: Int)
        
        var resources: [Machine.ResourceName: Machine.ResourceValue] {
            switch self {
            case .none:
                return [:]
                
            case .ramExpansionUnit(let size):
                return [
                    .REU: .Bool(true),
                    .REUsize: .Int(Int32(size))
                ]
            }
        }
    }

    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage?
    var priority: Int
    
    var type: CartridgeType
    
    var resources: [Machine.ResourceName: Machine.ResourceValue] {
        return type.resources
    }
    
    init(identifier: String, name: String, fullName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, type: CartridgeType) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.type = type
        switch type {
        case .ramExpansionUnit(let size):
            if size > 1024 && size % 1024 == 0 {
                variantName = "\(size / 1024) megabytes"
            }
            else {
                variantName = "\(size) kilobytes"
            }
        default:
            break
        }
    }
    
    static let none = Cartridge(identifier: "none", name: "None", iconName: nil, type: .none)
    
    static let cartridges: [Cartridge] = [
        none,
        
        Cartridge(identifier: "1700",
                  name: "1700",
                  fullName: "Commodore 1700 REU",
                  iconName: "Commodore RAM Expansion",
                  type: .ramExpansionUnit(size: 128)),
        
        Cartridge(identifier: "1764",
                  name: "1764",
                  fullName: "Commodore 1764 REU",
                  iconName: "Commodore RAM Expansion",
                  type: .ramExpansionUnit(size: 256)),
        
        Cartridge(identifier: "1750",
                  name: "1750",
                  fullName: "Commodore 1750 REU",
                  iconName: "Commodore RAM Expansion",
                  type: .ramExpansionUnit(size: 512)),
        
        Cartridge(identifier: "1750XL",
                  name: "1750XL",
                  fullName: "CMD 1750XL REU",
                  iconName: "CMD 1750XL REU",
                  type: .ramExpansionUnit(size: 2048)),
        
        Cartridge(identifier: "4MB REU",
                  name: "4MB REU",
                  fullName: "4MB REU (emulated by Ultimate II)",
                  iconName: "Ultimate II",
                  type: .ramExpansionUnit(size: 4096)),
        
        Cartridge(identifier: "8MB REU",
                  name: "8MB REU",
                  fullName: "8MB REU (emulated by Ultimate II)",
                  iconName: "Ultimate II",
                  type: .ramExpansionUnit(size: 8192)),
        
        Cartridge(identifier: "16MB REU",
                  name: "16MB REU",
                  fullName: "16MB REU (emulated by Ultimate II)",
                  iconName: "Ultimate II",
                  type: .ramExpansionUnit(size: 16384))
    ]
    
    static private var byIdentifier = [String: Cartridge]()
    
    static func cartridge(identifier: String) -> Cartridge? {
        if byIdentifier.isEmpty {
            for cartridge in cartridges {
                byIdentifier[cartridge.identifier] = cartridge
            }
        }
        
        return byIdentifier[identifier]
    }

}
