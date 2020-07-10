//
//  Machine.swift
//  Emulator
//
//  Created by Dieter Baron on 07.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
