/*
 Machine.swift -- Instance of a Machine (Configuration and Media)
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
import C64UIComponents
import RetroMedia

public class Machine {
    public var config: MachineConfig
    public var media: [Medium]
    public var register: MachinePartRegister
    
    public let port = Port(name: "Computer", key: .computer, connectorTypes: [.computer])
    
    public var computer: Device
    
    public init(config: MachineConfig, media: [Medium]) {
        self.config = config
        self.media = media
        self.register = MachinePartRegister(media: media)
        
        if let computerConfig = config.node(for: .computer), let computer = Device(config: computerConfig, compatibleWith: port) {
            self.computer = computer
        }
        else {
            let computers = register.sortedParts.filter({ $0 is Computer })
            var computer: Computer?
            
            if let medium = media.first {
                computer = computers.first(where: { ($0 as? Computer)?.model.type.supportedMediaTypes.contains(medium.type) ?? false }) as? Computer
            }
            else {
                computer = computers.first as? Computer
            }
            
            let computerPart = computer ?? Computer.c64Pal
            
            self.computer = Device(part: computerPart, config: config.node(for: .computer, create: true)!)
        }
        
        // TODO: resolve auto parts
    }
}
