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

import Foundation

struct CasstteDrive: MachinePart {
    var identifier: String
    var name: String
    var fullName: String
    var icon: UIImage?
    var priority: Int
    
    var caseColor: UIColor?
    
    init(identifier: String, name: String, fullName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, caseColorName: String?) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        if let caseColorName = caseColorName {
            self.caseColor = UIColor(named: caseColorName)
        }
    }
    
    var hasStatus: Bool { return identifier != "none" }
    
    static let none = CasstteDrive(identifier: "none", name: "None", iconName: nil, priority: 0, caseColorName: nil)
    
    static let drives = [
        none,
        CasstteDrive(identifier: "1530", name: "1530", fullName: "Commodore 1530 C2N", iconName: "Commodore 1530 C2N", caseColorName: "1530 C2N Case")
    ]
    
    static private var byIdentifier = [String: CasstteDrive]()
    
    static func drive(identifier: String) -> CasstteDrive? {
        if byIdentifier.isEmpty {
            for drive in drives {
                byIdentifier[drive.identifier] = drive
            }
        }
        
        return byIdentifier[identifier]
    }
}
