/*
 MachineConfig.swift -- Machine Configuration
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

struct MachineConfig: ExpressibleByDictionaryLiteral {
    enum Key: String, CaseIterable {
        case borderMode
        case cassetteDrive
        case computer
        case controlPort1
        case controlPort2
        case diskDrive8
        case diskDrive9
        case diskDrive10
        case diskDrive11
        case expansionPort
        case singularAdapterMode
        case userPortJoystick1
        case userPortJoystick2
        case userPort
        
        var supportsAuto: Bool {
            switch self {
            case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .cassetteDrive, .expansionPort:
                return true
            default:
                return false
            }
        }
    }
    
    enum BorderMode: String, CaseIterable, CustomStringConvertible {
        case auto
        case hide
        case show
        
        var label: String {
            switch self {
            case .auto:
                return "Automatic"
            case .hide:
                return "Hide"
            case .show:
                return "Show"
            }
        }
        
        var description: String {
            return label
        }

        init?(label: String) {
            if let mode = BorderMode.allCases.first(where: { $0.label == label }) {
                self = mode
            }
            else {
                return nil
            }
        }

        var cValue: Int32 {
            switch self {
            case .auto:
                return 0
            case .hide:
                return 2
            case .show:
                return 1
            }
        }
        
        // for migration from Defaults
        init?(int: Int) {
            if let mode = BorderMode.allCases.first(where: { $0.cValue == int }) {
                self = mode
            }
            else {
                return nil
            }
        }
    }
    
    enum SingularAdapterMode: String, CaseIterable, CustomStringConvertible {
        case cga
        case hs
        case bba
        case off
        
        var description: String {
            switch self {
            case .cga:
                return "Classical Game Adapter / Protovision"
            case .hs:
                return "Digital Excess & Hitmen"
            case .bba:
                return "Kingsoft Bug Bomber Adapter"
            case .off:
                return "Disable"
            }
        }
    }

    
    enum Value: Codable, Equatable {
        case string(_ value: String)
        case integer(_ value: Int)
        case number(_ value: Double)
        case boolean(_ value: Bool)
        
        init(from decoder: Decoder) throws {
            let container = try decoder.singleValueContainer()
            do {
                let value = try container.decode(String.self)
                self = .string(value)
                return
            }
            catch { }
            do {
                let value = try container.decode(Bool.self)
                self = .boolean(value)
                return
            }
            catch { }
            do {
                let value = try container.decode(Int.self)
                self = .integer(value)
                return
            }
            catch { }
            do {
                let value = try container.decode(Double.self)
                self = .number(value)
                return
            }
            catch {
                throw error
            }
        }
        
        func encode(to encoder: Encoder) throws {
            var container = encoder.singleValueContainer()
            switch self {
            case .string(let value):
                try container.encode(value)
                
            case .integer(let value):
                try container.encode(value)

            case .number(let value):
                try container.encode(value)

            case .boolean(let value):
                try container.encode(value)

            }
        }
        
        static func ==(lhs: Value, rhs: String) -> Bool {
            switch lhs {
            case .string(let value):
                return value == rhs
                
            default:
                return false
            }
        }
    }

//    typealias Value = String
    
    var url: URL?
    
    private var values = [String: Value]()
    
    static let diskDriveKeys: [Key] = [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11 ]
    
    static let defaultConfig: MachineConfig = [
        .borderMode: .string("auto"),
        .cassetteDrive: .string("auto"),
        .computer: .string("C64 PAL"),
        .controlPort1: .string("Competition Pro"),
        .controlPort2: .string("Competition Pro"),
        .diskDrive8: .string("auto"),
        .diskDrive9: .string("auto"),
        .diskDrive10: .string("auto"),
        .diskDrive11: .string("auto"),
        .expansionPort: .string("auto"),
        .singularAdapterMode: .integer(Int(UserPortModule.ViceJoystickType.cga.rawValue)),
        .userPortJoystick1: .string("Competition Pro"),
        .userPortJoystick2: .string("Competition Pro")
    ]
    
    private struct ValueCodingKey: CodingKey {
        init?(stringValue: String) { self.stringValue = stringValue }
        let stringValue: String
        
        init?(intValue: Int) { return nil }
        var intValue: Int? { return nil }
    }
    
    init() {
    }
    
    init(dictionaryLiteral elements: (Key, Value)...) {
        for (key, value) in elements {
            set(value: value, for: key)
        }
    }
    
    init(contentsOf url: URL, options: Data.ReadingOptions = []) {
        do {
            let data = try Data(contentsOf: url, options: options)
            let decoder = JSONDecoder()
            values = try decoder.decode([String:Value].self, from: data)
        }
        catch { }
        self.url = url
    }
    
    func write(to url: URL, options: Data.WritingOptions = []) throws {
        let encoder = JSONEncoder()
        let data = try encoder.encode(values)
        try data.write(to: url, options: options)
    }
    
    func save() throws {
        guard let url = url else { return /* TODO: throw error */ }
        
        try self.write(to: url, options: [.atomic] )
    }

    func hasValue(for key: Key) -> Bool {
        return values[key.rawValue] != nil
    }

    func value(for key: Key) -> Value? {
        return values[key.rawValue]
    }
    
    mutating func set(value: Value?, for key: Key) {
        if let value = value {
            values[key.rawValue] = value
        }
        else {
            values.removeValue(forKey: key.rawValue)
        }
    }
    
    func string(for key: Key) -> String? {
        guard let value = value(for: key) else { return nil }
        switch value {
        case .string(let string):
            return string
            
        default:
            return nil
        }
    }
    
    mutating func set(string: String?, for key: Key) {
        if let string = string {
            set(value: .string(string), for: key)
        }
        else {
            set(value: nil, for: key)
        }
    }
    
    subscript(key: Key) -> Value? {
        get {
            return value(for: key)
        }
        set {
            set(value: newValue, for: key)
        }
    }
    
    func differences(to config: MachineConfig) -> Set<Key> {
        var diffs = Set<Key>()
        
        for key in Key.allCases {
            if value(for: key) != config.value(for: key) {
                diffs.insert(key)
            }
        }
        
        return diffs
    }
}
