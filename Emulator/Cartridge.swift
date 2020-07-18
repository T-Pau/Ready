/*
 Cartridge.swift -- Cartridge (without ROM Image) Harware Part
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

public enum CartridgeType {
    case none
    case expander
    case io
    case main
}

public protocol Cartridge {
    var identifier: String { get }
    var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] { get }
    var numberOfSlots: Int { get }
    var cartridgeType: CartridgeType { get }
}

extension Cartridge {
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] {
        return [:]
    }
    public var numberOfSlots: Int {
        return 0
    }
}

public struct OtherCartridge: Cartridge {
    public enum MidiMode: Int32 {
        case sequential = 0
        case passport = 1
        case datel = 2
        case namesoft = 3
        case maplin = 4
    }
    
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var priority: Int
    public var connector: ConnectorType


    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue]
    public var numberOfSlots: Int
    public var cartridgeType: CartridgeType
    
    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, connector: ConnectorType = .c64ExpansionPort, cartridgeType: CartridgeType, numberOfSlots: Int = 0, resources: [MachineOld.ResourceName: MachineOld.ResourceValue]) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.variantName = variantName
        self.priority = priority
        self.connector = connector
        self.resources = resources
        self.cartridgeType = cartridgeType
        self.numberOfSlots = numberOfSlots
    }

    
    public static let none = OtherCartridge(identifier: "none", name: "None", iconName: nil, connector: .none, cartridgeType: .none, resources: [:])

    public static let expander = OtherCartridge(identifier: "Mini X-Pander", name: "X-Pander", fullName: "MIni X-Pander", iconName: "Mini X-Pander", cartridgeType: .expander, numberOfSlots: 2, resources: [:])

    private static var _cartridges = MachinePartList(sections: [])
    public static var cartridges: MachinePartList {
        if _cartridges.isEmpty {
            _cartridges = MachinePartList(sections: [
                MachinePartSection(title: nil, parts: [
                    none
                ]),
        
                MachinePartSection(title: "RAM Expansion Units", parts: RamExpansionUnit.ramExpansionUnits.sorted(by: { $0.key < $1.key }).map({ $0.value })),
                
                MachinePartSection(title: "Other Cartridges", parts: [
                    expander,
                    Ide64Cartridge(version: .version4_1),

/*                    OtherCartridge(identifier: "MIDI Print Technik",
                                   name: "MIDI",
                                   fullName: "MIDI",
                                   variantName: "Sequential",
                                   iconName: "MIDI",
                                   cartridgeType: .io,
                                   resources: [
                                    .MIDIEnable: .Bool(true),
                                    .MIDIMode: .Int(MidiMode.sequential.rawValue)
                        ]), */
                    
                    OtherCartridge(identifier: "CPM",
                                   name: "CP/M",
                                   fullName: "CP/M Cartridge",
                                   iconName: "CPM Cartridge",
                                   cartridgeType: .main,
                                   resources: [
                                    .CPMCart: .Bool(true)
                        ])
                    ])
                ])
        }
        return _cartridges
    }
    
    static private var byIdentifier = [String: Cartridge]()
    
    public static func cartridge(identifier: String) -> Cartridge? {
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

