/*
 CartridgeImage.swift -- Access Tape Images
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

public protocol TapeImage {
    static func image(from url: URL) -> TapeImage?
    static func image(from bytes: Data) -> TapeImage?

    var url: URL? { get set }
    var name: String? { get }
}

extension TapeImage {
    public static func image(directory: URL, file: String?) -> TapeImage? {
        guard let file = file else { return nil }
        return image(from: directory.appendingPathComponent(file))
    }
    
    public static func image(from url: URL) -> TapeImage? {
        guard let bytes = FileManager.default.contents(atPath: url.path) else { return nil }
        guard var image = image(from: bytes) else { return nil }
        image.url = url
        return image
    }
    
    public static func image(from bytes: Data) -> TapeImage? {
        if let image = T64Image(bytes: bytes) {
            return image
        }
        if let image = TapImage(bytes: bytes) {
            return image
        }
        return nil
    }

}

public struct T64Image: TapeImage {
    public var bytes: Data
    public var url: URL? = nil
    public var name: String?
    
    private static let signatures = [
        "C64 tape image file",
        "C64S tape file",
        "C64S tape image file"
    ]
    
    public init?(bytes: Data) {
        guard bytes.count > 64 else { return nil } // too short for header
        guard let signature = String(bytes: bytes[0...19], encoding: .ascii) else { return nil }
        
        guard T64Image.signatures.contains(signature.trimmingCharacters(in: CharacterSet(charactersIn: "\0"))) else { return nil }
        
        name = String(bytes: bytes[0x28..<0x40], encoding: .ascii)?.trimmingCharacters(in: CharacterSet(charactersIn: " \0")) // TODO: correct encoding?
        if name?.isEmpty ?? false {
            name = nil
        }
        
        // TODO: read directory
        
        self.bytes = bytes
    }
}

public struct TapImage: TapeImage {
    public var bytes: Data
    public var url: URL? = nil
    public var name: String? { return nil }
    
    public init?(bytes: Data) {
        guard bytes.count > 14 else { return nil } // too short for header
        guard String(bytes: bytes[0..<0xc], encoding: .ascii) == "C64-TAPE-RAW" else { return nil }

        self.bytes = bytes
    }
}
