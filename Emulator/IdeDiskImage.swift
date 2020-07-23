/*
 IdeDiskDrive.swift -- Information about IDE Disk Drives
 Copyright (C) 2019 Dieter Baron
 
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

public struct IdeDiskImage {
    public enum IdeType {
        case cd
        case compactFlash
        case sdCard
        case hardDisk
        
        public init?(pathExtension: String) {
            switch pathExtension {
            case "cfa":
                self = .compactFlash
            case "hdd":
                self = .hardDisk
            case "iso":
                self = .cd
            case "sdcard":
                self = .sdCard
            default:
                return nil
            }
        }
    }
    
    public var ideType: IdeType
    public var url: URL?
    public var size: Int
    
    public init?(url: URL) {
        guard let ideType = IdeType(pathExtension: url.pathExtension) else { return nil }
        
        self.ideType = ideType
        self.url = url
        do {
            let values = try url.resourceValues(forKeys: [.isDirectoryKey, .fileSizeKey])
            if values.isDirectory ?? false {
                // TODO: get size of directory?
                // TODO: get listing
                self.size = 0
            }
            else {
                guard let size = values.fileSize else { return nil }
                self.size = size
            }
        }
        catch {
            return nil
        }
    }
}

