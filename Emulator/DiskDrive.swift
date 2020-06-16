/*
 DiskDrive.swift -- Information about Disk Drives
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

public struct DiskDrive: MachinePart {
    public enum ViceType: Int32 {
        case none = 0
        case cbm1541 = 1541
        case cbm1541II = 1542
        case cbm1571 = 1571
        case cbm1581 = 1581
        case cmd_fd2000 = 2000
        case cmd_fd4000 = 4000
    }
    
    public struct Led {
        public var isRound: Bool
        public var darkColor: UIColor
        public var lightColor: UIColor
        
        public init(isRound: Bool, colorName: String) {
            self.isRound = isRound
            darkColor = UIColor(named: "LED \(colorName) Off") ?? UIColor.darkGray
            lightColor = UIColor(named: "LED \(colorName) On") ?? UIColor.lightGray
        }
    }
    
    public enum MediaType {
        case singleDensitySingleSided5_25
        case singleDensityDoubleSided5_25
        case doubleDensitySingleSided5_25
        case doubleDensityDoubleSided5_25
        case doubleDensity3_5
        case cmdDoubleDensity3_5
        case cmdHighDensity3_5
        case cmdExtendedDensity3_5
        
        public var is5_25Inch: Bool {
            switch self {
            case .singleDensitySingleSided5_25, .singleDensityDoubleSided5_25, .doubleDensitySingleSided5_25, .doubleDensityDoubleSided5_25:
                return true
            case .doubleDensity3_5, .cmdDoubleDensity3_5, .cmdHighDensity3_5, .cmdExtendedDensity3_5:
                return false
            }
        }
        
        public var isDoubleSided: Bool {
            switch self {
            case .singleDensitySingleSided5_25, .doubleDensitySingleSided5_25:
                return false
            case .singleDensityDoubleSided5_25, .doubleDensityDoubleSided5_25, .doubleDensity3_5, .cmdDoubleDensity3_5, .cmdHighDensity3_5, .cmdExtendedDensity3_5:
                return true
            }
        }
    }
    
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var priority: Int
    
    public var viceType: ViceType
    public var supportedMediaTypes: Set<MediaType>
    public var leds: [Led]
    public var caseColor: UIColor?
    public var textColor: UIColor?
    public var biosKey: Defaults.Key?
    public var jiffyDosKey: Defaults.Key?
    public var isDoubleSided: Bool

    public var image: DiskImage?

    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, viceType: ViceType, supportedMediaTypes: Set<MediaType>, leds: [Led], caseColorName: String?, textColorName: String? = nil, biosKey: Defaults.Key? = nil, jiffyDosKey: Defaults.Key? = nil, image: DiskImage? = nil) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.viceType = viceType
        self.supportedMediaTypes = supportedMediaTypes
        self.leds = leds
        if let caseColorName = caseColorName {
            self.caseColor = UIColor(named: caseColorName)
        }
        if let textColorName = textColorName {
            self.textColor = UIColor(named: textColorName)
        }
        else {
            self.textColor = UIColor.darkText
        }
        self.biosKey = biosKey
        self.jiffyDosKey = jiffyDosKey
        self.image = image
        self.isDoubleSided = supportedMediaTypes.contains(where: { $0.isDoubleSided })
    }
    
    public var hasStatus: Bool { return viceType != .none }
    
    public static let none = DiskDrive(identifier: "none",
                                name: "None",
                                iconName: nil,
                                priority: 0,
                                viceType: .none,
                                supportedMediaTypes: [],
                                leds: [],
                                caseColorName: nil)

    public static let sx64 = DiskDrive(identifier: "SX64",
                                name: "SX 64",
                                iconName: "Commodore SX64",
                                viceType: .cbm1541,
                                supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                                leds: [Led(isRound: true, colorName: "1541")],
                                caseColorName: "SX64 Case",
                                textColorName: "SX64 Frame",
                                jiffyDosKey: .BiosJiffyDos1541)

    public static var drives = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: [
            none
        ]),
        
        MachinePartSection(title: "5.25\"", parts: [
            DiskDrive(identifier: "1541 Alps",
                      name: "1541",
                      fullName: "Commodore 1541",
                      variantName: "Alps Drive",
                      iconName: "Commodore 1541 Alps",
                      viceType: .cbm1541,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: true, colorName: "1541")],
                      caseColorName: "C64 Case",
                      jiffyDosKey: .BiosJiffyDos1541),
            
            DiskDrive(identifier: "1541 Newtroncs",
                      name: "1541",
                      fullName: "Commodore 1541",
                      variantName: "Newtronics Drive",
                      iconName: "Commodore 1541 Newtronics",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1541,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541")],
                      caseColorName: "C64 Case",
                      jiffyDosKey: .BiosJiffyDos1541),
            
            DiskDrive(identifier: "1541C Alps",
                      name: "1541C",
                      fullName: "Commodore 1541C",
                      variantName: "Alps Drive",
                      iconName: "Commodore 1541C Alps",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1541,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1541),
            
            DiskDrive(identifier: "1541C Newtroncs",
                      name: "1541C",
                      fullName: "Commodore 1541C",
                      variantName: "Newtronics Drive",
                      iconName: "Commodore 1541C Newtronics",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1541,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1541),
            
            DiskDrive(identifier: "1541-II JPN",
                      name: "1541-II",
                      fullName: "Commodore 1541-II",
                      variantName: "JPN Drive",
                      iconName: "Commodore 1541-II JPN",
                      priority: MachinePartHighPriority,
                      viceType: .cbm1541II,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541-II JPN")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1541II),
            
            DiskDrive(identifier: "1541-II Chinon",
                      name: "1541-II",
                      fullName: "Commodore 1541-II",
                      variantName: "Chinon Drive",
                      iconName: "Commodore 1541-II Chinon",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1541II,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541-II Chinon")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1541II),
            DiskDrive(identifier: "1571",
                      name: "1571",
                      fullName: "Commodore 1571",
                      iconName: "Commodore 1571",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1571,
                      supportedMediaTypes: [ .singleDensitySingleSided5_25, .singleDensityDoubleSided5_25 ],
                      leds: [Led(isRound: false, colorName: "1541-II Chinon")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1571)
        ]),
        
        MachinePartSection(title: "3.5\"", parts: [
            DiskDrive(identifier: "1581",
                      name: "1581",
                      fullName: "Commodore 1581",
                      iconName: "Commodore 1581",
                      viceType: .cbm1581,
                      supportedMediaTypes: [ .doubleDensity3_5 ],
                      leds: [Led(isRound: false, colorName: "1581")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1581),
            
            DiskDrive(identifier: "FD-2000",
                      name: "FD-2000",
                      fullName: "CMD FD-2000",
                      iconName: "CMD FD",
                      viceType: .cmd_fd2000,
                      supportedMediaTypes: [ .doubleDensity3_5, .cmdDoubleDensity3_5, .cmdHighDensity3_5 ],
                      leds: [Led(isRound: false, colorName: "FD Green"), Led(isRound: false, colorName: "FD Red")],
                      caseColorName: "CMD FD Case",
                      textColorName: "CMD FD Text",
                      biosKey: .BiosFD2000),
            
            DiskDrive(identifier: "FD-4000",
                      name: "FD-4000",
                      fullName: "CMD FD-4000",
                      iconName: "CMD FD",
                      viceType: .cmd_fd4000,
                      supportedMediaTypes: [ .doubleDensity3_5, .cmdDoubleDensity3_5, .cmdHighDensity3_5, .cmdExtendedDensity3_5 ],
                      leds: [Led(isRound: false, colorName: "FD Green"), Led(isRound: false, colorName: "FD Red")],
                      caseColorName: "CMD FD Case",
                      textColorName: "CMD FD Text",
                      biosKey: .BiosFD4000)
        ])
    ])
    
    static private var byIdentifier = [String: DiskDrive]()
    
    public static func drive(identifier: String) -> DiskDrive? {
        if byIdentifier.isEmpty {
            for drive in drives.parts {
                byIdentifier[drive.identifier] = drive as? DiskDrive
            }
        }
        
        return byIdentifier[identifier]
    }
    
    public static func getDriveSupporting(image: DiskImage) -> DiskDrive? {
        return getDriveSupporting(mediaType: image.mediaType)
    }

    public static func getDriveSupporting(mediaType: MediaType) -> DiskDrive? {
            for drive in drives.parts.sorted(by: { $0.priority > $1.priority }) as? [DiskDrive] ?? [] {
                if drive.supportedMediaTypes.contains(mediaType) {
                    return drive
                }
        }
        return nil
    }
    
    public func supports(image: DiskImage) -> Bool {
        return supportedMediaTypes.contains(image.mediaType)
    }
}
