/*
 MachinePartRegister.swift -- Collection of All Known Machine Parts
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

import C64UIComponents
import RetroMedia

public struct MachinePartRegister {
    public struct Section {
        public var title: String?
        public var containsConnectors = Set<ConnectorType>()
        public var parts: [MachinePart]
        
        public init(title: String?, parts: [MachinePart]) {
            self.title = title
            self.parts = parts
            for part in parts {
                containsConnectors.insert(part.connector)
            }
        }
    }
    private var partByIdentifier = [String: MachinePart]()
    private var mediaByFilename = [String: MachinePart]()
    
    public var media = [Medium]()
    public var sections: [Section]
    private(set) public var sortedParts: [MachinePart]

    public func part(for config: MachineConfig.Node) -> MachinePart? {
        guard let identifier = config.identifier else { return nil }

        if (identifier == "media") {
            guard let filename = config.string(for: .filename) else { return nil }
            return mediaByFilename[filename]
        }
        else {
            return partByIdentifier[identifier]
        }
    }
    
    init(sections: [Section]) {
        self.sections = sections
        
        for section in sections {
            for part in section.parts {
                partByIdentifier[part.identifier] = part
            }
        }
        
        sortedParts = sections.flatMap({ $0.parts }).sorted(by: { $0.priority > $1.priority })
    }

    init(media: [Medium]) {
        self =  MachinePartRegister.default
        self.media = media
        
        var parts = [MachinePart]()
        for medium in media {
            guard let part = medium as? MachinePart else { continue }
            mediaByFilename[medium.url.lastPathComponent] = part
            parts.append(part)
        }
        
        sections.insert(Section(title: "Media", parts: parts), at: 0)
    }
    
    public func partList(for port: Port) -> [Section] {
        var filteredSections = [Section]()
        
        for section in sections {
            if section.containsConnectors.intersection(port.connectorTypes).isEmpty {
                continue
            }
            
            let filteredSection = Section(title: section.title, parts: section.parts.filter({ $0.isCompatible(with: port) }))
            if !filteredSection.parts.isEmpty {
                filteredSections.append(filteredSection)
            }
        }
        
        return filteredSections
    }

    public static var `default` = MachinePartRegister(sections: [
        // TODO: move various sections here
    ])
}
