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
}

public struct FuseEmulatorInfo : EmulatorInfo {
    public var arguments: [String]
}

public struct ViceEmulatorInfo: EmulatorInfo {
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue]
}
