/*
 MachinePart.swift -- Static information about a Device
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

import UIKit

public let MachinePartLowPriority = 50
public let MachinePartNormalPriority = 100
public let MachinePartHighPriority = 200


public protocol MachinePart {
    var identifier: String { get }
    var name: String { get }
    var fullName: String { get }
    var variantName: String? { get }
    var icon: UIImage? { get }
    var smallIcon: UIImage? { get }
    var priority: Int { get }
    var ports: [Port] { get }
    var connector: ConnectorType { get }
    func emulatorInfo(for type: Computer.ComputerType) -> EmulatorInfo?
}

extension MachinePart {
    public var fullName: String { return name }
    public var smallIcon: UIImage? { return icon  }
    public var variantName: String? { return nil }
    public var ports: [Port] { return [] }
    
    public func emulatorInfo(for type: Computer.ComputerType) -> EmulatorInfo? {
        return nil
    }
    
    public func isCompatible(with port: Port) -> Bool {
        return connector == .none || port.connectorTypes.contains(connector)
    }
    
    public func has(port key: MachineConfig.Key) -> Bool {
        return port(for: key) != nil
    }
    
    public func port(for key: MachineConfig.Key) -> Port? {
        return ports.first(where: { $0.key == key })
    }
    
    public func portNew(for key: MachineConfig.Key) -> Port? {
        // TODO: use function above on switch from MachineConfig.Key to MachineConfig.Key
        return nil
    }
}

public struct MachinePartSection {
    public var title: String?
    public var parts: [MachinePart]
    
    public init(title: String?, parts: [MachinePart]) {
        self.title = title
        self.parts = parts
    }
}

public struct MachinePartList {
    public var sections: [MachinePartSection]
    
    public init(sections: [MachinePartSection]) {
        self.sections = sections
    }
    
    public subscript(_ indexPath: IndexPath) -> MachinePart {
        return sections[indexPath.section].parts[indexPath.row]
    }
    
    public func filter(_ isIncluded: ((MachinePart) throws -> Bool)) rethrows -> MachinePartList {
        return MachinePartList(sections: try sections.map({
            MachinePartSection(title: $0.title, parts: try $0.parts.filter({ try isIncluded($0) }))
        }).filter({
            !$0.parts.isEmpty
        }))
    }
    
    public var parts: [MachinePart] {
        return sections.flatMap({ $0.parts })
    }
    
    public var isEmpty: Bool {
        return sections.isEmpty
    }
    
    public mutating func insert(_ machienPart: MachinePart, at indexPath: IndexPath) {
        sections[indexPath.section].parts.insert(machienPart, at: indexPath.row)
    }
}
