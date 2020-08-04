/*
 ByteStream.swift -- High Level Read/Write Access to Binary Data
 Copyright (C) 2020 Dieter Baron
 
 The author can be contacted at <dillo@tpau.group>
 
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

struct ByteStream {
    var bytes: ArraySlice<UInt8>
    var littleEndian: Bool

    var index: Int
    
    init(bytes: [UInt8], range: Range<Int>? = nil, littleEndian: Bool = true) {
        self.init(bytes: ArraySlice<UInt8>(bytes), range: range, littleEndian: littleEndian)
    }

    init(bytes: ArraySlice<UInt8>, range: Range<Int>? = nil, littleEndian: Bool = true) {
        if let range = range {
            self.bytes = bytes[range]
        }
        else {
            self.bytes = bytes
        }
        self.littleEndian = littleEndian
        
        self.index = self.bytes.startIndex
    }
    
    var offset: Int {
        get {
            return index - bytes.startIndex
        }
        set {
            index = newValue + bytes.startIndex
            if index < bytes.startIndex {
                index = bytes.startIndex
            }
            else if index > bytes.endIndex {
                index = bytes.endIndex
            }
        }
    }
    
    var count: Int {
        return bytes.count
    }
    
    mutating func getStream(_ size: Int) -> ByteStream? {
        guard let b = getBytes(size) else { return nil }
        
        return ByteStream(bytes: b, littleEndian: littleEndian)
    }
    
    mutating func getByte() -> UInt8? {
        guard available(size: 1) else { return nil }
        
        defer {
            index += 1
        }
        return bytes[index]
    }
    
    mutating func getBytes(_ size: Int) -> ArraySlice<UInt8>? {
        guard available(size: size) else { return nil }
        
        defer {
            index += size
        }
        
        return bytes[index ..< index + size]
    }
    
    mutating func getUInt16(littleEndian: Bool? = nil) -> UInt16? {
        guard available(size: 2) else { return nil }

        defer {
            index += 2
        }

        if littleEndian ?? self.littleEndian {
            return UInt16(bytes[index]) | (UInt16(bytes[index + 1]) << 8)
        }
        else {
            return (UInt16(bytes[index]) << 8) | UInt16(bytes[index + 1])
        }
    }
    
    mutating func getUInt24(littleEndian: Bool? = nil) -> UInt32? {
        guard available(size: 3) else { return nil }
        
        defer {
            index += 3
        }

        if littleEndian ?? self.littleEndian {
            return UInt32(bytes[index]) | (UInt32(bytes[index + 1]) << 8) | (UInt32(bytes[index + 2]) << 16)
        }
        else {
            return (UInt32(bytes[index]) << 16) | (UInt32(bytes[index + 1]) << 8) | UInt32(bytes[index + 2])
        }
    }

    mutating func getUInt32(littleEndian: Bool? = nil) -> UInt32? {
        guard available(size: 4) else { return nil }
        
        defer {
            index += 4
        }

        if littleEndian ?? self.littleEndian {
            return UInt32(bytes[index]) | (UInt32(bytes[index + 1]) << 8) | (UInt32(bytes[index + 2]) << 16) | (UInt32(bytes[index + 3]) << 24)
        }
        else {
            return (UInt32(bytes[index]) << 24) | (UInt32(bytes[index + 1]) << 16) | (UInt32(bytes[index + 2]) << 8) | UInt32(bytes[index + 3])
        }
    }
    
    mutating func getUInt64(littleEndian: Bool? = nil) -> UInt64? {
        guard available(size: 8) else { return nil }
        
        defer {
            index += 8
        }

        if littleEndian ?? self.littleEndian {
            let value = UInt64(bytes[index]) | (UInt64(bytes[index + 1]) << 8) | (UInt64(bytes[index + 2]) << 16) | (UInt64(bytes[index + 3]) << 24)
            return value | (UInt64(bytes[index + 4]) << 32) | (UInt64(bytes[index + 5]) << 40) | (UInt64(bytes[index + 6]) << 48) | (UInt64(bytes[index + 7]) << 56)
        }
        else {
            let value = UInt64(bytes[index] << 56) | (UInt64(bytes[index + 1]) << 48) | (UInt64(bytes[index + 2]) << 40) | (UInt64(bytes[index + 3]) << 32)
            return value | (UInt64(bytes[index + 4]) << 24) | (UInt64(bytes[index + 5]) << 16) | (UInt64(bytes[index + 6]) << 8) | UInt64(bytes[index + 7])
        }
    }
    
    mutating func put(byte: UInt8) {
        guard available(size: 1) else { return }

        bytes[index] = byte
        index += 1
    }

    mutating func put(uInt16 value: UInt16, littleEndian: Bool? = nil) {
        guard available(size: 2) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[index] = UInt8(value & 0xff)
            bytes[index + 1] = UInt8(value >> 8)
        }
        else {
            bytes[index] = UInt8(value >> 8)
            bytes[index + 1] = UInt8(value & 0xff)
        }
        index += 2
    }

    mutating func put(uInt24 value: UInt32, littleEndian: Bool? = nil) {
        guard available(size: 3) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[index] = UInt8(value & 0xff)
            bytes[index + 1] = UInt8((value >> 8) & 0xff)
            bytes[index + 2] = UInt8((value >> 16) & 0xff)
        }
        else {
            bytes[index] = UInt8((value >> 16) & 0xff)
            bytes[index + 1] = UInt8((value >> 8) & 0xff)
            bytes[index + 2] = UInt8(value & 0xff)
        }
        index += 3
    }

    mutating func put(uInt32 value: UInt32, littleEndian: Bool? = nil) {
        guard available(size: 4) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[index] = UInt8(value & 0xff)
            bytes[index + 1] = UInt8((value >> 8) & 0xff)
            bytes[index + 2] = UInt8((value >> 16) & 0xff)
            bytes[index + 3] = UInt8(value >> 24)
        }
        else {
            bytes[index ] = UInt8(value >> 24)
            bytes[index + 1] = UInt8((value >> 16) & 0xff)
            bytes[index + 2] = UInt8((value >> 8) & 0xff)
            bytes[index + 3] = UInt8(value & 0xff)
        }
        index += 4
    }

    mutating func put(uInt64 value: UInt64, littleEndian: Bool? = nil) {
        guard available(size: 8) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[index] = UInt8(value & 0xff)
            bytes[index + 1] = UInt8((value >> 8) & 0xff)
            bytes[index + 2] = UInt8((value >> 16) & 0xff)
            bytes[index + 3] = UInt8((value >> 24) & 0xff)
            bytes[index + 4] = UInt8((value >> 32) & 0xff)
            bytes[index + 5] = UInt8((value >> 40) & 0xff)
            bytes[index + 6] = UInt8((value >> 48) & 0xff)
            bytes[index + 7] = UInt8(value >> 56)
        }
        else {
            bytes[index ] = UInt8(value >> 56)
            bytes[index + 1] = UInt8((value >> 48) & 0xff)
            bytes[index + 2] = UInt8((value >> 40) & 0xff)
            bytes[index + 3] = UInt8((value >> 32) & 0xff)
            bytes[index + 4] = UInt8((value >> 24) & 0xff)
            bytes[index + 5] = UInt8((value >> 16) & 0xff)
            bytes[index + 6] = UInt8((value >> 8) & 0xff)
            bytes[index + 7] = UInt8(value & 0xff)
        }
        index += 8
    }

    func available(size: Int) -> Bool {
        return index + size <= bytes.endIndex
    }
}
