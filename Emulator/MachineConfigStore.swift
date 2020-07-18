/*
 MachinConfigStore.swift -- Store One Layer of Machine Configuration
 Copyright (C) 2019-2020 Dieter Baron
 
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

struct MyError: Error {
    
}

public class MachineConfigStore {
     public enum Value: Codable, Equatable {
        case boolean(_ value: Bool)
        case dictionary(_ value: Node)
        case integer(_ value: Int)
        case number(_ value: Double)
        case string(_ value: String)
        
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
            catch { }
            do {
                let value = try container.decode(Node.self)
                self = .dictionary(value)
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
                
            case .dictionary(let value):
                try container.encode(value)
            }
        }
    }

    public class Node: Codable, Equatable {
        public var values = [MachineConfig.Key: Value]()
        
        public init() { }

        required public init(from decoder: Decoder) throws {
            let container = try decoder.container(keyedBy: MachineConfig.Key.self)
            for key in container.allKeys {
                values[key] = try container.decode(Value.self, forKey: key)
            }
        }
        
        private var format1Migration: [MachineConfig.Key: [MachineConfig.Key]] = [
            .borderMode: [.screen, .borderMode],
            .cassetteDrive: [.cassetteDrive, .identifier],
            .computer: [.computer, .identifier],
            .controlPort1: [.joystickPort, .identifier],
            .controlPort2: [.joystickPort2, .identifier],
            .diskDrive8: [.diskDrive, .identifier],
            .diskDrive9: [.diskDrive2, .identifier],
            .diskDrive10: [.diskDrive3, .identifier],
            .diskDrive11: [.diskDrive4, .identifier],
            .expansionPort: [.expansionPort, .identifier],
            .expansionPort1: [.expansionPort, .expansionPort, .identifier],
            .expansionPort2: [.expansionPort, .expansionPort2, .identifier],
            .screen: [.screen, .identifier],
            .singularAdapterMode: [.expansionPort, .singularAdapterMode],
            .userPort: [.userPort, .identifier],
            .userPortJoystick1: [.userPort, .joystickPort, .identifier],
            .userPortJoystick2: [.userPort, .joystickPort2, .identifier]
        ]
        
        init(migrateVersion1 node: Node) {
            self[.formatVersion] = .integer(2)
            for (oldKey, keyPath) in format1Migration {
                guard let newKey = keyPath.last else { continue }
                if let value = node[oldKey] {
                    var newNode: Node? = self
                    for newKey in keyPath.dropLast() {
                        newNode = newNode?.node(for: newKey, create: true)
                    }
                    newNode?[newKey] = value
                }
            }
        }
        
        public func encode(to encoder: Encoder) throws {
            var container = encoder.container(keyedBy: MachineConfig.Key.self)
            
            for (key, value) in values {
                switch value {
                case .dictionary(let child):
                    if child.isEmpty() {
                        continue
                    }
                default:
                    break
                }
                try container.encode(value, forKey: key)
            }
        }

        public static func == (lhs: MachineConfigStore.Node, rhs: MachineConfigStore.Node) -> Bool {
            return lhs.values == rhs.values
        }
        
        public subscript(_ key: MachineConfig.Key) -> Value? {
            get {
                return values[key]
            }
            set {
                values[key] = newValue
            }
        }
        
        public func isEmpty() -> Bool {
            for (_, value) in values {
                switch value {
                case .boolean(_), .integer(_), .number(_), .string(_):
                    return false
                    
                case .dictionary(let node):
                    if !node.isEmpty() {
                        return false
                    }
                }
            }
            
            return true
        }
        
        public func integer(for key: MachineConfig.Key) -> Int? {
            guard let value = self[key] else { return nil }
            switch value {
            case .integer(let integer):
                return integer
                
            default:
                return nil
            }
        }
        
        public func node(for key: MachineConfig.Key, create: Bool = false) -> Node? {
            if let value = self[key] {
                switch value {
                case .dictionary(let node):
                    return node
                    
                default:
                    return nil
                }
            }
            else if create {
                let node = Node()
                self[key] = .dictionary(node)
                return node
            }
            
            return nil
        }
    }
    
    var root = Node()
    var url: URL?
    
    init(contentsOf url: URL) throws {
        self.url = url

        let data = try Data(contentsOf: url)
        let decoder = JSONDecoder()
        
        let node = try decoder.decode(Node.self, from: data)
        
        let formatVersion = node.integer(for: .formatVersion) ?? 1
        switch formatVersion {
        case 1:
            root = Node(migrateVersion1: node)
        case 2:
            root = node
            
        default:
            throw MyError() // TOOD: unknown format version
        }
    }
    
    func save() throws {
        guard let url = url else { return }
        let encoder = JSONEncoder()
        let data = try encoder.encode(root)
        try data.write(to: url, options: .atomic)
    }
}
