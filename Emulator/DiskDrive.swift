/*
 DiskDrive.swift -- Information about Disk Drives
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
    
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var priority: Int
    public var connector: ConnectorType { .commodoreIEC }
    
    public var viceType: ViceType
    public var supportedMediaTypes: Set<ConnectorType>
    public var leds: [Led]
    public var caseColor: UIColor?
    public var textColor: UIColor?
    public var biosKey: Defaults.Key?
    public var jiffyDosKey: Defaults.Key?
    public var isDoubleSided: Bool

    public var image: DiskImage?
    
    public static func isDoubleSided(connector: ConnectorType) -> Bool {
        switch connector {
        case .floppy3_5DoubleDensityDoubleSidedCmd,
                .floppy3_5DoubleDensityDoubleSidedCommodore,
                .floppy3_5ExtendedDensityDoubleSidedCmd,
                .floppy3_5HighDensityDoubleSidedCmd,
                .floppy5_25SingleDensityDoubleSidedCommodore:
            return true

        case .floppy5_25SingleDensitySingleSidedCommodore:
            return false

        default:
            return false
        }
    }
    
    public static func is5_25(connector: ConnectorType) -> Bool {
        switch connector {
        case .floppy3_5DoubleDensityDoubleSidedCmd,
                .floppy3_5DoubleDensityDoubleSidedCommodore,
                .floppy3_5ExtendedDensityDoubleSidedCmd,
                .floppy3_5HighDensityDoubleSidedCmd:
            return false
            
        case .floppy5_25SingleDensityDoubleSidedCommodore,
                .floppy5_25SingleDensitySingleSidedCommodore:
            return true
            
        default:
            return false
        }

    }

    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, viceType: ViceType, supportedMediaTypes: Set<ConnectorType>, leds: [Led], caseColorName: String?, textColorName: String? = nil, biosKey: Defaults.Key? = nil, jiffyDosKey: Defaults.Key? = nil, image: DiskImage? = nil) {
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
        self.isDoubleSided = supportedMediaTypes.contains(where: { DiskDrive.isDoubleSided(connector: $0) })
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
                                supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore ],
                      leds: [Led(isRound: false, colorName: "1541-II Chinon")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1541II),
            DiskDrive(identifier: "1571",
                      name: "1571",
                      fullName: "Commodore 1571",
                      iconName: "Commodore 1571",
                      priority: MachinePartLowPriority,
                      viceType: .cbm1571,
                      supportedMediaTypes: [ .floppy5_25SingleDensitySingleSidedCommodore, .floppy5_25SingleDensityDoubleSidedCommodore ],
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
                      supportedMediaTypes: [ .floppy3_5DoubleDensityDoubleSidedCommodore ],
                      leds: [Led(isRound: false, colorName: "1581")],
                      caseColorName: "C64C Case",
                      jiffyDosKey: .BiosJiffyDos1581),
            
            DiskDrive(identifier: "FD-2000",
                      name: "FD-2000",
                      fullName: "CMD FD-2000",
                      iconName: "CMD FD",
                      viceType: .cmd_fd2000,
                      supportedMediaTypes: [ .floppy3_5DoubleDensityDoubleSidedCommodore, .floppy3_5DoubleDensityDoubleSidedCmd, .floppy3_5HighDensityDoubleSidedCmd ],
                      leds: [Led(isRound: false, colorName: "FD Green"), Led(isRound: false, colorName: "FD Red")],
                      caseColorName: "CMD FD Case",
                      textColorName: "CMD FD Text",
                      biosKey: .BiosCmdFd2000),
            
            DiskDrive(identifier: "FD-4000",
                      name: "FD-4000",
                      fullName: "CMD FD-4000",
                      iconName: "CMD FD",
                      viceType: .cmd_fd4000,
                      supportedMediaTypes: [ .floppy3_5DoubleDensityDoubleSidedCommodore, .floppy3_5DoubleDensityDoubleSidedCmd, .floppy3_5HighDensityDoubleSidedCmd, .floppy3_5ExtendedDensityDoubleSidedCmd ],
                      leds: [Led(isRound: false, colorName: "FD Green"), Led(isRound: false, colorName: "FD Red")],
                      caseColorName: "CMD FD Case",
                      textColorName: "CMD FD Text",
                      biosKey: .BiosCmdFd4000)
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
        return getDriveSupporting(connector: image.connector)
    }

    public static func getDriveSupporting(connector: ConnectorType) -> DiskDrive? {
            for drive in drives.parts.sorted(by: { $0.priority > $1.priority }) as? [DiskDrive] ?? [] {
                if drive.supportedMediaTypes.contains(connector) {
                    return drive
                }
        }
        return nil
    }
    
    public func supports(image: MediaItem) -> Bool {
        return supportedMediaTypes.contains(image.connector)
    }

    public func supports(image: DiskImage) -> Bool {
        return supportedMediaTypes.contains(image.connector)
    }
}
