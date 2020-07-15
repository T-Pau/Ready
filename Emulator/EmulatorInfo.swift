//
//  EmulatorInfo.swift
//  Emulator
//
//  Created by Dieter Baron on 11.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import Foundation

public protocol EmulatorInfo {
    
}

// TODO: move to emulator framework

public struct Atari800EmulatorInfo : EmulatorInfo {
    public var arguments: [String]
    
    public init(arguments: [String]) {
        self.arguments = arguments
    }
    
    public init(xlOS osRom: String, basicRom: String, ntsc: Bool = false) {
        arguments = [
            "-xl",
            "-xlxe_rom", "@DATADIR@/\(osRom).rom",
            "-basic_rom", "@DATADIR@/\(basicRom).rom",
            ntsc ? "-ntsc" : "-pal"
        ]
    }
}

public struct FuseEmulatorInfo : EmulatorInfo {
    public var arguments: [String]
}

public struct ViceEmulatorInfo: EmulatorInfo {
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue]
}

public struct X16EmulatorInfo : EmulatorInfo {
    public var arguments: [String]
}
