//
//  MachineConfigStore.swift
//  Emulator
//
//  Created by Dieter Baron on 04.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import Foundation

struct MyError: Error {
    
}

public class MachineConfigStore {
    public enum Key: String, CaseIterable, CodingKey {
        case borderMode
        case cassetteDrive
        case computer
        case diskDrive
        case diskDrive2
        case diskDrive3
        case diskDrive4
        case expansionPort
        case expansionPort2
        case formatVersion
        case identifier
        case joystickPort
        case joystickPort2
        case screen
        case singularAdapterMode
        case userPort

        // format 1 legacy keys
        case controlPort1 // joystickPort
        case controlPort2 // joystickPort2
        case diskDrive8 // diskDrive
        case diskDrive9 // diskDrive2
        case diskDrive10 // diskDrive3
        case diskDrive11 // diskDrive4
        case expansionPort1 // expansionPort.expansionPort
        case userPortJoystick1 // userPort.joystickPort
        case userPortJoystick2 // userPort.joystickPort
    }
    
    public enum Value: Codable, Equatable {
        case string(_ value: String)
        case integer(_ value: Int)
        case number(_ value: Double)
        case boolean(_ value: Bool)
        case dictionary(_ value: Node)
        
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
        public var values = [Key: Value]()
        
        public init() { }

        required public init(from decoder: Decoder) throws {
            let container = try decoder.container(keyedBy: Key.self)
            for key in container.allKeys {
                values[key] = try container.decode(Value.self, forKey: key)
            }
        }
        
        private var format1Migration: [Key: [Key]] = [
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
            var container = encoder.container(keyedBy: Key.self)
            
            for (key, value) in values {
                try container.encode(value, forKey: key)
            }
        }

        public static func == (lhs: MachineConfigStore.Node, rhs: MachineConfigStore.Node) -> Bool {
            return lhs.values == rhs.values
        }
        
        public subscript(_ key: Key) -> Value? {
            get {
                return values[key]
            }
            set {
                values[key] = newValue
            }
        }
        
        public func int(for key: Key) -> Int? {
            guard let value = self[key] else { return nil }
            switch value {
            case .integer(let integer):
                return integer
                
            default:
                return nil
            }
        }
        
        public func node(for key: Key, create: Bool = false) -> Node? {
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
        
        let formatVersion = node.int(for: .formatVersion) ?? 1
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
