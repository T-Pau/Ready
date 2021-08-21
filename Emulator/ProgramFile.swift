/*
 ProgramFile.swift -- Access 64 Program Files
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

public struct ProgramFile {
    public var bytes: Data
    public var name: String?
    public var url: URL?
    public var connector: ConnectorType { return .programCommodore }

    public init?(directory: URL, file: String?) {
        guard let file = file else { return nil }
        self.init(url: directory.appendingPathComponent(file))
    }
    
    public init?(url: URL) {
        do {
            bytes = try Data(contentsOf: url)
        }
        catch {
            return nil
        }
        self.url = url

        if String(bytes: bytes[0x00 ..< 0x08], encoding: .ascii) == "C64File\0" {
            name = String(bytes: bytes[0x08 ..< 0x017], encoding: .isoLatin1)?.trimmingCharacters(in: CharacterSet(charactersIn: "\0"))
            // TODO: strip trailing space/0xa0?
        }
    }
}
