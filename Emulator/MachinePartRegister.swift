/*
 MachinePartRegister.swift -- Collection of All Known Machine Parts
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
