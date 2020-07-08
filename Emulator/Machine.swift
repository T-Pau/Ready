//
//  Machine.swift
//  Emulator
//
//  Created by Dieter Baron on 07.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import Foundation
import C64UIComponents

public class Machine {
    public var config: MachineConfig
    public var media: [MediaItem]
    
    public init(config: MachineConfig, media: [MediaItem]) {
        self.config = config
        self.media = media
    }
    
    
}
