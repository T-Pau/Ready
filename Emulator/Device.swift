/*
 Device.swift -- Configured Instance of a MachinePart
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
