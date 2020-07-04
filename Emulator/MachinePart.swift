/*
 MachinePart.swift -- Static information about a Device
 Copyright (C) 2019-2020 Dieter Baron
 
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
}

extension MachinePart {
    public var fullName: String { return name }
    public var smallIcon: UIImage? { return icon  }
    public var variantName: String? { return nil }
    public var ports: [Port] { return [] }
    
    public func isCompatible(with port: Port) -> Bool {
        return connector == .none || port.connectorTypes.contains(connector)
    }
    
    public func has(port key: MachineConfigOld.Key) -> Bool {
        return port(for: key) != nil
    }
    
    public func port(for key: MachineConfigOld.Key) -> Port? {
        return ports.first(where: { $0.key == key })
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
