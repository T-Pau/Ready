/*
 Ramlink.swift -- CMD RAMLink
 Copyright (C) 2021 Dieter Baron
 
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

import UIKit
import C64UIComponents

public struct Ramlink {
    public var identifier: String
    public var name: String
    public var variantName: String?
    public var icon: UIImage?
    public var connector: ConnectorType
    public var url: URL?
    public var size: Int

    public init?(url: URL) {
        do {
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            guard let fileSize = attributes[FileAttributeKey.size] as? UInt64 else { return nil }
            guard fileSize % (1024 * 1024) == 0 else { return nil }
            guard [1, 2, 3, 4, 5, 8, 9, 12, 13, 16].contains(fileSize / (1024 * 1024)) else { return nil }

            identifier = url.path
            size = Int(fileSize)
            name = "CMD RAMLink"
            self.url = url
            variantName = "\(url.lastPathComponent) (\(size / (1024*1024))mb)"
            connector = .c64ExpansionPort
            icon = UIImage(named: "CMD RAMLink")
        }
        catch {
            return nil
        }
    }
}

extension Ramlink: Cartridge {
    public var cartridgeType: CartridgeType {
        return .main
    }
    
    public var resources: [MachineOld.ResourceName: MachineOld.ResourceValue] {
        guard let url = url, let bios = Defaults.standard.biosCmdRamlink else { return [:] }
        return [
            .RAMLINK: .Bool(true),
            .RAMLINKImageWrite: .Bool(true),
            .RAMLINKfilename: .String(url.path),
            .RAMLINKsize: .Int(Int32(size / (1024 * 1024))),
            .CartridgeType: .Int(73),
            .CartridgeFile: .String(Defaults.biosURL.appendingPathComponent(bios).path)
        ]
    }
}


extension Ramlink: MachinePart {
    public var priority: Int {
        return MachinePartNormalPriority
    }
}
