/*
 Device.swift -- Configured Instance of a MachinePart
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

public class Device {
    public var part: MachinePart
    public var config: MachineConfig.Node
    
    public var devices: [MachineConfig.Key: Device]
    
    public convenience init?(config: MachineConfig.Node, compatibleWith port: Port? = nil) {
        guard let potentialPart = MachinePartRegister.default.part(for: config) else { return nil }
        if let port = port {
            guard potentialPart.isCompatible(with: port) else { return nil }
        }
        self.init(part: potentialPart, config: config)
    }
    
    public init(part: MachinePart, config: MachineConfig.Node) {
        self.part = part
        self.config = config
        
        devices = [:]
        for port in part.ports {
            guard let deviceConfig = config.node(for: port.key) else { continue }
            devices[port.key] = Device(config: deviceConfig, compatibleWith: port)
        }
    }
    
    func device(for key: MachineConfig.Key) -> Device? {
        guard let port = part.portNew(for: key) else { return nil }
        guard let deviceConfig = config.node(for: key, create: true) else { return nil }
        return Device(config: deviceConfig, compatibleWith: port)
    }
}
