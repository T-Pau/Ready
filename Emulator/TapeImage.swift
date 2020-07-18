/*
 TapeImage.swift -- Access Tape Images
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
        if let image = SpectrumTapImage(bytes: bytes) {
            return image
        }
        if let image = SpectrumTZXImage(bytes: bytes) {
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

public struct SpectrumTapImage: TapeImage {
    public var bytes: Data
    public var url: URL?
    public var name: String?
    
    public init?(bytes: Data) {
        guard bytes.count > 21 else { return nil } // too short for header
        guard bytes[0] == 19 && bytes[1] == 0 && bytes[2] == 0 else { return nil } // expect header block
        
        var checksum: UInt8 = 0
        for byte in bytes[2 ..< 21] {
            checksum ^= byte
        }
        guard checksum == 0 else { return nil }
        
        self.name = String(bytes: bytes[4 ..< 14], encoding: .ascii)?.trimmingCharacters(in: .whitespaces)
        self.bytes = bytes
    }
}

public struct SpectrumTZXImage: TapeImage {
    public var bytes: Data
    public var url: URL?
    public var name: String?
    
    public init?(bytes: Data) {
        guard bytes.count > 8 else { return nil } // too short for header
        guard String(bytes: bytes[0..<7], encoding: .ascii) == "ZXTape!" && bytes[7] == 26 else { return nil }
        
        self.bytes = bytes
    }
}
