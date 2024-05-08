/*
 MediaList.swift -- Serialized list of media items.
 Copyright (C) 2021 Dieter Baron
 
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
import Emulator

struct MediaList: Codable {
    struct Item : Codable {
        var fileName: String
        
        init(fileName: String) {
            self.fileName = fileName
        }
    }
    
    private enum CodingKeys: String, CodingKey {
        case items
    }
    
    var directory = URL(fileURLWithPath: ".")
    var items = [Item]()
    
    private struct LegacyTools : Codable {
        var cartridges: [String]
        var disks: [String]
    }
    
    static let listFileName = "Media List.json"
    
    init(directory: URL) {
        do {
            do {
                let data = try Data(contentsOf: directory.appendingPathComponent(MediaList.listFileName))
                self = try JSONDecoder().decode(MediaList.self, from: data)
            }
            catch {
                let data = try Data(contentsOf: directory.appendingPathComponent("tools.json"))
                let tools = try JSONDecoder().decode(LegacyTools.self, from: data)
                items.append(contentsOf: tools.cartridges.map({ Item(fileName: $0) }))
                items.append(contentsOf: tools.disks.map({ Item(fileName: $0) }))
            }
        }
        catch {
            print("can't read media list from file in '\(directory)'")
            // TODO: list directory as fallback
        }
        self.directory = directory
    }
    
    init(mediaItems: [MediaItem]) {
        items = mediaItems.compactMap {
            if let name=$0.url?.lastPathComponent {
                return Item(fileName: name)
            }
            else {
                return nil
            }
        }
    }
    
    var mediaItems: [MediaItem] {
        print("DEBUG: getting \(items.count) media items in '\(directory)'")
        return items.compactMap { item in
            let url = directory.appendingPathComponent(item.fileName)
            print("DEBUG: getting item '\(url)'")
            guard let type = C64FileType.type(for: URL(fileURLWithPath: item.fileName).pathExtension) else { return nil }
            switch type {
            case .cartridge:
                return CartridgeImage(url: url)
            case .disk:
                return DxxImage.image(from: url) as? MediaItem
            case .ideDisk:
                return IdeDiskImage(url: url)
            case .programFile:
                return ProgramFile(url: url)
            case .ramExpansionUnit:
                return RamExpansionUnit(url: url)
            case .ramlink:
                return Ramlink(url: url)
            case .tape:
                return T64Image.image(from: url) as? MediaItem
            }
        }
    }
    
    func save(directory: URL) throws {
        let data = try JSONEncoder().encode(self)
        try data.write(to: directory.appendingPathComponent(MediaList.listFileName), options: .atomic)
    }
}

extension MediaList {
    init(game: Game) {
        if FileManager.default.fileExists(atPath: game.directoryURL.appendingPathComponent(MediaList.listFileName).path) {
            print("DEBUG: loading game '\(game.name)' media list from file")
            self = MediaList(directory: game.directoryURL)
            return
        }
        
        directory = game.directoryURL
        print("DEBUG: migrating game '\(game.name)' media list to file")

        var trash = [String]()
        
        if let cartridgeFile = game.cartridgeFile {
            if let eepromFile = game.cartridgeEEPROM {
                do {
                    items.append(Item(fileName: try convertGmod2(flashFile: cartridgeFile, eepromFile: eepromFile, directory: game.directoryURL)))
                    trash.append(cartridgeFile)
                    trash.append(eepromFile)
                }
                catch {
                    // TODO: how to handle conversion error?
                }
            }
            else {
                items.append(Item(fileName: cartridgeFile))
            }
        }

        if let ramExpansionFile = game.ramExpansionFile {
            items.append(Item(fileName: ramExpansionFile))
        }
        
        if let programFile = game.programFile {
            items.append(Item(fileName: programFile))
        }

        if let disks = game.disks?.array as? [Disk] {
            for disk in disks {
                items.append(Item(fileName: disk.fileName))
            }
        }
        
        if let disks = game.ideDisks?.array as? [Disk] {
            for disk in disks {
                items.append(Item(fileName: disk.fileName))
            }
        }

        if let tapeFile = game.tapeFile {
            items.append(Item(fileName: tapeFile))
        }

        do {
            try save(directory: game.directoryURL)
            game.cartridgeEEPROM = nil
            game.cartridgeFile = nil
            game.ramExpansionFile = nil
            game.programFile = nil
            game.disks = nil
            game.ideDisks = nil
            game.tapeFile = nil
            try game.save()
            for file in trash {
                try? FileManager.default.removeItem(at: game.directoryURL.appendingPathComponent(file))
            }
        }
        catch { }
    }
    
    // TODO: move elsewhere
    
    func convertGmod2(flashFile: String, eepromFile: String, directory: URL) throws -> String {
        let crtName = flashFile.replacingOccurrences(of: "(-flash)?.bin", with: "", options: [.regularExpression], range: nil).appending(".crt")
        let flash = try Data(contentsOf: directory.appendingPathComponent(flashFile))
        let eeprom = try Data(contentsOf: directory.appendingPathComponent(eepromFile))
        
        let crt = Gmod2Crt(flash: flash, eeprom: eeprom)

        try crt.crt.write(to: directory.appendingPathComponent(crtName))
        
        return crtName
    }
}

// TODO: move elsewhere

struct Gmod2Crt {
    var crt = Data()
    
    init(flash: Data, eeprom: Data) {
        crt.append(Data("C64 CARTRIDGE   ".utf8))
        crt.append(contentsOf: encode32(value: 0x40)) // header length
        crt.append(contentsOf: [1, 0])
        crt.append(contentsOf: encode16(value: 60))
        crt.append(contentsOf: [1, 0])
        crt.append(contentsOf: [0, 0, 0, 0, 0, 0])
        crt.append(Data(String(repeating: "\0", count: 32).utf8))
    
        for bank in 0 ..< 64 {
            crt.append(make_chip(type: 2, bank: bank, loadAddress: 0x8000, data: flash.subdata(in: (bank * 0x2000) ..< ((bank + 1) * 0x2000))))
        }
    
        crt.append(make_chip(type: 2, bank: 0, loadAddress: 0xde00, data: eeprom))
    }

    func encode16(value: Int) -> [UInt8] {
        return [
            UInt8((value >> 8) & 0xff),
            UInt8(value & 0xff)
        ]
    }
    
    func encode32(value: Int) -> [UInt8] {
        return [
            UInt8(value >> 24),
            UInt8((value >> 16) & 0xff),
            UInt8((value >> 8) & 0xff),
            UInt8(value & 0xff)
        ]
    }
    
    func make_chip(type: Int, bank: Int, loadAddress: Int, data: Data) -> Data {
        var chip = Data()
        
        chip.append(Data("CHIP".utf8))
        chip.append(contentsOf: encode32(value: data.count + 0x10))
        chip.append(contentsOf: encode16(value: type))
        chip.append(contentsOf: encode16(value: bank))
        chip.append(contentsOf: encode16(value: loadAddress))
        chip.append(contentsOf: encode16(value: data.count))
        chip.append(data)
        
        return chip
    }
}
