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
    
    init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, resources: [Machine.ResourceName: Machine.ResourceValue]) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.variantName = variantName
        self.priority = priority
        self.resources = resources
    }

    
    static let none = OtherCartridge(identifier: "none", name: "None", iconName: nil, resources: [:])


    static var _cartridges = MachinePartList(sections: [])
    static var cartridges: MachinePartList {
        if _cartridges.isEmpty {
            _cartridges = MachinePartList(sections: [
                MachinePartSection(title: nil, parts: [
                    none
                ]),
        
                MachinePartSection(title: "RAM Expansion Units", parts: RamExpansionUnit.ramExpansionUnits.sorted(by: { $0.key < $1.key }).map({ $0.value })),
                
                MachinePartSection(title: "Other Cartridges", parts: [
                    Ide64Cartridge(version: .version4_1),
                    
/*                    OtherCartridge(identifier: "MIDI Print Technik",
                                   name: "MIDI",
                                   fullName: "MIDI",
                                   variantName: "Sequential",
                                   iconName: "MIDI",
                                   resources: [
                                    .MIDIEnable: .Bool(true),
                                    .MIDIMode: .Int(0) // TODO: sybmolic name
                        ]), */
                    
                    OtherCartridge(identifier: "CPM",
                                   name: "CP/M",
                                   fullName: "CP/M Cartridge",
                                   iconName: "CPM Cartridge",
                                   resources: [
                                    .CPMCart: .Bool(true)
                        ])
                    ])
                ])
        }
        return _cartridges
    }
    
    static private var byIdentifier = [String: Cartridge]()
    
    static func cartridge(identifier: String) -> Cartridge? {
        if byIdentifier.isEmpty {
            for cartridge in cartridges.parts {
                byIdentifier[cartridge.identifier] = cartridge as? Cartridge
            }
        }
        
        return byIdentifier[identifier]
    }

}

extension OtherCartridge: MachinePart {
    
}

