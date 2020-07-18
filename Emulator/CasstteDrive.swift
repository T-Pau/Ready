/*
 CassetteDrive.swift -- Cassete Drive Harware Part
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

import UIKit

import RetroMedia

public struct CasstteDrive: MachinePart {
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var priority: Int
    public var connector: ConnectorType
    public var ports: [Port]
    
    public var caseColor: UIColor?
    
    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, connector: ConnectorType, mediaType: MediaType, caseColorName: String?) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        if let caseColorName = caseColorName {
            self.caseColor = UIColor(named: caseColorName)
        }
        self.connector = connector
        self.ports = [Port(name: "Cassette", key: .medium, connectorTypes: [.media(mediaType)])]
    }
    
    public var hasStatus: Bool { return identifier != "none" }
    
    public static let none = CasstteDrive(identifier: "none", name: "None", iconName: nil, priority: 0, connector: .none, mediaType: .cassetteCommodore, caseColorName: nil)
    
    public static let drives = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: [
            none
        ]),
        
        MachinePartSection(title: "Datasette", parts: [
            CasstteDrive(identifier: "1530", name: "1530", fullName: "Commodore 1530 C2N", iconName: "Commodore 1530 C2N", connector: .commodoreTapePort, mediaType: .cassetteCommodore, caseColorName: "1530 C2N Case"),

            CasstteDrive(identifier: "1530 Plus/4", name: "1530", fullName: "Commodore 1530 C2N", variantName: "for C16, Plus/4", iconName: "Commodore 1530 C2N Black", connector: .plus4TapePort, mediaType: .cassetteCommodore, caseColorName: "C16 Case"),
            
            CasstteDrive(identifier: "Tape Recorder", name: "Recorder", fullName: "Tape Recorder", iconName: "Tape Recorder", connector: .audioJackTape, mediaType: .cassetteSpectrum, caseColorName: "Tape Recorder Case"),

            CasstteDrive(identifier: "Atari 1010", name: "Atari 1010", fullName: "Atari 1010", iconName: "Atari 1010", connector: .atariSio, mediaType: .cassetteAtari, caseColorName: "Atari XL Case")

        ])
    ])
    
    static private var byIdentifier = [String: CasstteDrive]()
    
    public static func drive(identifier: String) -> CasstteDrive? {
        if byIdentifier.isEmpty {
            for drive in drives.parts {
                byIdentifier[drive.identifier] = drive as? CasstteDrive
            }
        }
        
        return byIdentifier[identifier]
    }
}
