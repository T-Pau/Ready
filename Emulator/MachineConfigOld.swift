/*
 MachineConfig.swift -- Machine Configuration
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

import Foundation

public struct MachineConfigOld: ExpressibleByDictionaryLiteral {
    public enum BorderMode: String, CaseIterable, CustomStringConvertible {
        case auto
        case hide
        case show
        
        public var label: String {
            switch self {
            case .auto:
                return "Automatic"
            case .hide:
                return "Hide"
            case .show:
                return "Show"
            }
        }
        
        public var description: String {
            return label
        }

        public init?(label: String) {
            if let mode = BorderMode.allCases.first(where: { $0.label == label }) {
                self = mode
            }
            else {
                return nil
            }
        }

        public var cValue: Int32 {
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
        public init?(int: Int) {
            if let mode = BorderMode.allCases.first(where: { $0.cValue == int }) {
                self = mode
            }
            else {
                return nil
            }
        }
    }
    
    public enum DisplayedScreens: Equatable {
        case all
        case auto
        case screen(_ index: Int)
        
        public var cValue: Int32 {
            switch self {
            case .all:
                return -1
            case .auto:
                return -2
                
            case .screen(let index):
                return Int32(index)
            }
        }
        
        public var stringValue: String {
            switch self {
            case .all:
                return "all"
            case .auto:
                return "auto"
                
            case .screen(let index):
                return "\(index)"
            }
        }
        
        public init?(stringValue: String) {
            if stringValue == "all" {
                self = .all
            }
            else if stringValue == "auto" {
                self = .auto
            }
            else if let index = Int(stringValue) {
                self = .screen(index)
            }
            else {
                return nil
            }
        }
    }

    public enum SingularAdapterMode: String, CaseIterable, CustomStringConvertible {
        case cga
        case hs
        case bba
        case off
        
        public var description: String {
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

    
    public enum Value: Codable, Equatable {
        case string(_ value: String)
        case integer(_ value: Int)
        case number(_ value: Double)
        case boolean(_ value: Bool)
        
        public init(from decoder: Decoder) throws {
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
        
        public func encode(to encoder: Encoder) throws {
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
        
        public static func ==(lhs: Value, rhs: String) -> Bool {
            switch lhs {
            case .string(let value):
                return value == rhs
                
            default:
                return false
            }
        }
    }

//    typealias Value = String
    
    public var url: URL?
    
    private var values = [String: Value]()
    
    public static let cartridgeKeys: [MachineConfig.Key] = [ .expansionPort, .expansionPort1, .expansionPort2 ]
    public static let diskDriveKeys: [MachineConfig.Key] = [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11 ]
    
    public static let defaultConfig: MachineConfigOld = [
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
        .expansionPort1: .string("auto"),
        .expansionPort2: .string("auto"),
        .singularAdapterMode: .integer(Int(UserPortModule.ViceJoystickType.cga.rawValue)),
        .userPortJoystick1: .string("Competition Pro"),
        .userPortJoystick2: .string("Competition Pro")
    ]
    
    public init() {
    }
    
    public init(dictionaryLiteral elements: (MachineConfig.Key, Value)...) {
        for (key, value) in elements {
            set(value: value, for: key)
        }
    }
    
    public init(contentsOf url: URL, options: Data.ReadingOptions = []) {
        do {
            let data = try Data(contentsOf: url, options: options)
            let decoder = JSONDecoder()
            values = try decoder.decode([String:Value].self, from: data)
        }
        catch { }
        self.url = url
    }
    
    public func write(to url: URL, options: Data.WritingOptions = []) throws {
        let encoder = JSONEncoder()
        let data = try encoder.encode(values)
        try data.write(to: url, options: options)
    }
    
    public func save() throws {
        guard let url = url else { return /* TODO: throw error */ }
        
        try self.write(to: url, options: [.atomic] )
    }

    func hasValue(for key: MachineConfig.Key) -> Bool {
        return values[key.rawValue] != nil
    }

    func value(for key: MachineConfig.Key) -> Value? {
        return values[key.rawValue]
    }
    
    mutating func set(value: Value?, for key: MachineConfig.Key) {
        if let value = value {
            values[key.rawValue] = value
        }
        else {
            values.removeValue(forKey: key.rawValue)
        }
    }
    
    func string(for key: MachineConfig.Key) -> String? {
        guard let value = value(for: key) else { return nil }
        switch value {
        case .string(let string):
            return string
            
        default:
            return nil
        }
    }
    
    mutating func set(string: String?, for key: MachineConfig.Key) {
        if let string = string {
            set(value: .string(string), for: key)
        }
        else {
            set(value: nil, for: key)
        }
    }
    
    subscript(key: MachineConfig.Key) -> Value? {
        get {
            return value(for: key)
        }
        set {
            set(value: newValue, for: key)
        }
    }
    
    public func differences(to config: MachineConfigOld) -> Set<MachineConfig.Key> {
        var diffs = Set<MachineConfig.Key>()
        
        for key in Key.allCases {
            if value(for: key) != config.value(for: key) {
                diffs.insert(key)
            }
        }
        
        return diffs
    }
}
