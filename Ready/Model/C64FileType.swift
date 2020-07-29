/*
 C64FileType.swift -- C64 File Type Information
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

struct C64FileType {
    enum MediaType: CaseIterable {
        case cartridge
        case disk
        case ideDisk
        case programFile
        case ramExpansionUnit
        case tape
        
        var typeIdentifiers: Set<String> {
            switch self {
            case .cartridge:
                return ["com.apple.macbinary-archive", "public.x509-certificate"]
            case .disk:
                return ["at.spiderlab.c64.d1m", "at.spiderlab.c64.d2m", "at.spiderlab.c64.d4m", "at.spiderlab.c64.d64", "at.spiderlab.c64.d81", "at.spiderlab.c64.g64", "org.sidmusic.d64"]
            case .ideDisk:
                return ["at.spiderlab.c64.cfa", "at.spiderlab.c64.hdd", "at.spiderlab.c64.iso", "at.spiderlab.c64.sdcard"]
            case .ramExpansionUnit:
                return ["at.spiderlab.c64.reu"]
            case .programFile:
                return ["at.spiderlab.c64.p00", "at.spiderlab.c64.prg", "org.sidmusic.prg"]
            case .tape:
                return ["at.spiderlab.c64.t64", "at.spiderlab.c64.tap", "at.spiderlab.c64.tzx", "org.sidmusic.tap"]
            }
        }
    }
    
    var type: MediaType
    var pathExtension: String
    var typeIdentifier: String
    
    static let knownExtensions: [String : MediaType ] = [
        "bin": .cartridge,
        "cfa": .ideDisk,
        "crt": .cartridge,
        "d1m": .disk,
        "d2m": .disk,
        "d4m": .disk,
        "d64": .disk,
        "d71": .disk,
        "d80": .disk,
        "d81": .disk,
        "d82": .disk,
        "g64": .disk,
        "g71": .disk,
        "hdd": .ideDisk,
        "iso": .ideDisk,
        "p00": .programFile,
        "p01": .programFile,
        "p02": .programFile,
        "p03": .programFile,
        "prg": .programFile,
        "reu": .ramExpansionUnit,
        "sdcard": .ideDisk,
        "t64": .tape,
        "tap": .tape,
        "tzx": .tape
    ]
    
    private static var _typeIdentifiers = Set<String>()
    
    static var typeIdentifiers: Set<String> {
        if _typeIdentifiers.isEmpty {
            for type in MediaType.allCases {
                _typeIdentifiers = _typeIdentifiers.union(type.typeIdentifiers)
            }
        }
        return _typeIdentifiers
    }
    
    static func type(for typeIdentifiers: [String]) -> C64FileType? {
        for typeIdentifier in typeIdentifiers {
            if let type = C64FileType(typeIdentifier: typeIdentifier) {
                return type
            }
        }
        
        return nil
    }

    init?(pathExtension: String) {
        self.pathExtension = pathExtension.lowercased()
        typeIdentifier = "at.spiderlab.c64." + self.pathExtension
        
        if let type = C64FileType.knownExtensions[self.pathExtension] {
            self.type = type
            
            if pathExtension == "bin" {
                typeIdentifier = "com.apple.macbinary-archive"
            }
            else if pathExtension == "crt" {
                typeIdentifier = "public.x509-certificate"
            }
            else if pathExtension == "p01" || pathExtension == "p02" || pathExtension == "p03" {
                typeIdentifier = "at.spiderlab.c64.p00"
            }
        }
        else {
            return nil
        }
    }
    
    init?(typeIdentifier: String) {
        self.typeIdentifier = typeIdentifier
        if typeIdentifier.hasPrefix("at.spiderlab.c64.") {
            pathExtension = String(typeIdentifier.dropFirst(17))
            
            if let type = C64FileType.knownExtensions[pathExtension] {
                self.type = type
            }
            else {
                return nil
            }
        }
        else if typeIdentifier.hasPrefix("org.sidmusic.") {
            pathExtension = String(typeIdentifier.dropFirst(13))

            if let type = C64FileType.knownExtensions[pathExtension] {
                self.type = type
                self.typeIdentifier = "at.spiderlab.c64.\(pathExtension)"
            }
            else {
                return nil
            }
        }
        else if typeIdentifier == "com.apple.macbinary-archive" {
            pathExtension = "bin"
            type = .cartridge
        }
        else if typeIdentifier == "public.x509-certificate" {
            pathExtension = "crt"
            type = .cartridge
        }
        else {
            return nil
        }
    }
    
}
