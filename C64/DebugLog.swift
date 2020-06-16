/*
 DebugLog.swift
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
import Emulator

class DebugLog {
    static var shared = DebugLog()
    
    private let fileHandle: FileHandle?
    
    init() {
        do {
            let fileManager = FileManager.default
            let fileURL = Defaults.libraryURL.appendingPathComponent("Debug Log.txt")
            if !fileManager.fileExists(atPath: fileURL.path) {
                fileManager.createFile(atPath: fileURL.path, contents: nil, attributes: nil)
            }
            try fileHandle = FileHandle(forUpdating: fileURL)
            fileHandle?.seekToEndOfFile()
        }
        catch {
            fileHandle = nil
        }
    }
    
    func log(_ message: String) {
        let dateFormatter = DateFormatter()
        dateFormatter.setLocalizedDateFormatFromTemplate("yyyy-MM-dd'T'HH:mm:ss.SSS")
        let logline = dateFormatter.string(from: Date()) + " \(message)\n"
        guard let data = logline.data(using: .utf8) else { return }
        fileHandle?.write(data)
        fileHandle?.synchronizeFile()
    }
}
