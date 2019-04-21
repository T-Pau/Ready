/*
 C64FileType.swift -- C64 File Type Information
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

struct C64FileType {
    enum MediaType: CaseIterable {
        case cartridge
        case disk
        case programFile
        case tape
        
        var typeIdentifiers: Set<String> {
            switch self {
            case .cartridge:
                return [ "com.apple.macbinary-archive", "public.x509-certificate" ]
            case .disk:
                return [ "at.spiderlab.c64.d1m", "at.spiderlab.c64.d2m", "at.spiderlab.c64.d4m", "at.spiderlab.c64.d64", "at.spiderlab.c64.d81", "at.spiderlab.c64.g64" ]
            case .programFile:
                return [ "at.spiderlab.c64.prg" ]
            case .tape:
                return [ "at.spiderlab.c64.t64", "at.spiderlab.c64.tap" ]
            }
        }
    }
    
    var type: MediaType
    var pathExtension: String
    var typeIdentifier: String
    
    static let knownExtensions: [String : MediaType ] = [
        "bin": .cartridge,
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
        "p00": .programFile,
        "prg": .programFile,
        "t64": .tape,
        "tap": .tape
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
            else if pathExtension == "p00" {
                typeIdentifier = "at.spiderlab.c64.prg"
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
