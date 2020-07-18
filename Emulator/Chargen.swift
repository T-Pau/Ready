/*
 Chargen.swift -- Render C64 Text
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

import UIKit

public struct Chargen {
    public var chars = [UInt8](repeating: 0, count: 256 * 8 * 8)
    
    public init?(bytes: Data) {
        guard bytes.count == 2048 else { return nil }

        for char in (0 ... 255) {
            for y in (0 ... 7) {
                let byte = bytes[Int(char * 8) + y]
                for x in (0 ... 7) {
                    chars[char * 64 + y * 8 + x] = byte & UInt8((1 << (7-x))) != 0 ? 0xff : 0x00
                }
            }
        }
    }
    
    public init?(contentsOf url: URL, bank: Int = 0) throws {
        let data = try Data(contentsOf: url)
        
        guard data.count >= (bank + 1) * 2048 else { return nil }
        self.init(bytes: Data(data[(bank * 2048) ..< ((bank + 1) * 2048)]))
    }
    
    private struct BuiltinKey: Hashable {
        var name: String
        var subdirectory: String
        var bank: Int
    }
    
    private static var builtin = [BuiltinKey: Chargen?]()
    
    private static func load(name: String, subdirectory: String, bank: Int) -> Chargen? {
        guard let url = Bundle.main.url(forResource: name, withExtension: "", subdirectory: subdirectory) else { return nil }

        do {
            return try Chargen(contentsOf: url, bank: bank)
        }
        catch {
            return nil
        }
    }
    
    public init?(named name: String, subdirectory: String, bank: Int = 0) {
        let key = BuiltinKey(name: name, subdirectory: subdirectory, bank: bank)
        let chargen: Chargen?
        
        if let cached = Chargen.builtin[key] {
            chargen = cached
        }
        else {
            chargen = Chargen.load(name: name, subdirectory: subdirectory, bank: bank)
            Chargen.builtin[key] = chargen
        }
        
        if let chargen = chargen {
            self = chargen
        }
        else {
            return nil
        }
    }
    
    public func render(line: [UInt8]) -> UIImage {
        let lineWidth = line.count * 8
        var data = Data(count: lineWidth * 8)

        for (index, char) in line.enumerated() {
            for y in (0 ... 7) {
                for x in (0 ... 7) {
                    data[index * 8 + x + y * lineWidth] = chars[Int(char) * 64 + y * 8 + x]
                }
            }
        }
        
        let ciImage = CIImage(bitmapData: data, bytesPerRow: lineWidth, size: CGSize(width: lineWidth, height: 8), format: .A8, colorSpace: nil)
        
        return UIImage(ciImage: ciImage).withRenderingMode(.alwaysTemplate)
    }
    
    private static let asciiToScreenMap: [UInt8] = [
        0x00, 0x20, 0x40, 0x00,
        0x40, 0x60, 0x00, 0x60
    ]
    private static let petsciiToScreenMap: [UInt8] = [
        0x00, 0x20, 0x00, 0x40,
        0x00, 0x60, 0x40, 0x60
    ]
    
    public static func petsciiToScreen(_ string: [UInt8]) -> [UInt8] {
        return string.map { Chargen.petsciiToScreenMap[Int($0 >> 5)] | ($0 & 0x1f) }
    }

    public static func asciiToScreen(_ string: [UInt8]) -> [UInt8] {
        return string.map { Chargen.asciiToScreenMap[Int($0 >> 5)] | ($0 & 0x1f) }
    }
}
