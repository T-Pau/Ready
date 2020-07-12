/*
 CassetteDrive.swift -- Cassete Drive Harware Part
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
