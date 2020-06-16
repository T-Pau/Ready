/*
 DummyMachinePart.swift -- Machine Part without Real Counterpart
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

public struct DummyMachinePart: MachinePart {
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var smallIcon: UIImage?
    public var priority: Int { return MachinePartLowPriority }

    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String? = nil) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        else {
            self.icon = nil
        }
        self.smallIcon = icon
    }
    
    public init(identifier: String, fullName: String, annotateName: Bool, basePart: MachinePart) {
        self.identifier = identifier
        name = basePart.name
        if annotateName {
            name = "\(name) (Automatic)"
        }
        self.fullName = fullName
        icon = basePart.icon
        smallIcon = basePart.smallIcon
        var description = basePart.fullName
        if let variantName = basePart.variantName {
            description += " (\(variantName))"
        }
        variantName = description
    }
        
    public static let none = DummyMachinePart(identifier: "none", name: "None")
}
