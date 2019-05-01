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

protocol Cartridge {
    var identifier: String { get }
    var resources: [Machine.ResourceName: Machine.ResourceValue] { get }
}

extension Cartridge {
    var resources: [Machine.ResourceName: Machine.ResourceValue] {
        return [:]
    }
}

struct OtherCartridge: Cartridge {
    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage?
    var priority: Int
    
    var resources: [Machine.ResourceName: Machine.ResourceValue]
    
    init(identifier: String, name: String, fullName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, resources: [Machine.ResourceName: Machine.ResourceValue]) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.resources = resources
    }

    
    static let none = OtherCartridge(identifier: "none", name: "None", iconName: nil, resources: [:])


    static var _cartridges = [Cartridge]()
    static var cartridges: [Cartridge] {
        if _cartridges.isEmpty {
            _cartridges = [none]
            _cartridges.append(contentsOf: RamExpansionUnit.ramExpansionUnits.sorted(by: { $0.key < $1.key }).map({ $0.value }))
        }
        
        return _cartridges
    }
    
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

extension OtherCartridge: MachinePart {
    
}
