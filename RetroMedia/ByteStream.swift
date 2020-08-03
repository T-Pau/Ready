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

    var offset: Int
    
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
        
        self.offset = self.bytes.startIndex
    }
    
    mutating func getStream(_ size: Int) -> ByteStream? {
        guard let b = getBytes(size) else { return nil }
        
        return ByteStream(bytes: b, littleEndian: littleEndian)
    }
    
    mutating func getByte() -> UInt8? {
        guard available(size: 1) else { return nil }
        
        defer {
            offset += 1
        }
        return bytes[offset]
    }
    
    mutating func getBytes(_ size: Int) -> ArraySlice<UInt8>? {
        guard available(size: size) else { return nil }
        
        defer {
            offset += size
        }
        return bytes[offset ..< offset + size]
    }
    
    mutating func getUInt16(littleEndian: Bool? = nil) -> UInt16? {
        guard let b = getBytes(4) else { return nil }
        
        if littleEndian ?? self.littleEndian {
            return UInt16(b[b.startIndex]) | (UInt16(b[b.startIndex + 1]) << 8)
        }
        else {
            return (UInt16(b[b.startIndex]) << 8) | UInt16(b[b.startIndex + 1])
        }
    }
    
    mutating func getUInt24(littleEndian: Bool? = nil) -> UInt32? {
        guard let b = getBytes(3) else { return nil }
        
        if littleEndian ?? self.littleEndian {
            return UInt32(b[b.startIndex]) | (UInt32(b[b.startIndex + 1]) << 8) | (UInt32(b[b.startIndex + 2]) << 16)
        }
        else {
            return (UInt32(b[b.startIndex]) << 16) | (UInt32(b[b.startIndex + 1]) << 8) | UInt32(b[b.startIndex + 2])
        }
    }

    mutating func getUInt32(littleEndian: Bool? = nil) -> UInt32? {
        guard let b = getBytes(4) else { return nil }
        
        if littleEndian ?? self.littleEndian {
            return UInt32(b[b.startIndex]) | (UInt32(b[b.startIndex + 1]) << 8) | (UInt32(b[b.startIndex + 2]) << 16) | (UInt32(b[b.startIndex + 3]) << 24)
        }
        else {
            return (UInt32(b[b.startIndex]) << 24) | (UInt32(b[b.startIndex + 1]) << 16) | (UInt32(b[b.startIndex + 2]) << 8) | UInt32(b[b.startIndex + 3])
        }
    }
    
    mutating func getUInt64(littleEndian: Bool? = nil) -> UInt64? {
        guard let b = getBytes(8) else { return nil }
        
        if littleEndian ?? self.littleEndian {
            return UInt32(b[b.startIndex]) | (UInt32(b[b.startIndex + 1]) << 8) | (UInt32(b[b.startIndex + 2]) << 16) | (UInt32(b[b.startIndex + 3]) << 24) | (UInt32(b[b.startIndex + 4]) << 32) | (UInt32(b[b.startIndex + 5]) << 40) | (UInt32(b[b.startIndex + 6]) << 48) | (UInt32(b[b.startIndex + 7]) << 56)
        }
        else {
            return (UInt32(b[b.startIndex]) << 56) | (UInt32(b[b.startIndex + 1]) << 48) | (UInt32(b[b.startIndex + 2]) << 40) | (UInt32(b[b.startIndex + 3]) << 32) | (UInt32(b[b.startIndex + 4]) << 24) | (UInt32(b[b.startIndex + 5]) << 16) | (UInt32(b[b.startIndex + 6]) << 8) | UInt32(b[b.startIndex + 7])
        }
    }
    
    mutating func put(byte: UInt8) {
        guard available(size: 1) else { return }

        bytes[offset] = byte
        offset += 1
    }

    mutating func put(uInt16 value: UInt16, littleEndian: Bool? = nil) {
        guard available(size: 2) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[offset] = UInt8(value & 0xff)
            bytes[offset + 1] = UInt8(value >> 8)
        }
        else {
            bytes[offset] = UInt8(value >> 8)
            bytes[offset + 1] = UInt8(value & 0xff)
        }
        offset += 2
    }

    mutating func put(uInt24 value: UInt32, littleEndian: Bool? = nil) {
        guard available(size: 3) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[offset] = UInt8(value & 0xff)
            bytes[offset + 1] = UInt8((value >> 8) & 0xff)
            bytes[offset + 2] = UInt8((value >> 16) & 0xff)
        }
        else {
            bytes[offset] = UInt8((value >> 16) & 0xff)
            bytes[offset + 1] = UInt8((value >> 8) & 0xff)
            bytes[offset + 2] = UInt8(value & 0xff)
        }
        offset += 3
    }

    mutating func put(uInt32 value: UInt32, littleEndian: Bool? = nil) {
        guard available(size: 4) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[offset] = UInt8(value & 0xff)
            bytes[offset + 1] = UInt8((value >> 8) & 0xff)
            bytes[offset + 2] = UInt8((value >> 16) & 0xff)
            bytes[offset + 3] = UInt8(value >> 24)
        }
        else {
            bytes[offset ] = UInt8(value >> 24)
            bytes[offset + 1] = UInt8((value >> 16) & 0xff)
            bytes[offset + 2] = UInt8((value >> 8) & 0xff)
            bytes[offset + 3] = UInt8(value & 0xff)
        }
        offset += 4
    }

    mutating func put(uInt64 value: UInt64, littleEndian: Bool? = nil) {
        guard available(size: 8) else { return }
        
        if littleEndian ?? self.littleEndian {
            bytes[offset] = UInt8(value & 0xff)
            bytes[offset + 1] = UInt8((value >> 8) & 0xff)
            bytes[offset + 2] = UInt8((value >> 16) & 0xff)
            bytes[offset + 3] = UInt8((value >> 24) & 0xff)
            bytes[offset + 4] = UInt8((value >> 32) & 0xff)
            bytes[offset + 5] = UInt8((value >> 40) & 0xff)
            bytes[offset + 6] = UInt8((value >> 48) & 0xff)
            bytes[offset + 7] = UInt8(value >> 56)
        }
        else {
            bytes[offset ] = UInt8(value >> 56)
            bytes[offset + 1] = UInt8((value >> 48) & 0xff)
            bytes[offset + 2] = UInt8((value >> 40) & 0xff)
            bytes[offset + 3] = UInt8((value >> 32) & 0xff)
            bytes[offset + 4] = UInt8((value >> 24) & 0xff)
            bytes[offset + 5] = UInt8((value >> 16) & 0xff)
            bytes[offset + 6] = UInt8((value >> 8) & 0xff)
            bytes[offset + 7] = UInt8(value & 0xff)
        }
        offset += 8
    }

    func available(size: Int) -> Bool {
        return offset + size <= bytes.endIndex
    }
    
    mutating func advance(by n: Int) {
        guard offset + n >= bytes.startIndex && offset + n <= bytes.endIndex else { return }
        offset += n
    }
    
    mutating func rewind(by n: Int) {
        advance(by: -n)
    }
    
    mutating func seek(to index: Int, fromEnd: Bool = false) {
        guard index <= bytes.count else { return }
        
        if fromEnd {
            offset = bytes.endIndex - index
        }
        else {
            offset = bytes.startIndex + index
        }
    }
}
