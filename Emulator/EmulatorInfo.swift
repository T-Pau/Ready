/*
 EmulatorInfo.swift -- Emulator-Specific Information for MachinePart
 Copyright (C) 2020 Dieter Baron
 
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
