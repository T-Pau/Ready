//
//  RamExpansionUnit.swift
//  C64
//
//  Created by Dieter Baron on 2019/05/01.
//  Copyright © 2019 Spiderlab. All rights reserved.
//

import Foundation

struct RamExpansionUnit {
    var identifier: String
    var name: String
    var fullName: String
    var variantName: String?
    var icon: UIImage?
    var url: URL?
    
    var size: Int
    
    static let ramExpansionUnits = [
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

    init(identifier: String, name: String, fullName: String? = nil, iconName: String?, size: Int) {
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
    }
    
    init?(size: Int) {
        guard size % 1024 == 0 else { return nil }
        guard let reu = RamExpansionUnit.ramExpansionUnits[size / 1024] else { return nil }
        self = reu
    }
    
    init?(url: URL) {
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
    var resources: [Machine.ResourceName: Machine.ResourceValue] {
        var resources: [Machine.ResourceName: Machine.ResourceValue] = [
            .REU: .Bool(true),
            .REUsize: .Int(Int32(size))
        ]
        if let fileName = url?.path {
            resources[.REUfilename] = .String(fileName)
        }
        
        return resources
    }
}

extension RamExpansionUnit: MachinePart {
    var priority: Int {
        return MachinePartNormalPriority
    }
}
