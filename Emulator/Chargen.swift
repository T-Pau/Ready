/*
 Chargen.swift -- Render C64 Text
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
