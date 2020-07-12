/*
 MachinConfigStore.swift -- Combined View of Layered Machine Configuration
 Copyright (C) 2019-2020 Dieter Baron
 
 This file is part of Ready, a Home Computer emulator for iOS.
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

public class MachineConfig {
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
        case filename
        case formatVersion
        case identifier
        case joystickPort
        case joystickPort2
        case medium
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
        
        
        // compatibility methods to allow replacing MachineConfig.Key
        public var driveNumber: Int? {
            switch  self {
            case .diskDrive8:
                return 8
            case .diskDrive9:
                return 9
            case .diskDrive10:
                return 10
            case .diskDrive11:
                return 11
            default:
                return nil
            }
        }

        public var expansionPortIndex: Int? {
            switch self {
            case .expansionPort:
                return 0
            case .expansionPort1:
                return 1
            case .expansionPort2:
                return 2
            default:
                return nil
            }
        }
        
        public var supportsAuto: Bool {
            switch self {
            case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11, .cassetteDrive, .expansionPort, .expansionPort1, .expansionPort2:
                return true
            default:
                return false
            }
        }
    }

    public class Node {
        private var layers: [MachineConfigStore.Node]
        
        init(layers: [MachineConfigStore.Node]) {
            self.layers = layers
        }
        
        func node(for key: Key, skipFirstLayer: Bool = false, create: Bool = false) -> Node? {

            var nodes = [MachineConfigStore.Node]()
            
            for layer in layers.dropFirst() {
                if let node = layer.node(for: key) {
                    nodes.append(node)
                }
            }
            
            if !skipFirstLayer {
                let createFirstLayer = create || !nodes.isEmpty
                if let node = layers.first?.node(for: key, create: createFirstLayer) {
                    nodes.insert(node, at: 0)
                }
            }

            if nodes.isEmpty {
                return nil
            }
            return Node(layers: nodes)
        }
        
        func scalarValue(for key: Key, skipFirstLayer: Bool = false) -> MachineConfigStore.Value? {
            let relevantLayers = skipFirstLayer ? layers.dropFirst() : ArraySlice(layers)
            for layer in relevantLayers {
                if let value = layer[key] {
                    switch value {
                    case .dictionary(_):
                        return nil
                    default:
                        return value
                    }
                }
            }
            return nil
        }
        
        func boolean(for key: Key, skipFirstLayer: Bool = false) -> Bool? {
            if let value = scalarValue(for: key, skipFirstLayer: skipFirstLayer) {
                switch value {
                case .boolean(let v):
                    return v
                default:
                    break
                }
            }
            return nil
        }
        
        public var identifier: String? { getIdentifier() }
        public var identifierSkippingFirstLayer: String? { getIdentifier(skipFirstLayer: true) }
        private func getIdentifier(skipFirstLayer: Bool = false) -> String? {
            let relevantLayers = skipFirstLayer ? layers.dropFirst() : ArraySlice(layers)
            for layer in relevantLayers {
                if let value = layer[.identifier] {
                    switch value {
                    case .string(let id):
                        if id == "auto" {
                            continue
                        }
                        else if id == "none" {
                            return nil
                        }
                        else {
                            return id
                        }
                        
                    default:
                        return nil
                    }
                }
            }
            return nil
        }

        func integer(for key: Key, skipFirstLayer: Bool = false) -> Int? {
            if let value = scalarValue(for: key, skipFirstLayer: skipFirstLayer) {
                switch value {
                case .integer(let v):
                    return v
                default:
                    break
                }
            }
            return nil
        }

        func number(for key: Key, skipFirstLayer: Bool = false) -> Double? {
            if let value = scalarValue(for: key, skipFirstLayer: skipFirstLayer) {
                switch value {
                case .number(let v):
                    return v
                default:
                    break
                }
            }
            return nil
        }

        func string(for key: Key, skipFirstLayer: Bool = false) -> String? {
            if let value = scalarValue(for: key, skipFirstLayer: skipFirstLayer) {
                switch value {
                case .string(let v):
                    return v
                default:
                    break
                }
            }
            return nil
        }
    }

    private var layers: [MachineConfigStore]
    
    public init(layers: [MachineConfigStore]) {
        self.layers = layers
    }
    
    public func save() throws {
        try layers[0].save()
    }
    
    public var root: Node {
        return Node(layers: layers.map { $0.root })
    }
    
    public func node(for key: Key, create: Bool = false) -> Node? {
        return root.node(for: key, create: create)
    }
}
