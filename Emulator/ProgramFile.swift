/*
 ProgramFile.swift -- Access 64 Program Files
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

import Foundation

public struct ProgramFile {
    public var bytes: Data
    public var name: String?
    public var url: URL?

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
