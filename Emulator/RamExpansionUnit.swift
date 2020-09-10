/*
 RamExpansionUnit.swift -- REU Cartridge
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

        32: RamExpansionUnit(identifier: "VIC-20 32K RAM",
                             name: "32K RAM",
                             fullName: "VIC-20 32K RAM",
                             iconName: "VIC-20 32K RAM",
                             size: 32,
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
                resources[.REUImageWrite] = .Bool(true)
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

            case 32:
                resources[.RAMBlock1] = .Bool(true)
                resources[.RAMBlock2] = .Bool(true)
                resources[.RAMBlock3] = .Bool(true)
                resources[.RAMBlock5] = .Bool(true)

            default:
                break
            }
            
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
