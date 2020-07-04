/*
 Defaults.swift -- Structured Access to User Preferences
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

public class Defaults {
    public enum SortCriterium: Int {
        case name = 0
        case publisher = 1
        case year = 2
        case added = 3
        case lastOpened = 4
        case publisherYear = 5
    }

    private static var standardInstance: Defaults?
    
    public static var standard: Defaults {
        if let standard = standardInstance {
            return standard
        }
        else {
            let standard = Defaults()
            standardInstance = standard
            return standard
        }
    }
    
    public enum Key: String {
        case BiosFD2000
        case BiosFD4000
        case BiosJiffyDosC64
        case BiosJiffyDos1541
        case BiosJiffyDos1541II
        case BiosJiffyDos1571
        case BiosJiffyDos1581
        case BorderMode
        case CapsLockAsCommodore
        case CurrentTab
        case EmulateDriveSounds
        case EnableJiffyDos
        case GroupGames
        case GroupFavorites
        case ShowUncategorized
        case SortCriterium
        case SortCriteriumFavorites
        case ToolsCartridgeFile
        case VideoFilter
    }
    
    public var biosFD2000: String? {
        get { return stringValue(for: .BiosFD2000) }
        set { set(value: newValue, for: .BiosFD2000) }
    }

    public var biosFD4000: String? {
        get { return stringValue(for: .BiosFD4000) }
        set { set(value: newValue, for: .BiosFD4000) }
    }

    public var biosJiffyDosC64: String? {
        get { return stringValue(for: .BiosJiffyDosC64) }
        set { set(value: newValue, for: .BiosJiffyDosC64) }
    }

    public var biosJiffyDos1541: String? {
        get { return stringValue(for: .BiosJiffyDos1541) }
        set { set(value: newValue, for: .BiosJiffyDos1541) }
    }
    
    public var biosJiffyDos1541II: String? {
        get { return stringValue(for: .BiosJiffyDos1541II) }
        set { set(value: newValue, for: .BiosJiffyDos1541II) }
    }

    public var biosJiffyDos1571: String? {
        get { return stringValue(for: .BiosJiffyDos1571) }
        set { set(value: newValue, for: .BiosJiffyDos1571) }
    }

    public var biosJiffyDos1581: String? {
        get { return stringValue(for: .BiosJiffyDos1581) }
        set { set(value: newValue, for: .BiosJiffyDos1581) }
    }
    
    public var capsLockAsCommodore: Bool {
        get {
            return boolValue(for: .CapsLockAsCommodore)
        }
        set {
            set(value: newValue, for: .CapsLockAsCommodore)
        }
    }
    
    public var emulateDriveSounds: Bool {
        get {
            return boolValue(for: .EmulateDriveSounds)
        }
        set {
            set(value: newValue, for: .EmulateDriveSounds)
        }
    }
    
    public var enableJiffyDOS: Bool {
        get {
            return boolValue(for: .EnableJiffyDos)
        }
        set {
            set(value: newValue, for: .EnableJiffyDos)
        }
    }
    
    public var groupGames: Bool {
        get {
            return boolValue(for: .GroupGames)
        }
        set {
            set(value: newValue, for: .GroupGames)
        }
    }

    public var groupFavorites: Bool {
        get {
            return boolValue(for: .GroupFavorites)
        }
        set {
            set(value: newValue, for: .GroupFavorites)
        }
    }
    
    public var machineConfig: MachineConfigOld {
        var config = MachineConfigOld(contentsOf: Defaults.applicationSupportURL.appendingPathComponent("Default Machine Configuration.json"))
        
        // migrate Border Mode from Defaults to default MachineConfig
        if !config.hasValue(for: .borderMode) {
            if let value = intValue(for: .BorderMode),
                let borderMode = MachineConfigOld.BorderMode(int: value)
            {
                config[.borderMode] = .string(borderMode.rawValue)
                do {
                    try config.save()
                    set(value: nil, for: .BorderMode)
                }
                catch { }
            }
        }
        
        return config
    }
    
    public var machineSpecification: MachineSpecification {
        get {
            return MachineSpecification(layers: [ machineConfig ])
        }
    }
    
    public var showUncategorized: Bool {
        get {
            return boolValue(for: .ShowUncategorized)
        }
        set {
            set(value: newValue, for: .ShowUncategorized)
        }
    }
    
    public var sortCriterium: SortCriterium {
        get {
            if let value = intValue(for: .SortCriterium) {
                return SortCriterium(rawValue: value) ?? .name
            }
            else {
                return .name
            }
        }
        set {
            set(value: newValue.rawValue, for: .SortCriterium)
        }
    }

    public var sortCriteriumFavorites: SortCriterium {
        get {
            if let value = intValue(for: .SortCriteriumFavorites) {
                return SortCriterium(rawValue: value) ?? .name
            }
            else {
                return .name
            }
        }
        set {
            set(value: newValue.rawValue, for: .SortCriteriumFavorites)
        }
    }

    public var toolsCartridgeFile: String? {
        get {
            return stringValue(for: .ToolsCartridgeFile)
        }
        set {
            set(value: newValue, for: .ToolsCartridgeFile)
        }
    }
    
    public var videoFilter: String {
        get {
            return stringValue(for: .VideoFilter) ?? "None"
        }
        set {
            set(value: newValue, for: .VideoFilter)
        }
    }
    
    public func boolValue(for key: Key) -> Bool {
        if let value =  UserDefaults.standard.value(forKey: key.rawValue) as? Bool {
            return value
        }
        switch key {
        case .CapsLockAsCommodore:
            return true
        case .EmulateDriveSounds:
            return true
        case .EnableJiffyDos:
            return true
        case .GroupGames:
            return false
        case .ShowUncategorized:
            return true
            
        default:
            return false
        }
    }

    public func intValue(for key: Key) -> Int? {
        return UserDefaults.standard.value(forKey: key.rawValue) as? Int
    }

    public func stringValue(for key: Key) -> String? {
        return UserDefaults.standard.value(forKey: key.rawValue) as? String
    }
    
    public func set(value: Bool, for key: Key) {
        UserDefaults.standard.set(value, forKey: key.rawValue)
    }

    public func set(value: Int, for key: Key) {
        UserDefaults.standard.set(value, forKey: key.rawValue)
    }

    public func set(value: String?, for key: Key) {
        UserDefaults.standard.set(value, forKey: key.rawValue)
    }
    
    public static var applicationSupportURL: URL {
        guard let path = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first else { fatalError("no ApplicationSupport directory found") }
        
        return URL(fileURLWithPath: path)
    }
    
    public static var viceDataURL: URL {
        return Bundle.main.resourceURL!.appendingPathComponent("vice")
    }
    
    public static var documentURL: URL {
        guard let documentDirectoryPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true).first else { fatalError("no document directory found") }
        
        return URL(fileURLWithPath: documentDirectoryPath)
    }
    
    
    public static var inboxName: String {
        return "Unsorted"
    }
    
    public static var exporedName: String {
        return "Exported"
    }
    
    public static var inboxURL: URL {
        return documentURL.appendingPathComponent(inboxName)
    }
    
    public static var exportedURL: URL {
        return documentURL.appendingPathComponent(exporedName)
    }
    
    public static var libraryURL: URL {
        return applicationSupportURL.appendingPathComponent("Library")
    }
    
    public static var biosURL: URL {
        return applicationSupportURL.appendingPathComponent("BIOS")
    }
    
    public static var toolsURL: URL {
        return applicationSupportURL.appendingPathComponent("Tools")
    }
}
