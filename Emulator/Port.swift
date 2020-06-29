/*
 Port.swift -- Specification of a port on a device
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

public struct Port {
    var name: String
    var key: MachineConfig.Key
    var connectorTypes: Set<ConnectorType>
    var supportsHotSwap: Bool
    var iconWidth: Int
    var iconHeight: Int
    
    public init(name: String, key: MachineConfig.Key, connectorTypes: Set<ConnectorType>, supportsHotSwap: Bool = false, iconWidth: Int = 1, iconHeight: Int = 1) {
        self.name = name
        self.key = key
        self.connectorTypes = connectorTypes
        self.supportsHotSwap = supportsHotSwap
        self.iconWidth = iconWidth
        self.iconHeight = iconHeight
    }
}

