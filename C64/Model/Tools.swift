/*
 Tools.swift -- Tools Machine Configuration
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
import C64UIComponents
import Emulator

class Tools {
    struct OldSerialized: Codable {
        struct Item: Codable {
            var name: String
        }
        
        var selectedCartridge: String?
        var cartridges: [Item]
        var disks: [Item]
    }
    
    struct Serialized: Codable {
        var cartridges: [String]
        var disks: [String]
        
        init(tools: Tools) {
            cartridges = tools.cartridges.compactMap({ $0.url?.lastPathComponent })
            disks = tools.disks.compactMap({ $0.url?.lastPathComponent })
        }

        init(migrate old: OldSerialized) {
            cartridges = old.cartridges.map({ $0.name }).filter({ $0 != old.selectedCartridge })
            if let cartridge = old.selectedCartridge {
                cartridges.insert(cartridge, at: 0)
            }
            disks = old.disks.map({ $0.name })
        }
    }
    
    var directoryURL: URL
    
    var cartridges = [CartridgeImage]()
    var disks = [DiskImage]()
    
    var selectedCartridge: CartridgeImage? {
        if cartridges.isEmpty {
            return nil
        }
        return cartridges[0]
    }
    
    private static var standardInstance: Tools?
    
    static var standard: Tools {
        if let standard = standardInstance {
            return standard
        }
        else {
            let standard = create()
            standardInstance = standard
            return standard
        }
    }
    
    init() {
        directoryURL = Defaults.toolsURL
    }
    
    static func create() -> Tools {
        let tools = Tools()
        let infoURL = tools.directoryURL.appendingPathComponent("tools.json")
        
        if FileManager.default.fileExists(atPath: infoURL.path) {
            let decoder = JSONDecoder()
            do {
                let data = try Data(contentsOf: infoURL)
                var serialized: Serialized
                do {
                    serialized = try decoder.decode(Tools.Serialized.self, from: data)
                    
                }
                catch {
                    let oldSerialized = try decoder.decode(Tools.OldSerialized.self, from: data)

                    serialized = Serialized(migrate: oldSerialized)
                }
                
                tools.cartridges = serialized.cartridges.compactMap({ CartridgeImage(directory: tools.directoryURL, file: $0) })
                tools.disks = serialized.disks.compactMap({ DxxImage.image(from: tools.directoryURL.appendingPathComponent($0)) })
                
                return tools
            }
            catch {
                return Tools()
            }
        }
        else {
            let tools = Tools()
            if let cartridgeFile = Defaults.standard.toolsCartridgeFile,
                let cartridge = CartridgeImage(directory: tools.directoryURL, file: cartridgeFile) {
                tools.cartridges.append(cartridge)
            }
            return tools
        }
    }
    
    func save() {
        let infoURL = directoryURL.appendingPathComponent("tools.json")

        let encoder = JSONEncoder()
        
        do {
            let serialized = Serialized(tools: self)
            let data = try encoder.encode(serialized)
            try data.write(to: infoURL)
        }
        catch { }
    }
}

extension Tools: GameViewItem {
    var type: GameViewItemType {
        return .tools
    }
    
    var title: String {
        return "Tools"
    }
    
    var media: GameViewItemMedia {
        return GameViewItemMedia(sections: [
            GameViewItemMedia.Section(type: .cartridge, supportsMultiple: true, supportsReorder: true, addInFront: false, items: cartridges),
            GameViewItemMedia.Section(type: .disks, supportsMultiple: true, supportsReorder: true, addInFront: false, items: disks as! [MediaItem])
        ])
    }
    
    var machine: MachineOld {
        let machine = MachineOld(specification: Defaults.standard.machineSpecification.appending(layer: MachineConfigOld()))

        if !cartridges.isEmpty {
            machine.cartridgeImage = cartridges[0]
        }
        machine.diskImages = disks
        
        return machine
    }
    
    func addMedia(mediaItem: MediaItem, at row: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge:
            guard let cartridge = mediaItem as? CartridgeImage else { return }
            cartridges.insert(cartridge, at: row)
            
        case .disks:
            guard let disk = mediaItem as? DiskImage else { return }
            disks.insert(disk, at: row)

        default:
            return
        }
    }
    
    func moveMedia(from sourceRow: Int, to destinationRow: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge:
            let cartridge = cartridges.remove(at: sourceRow)
            cartridges.insert(cartridge, at: destinationRow)
            
        case .disks:
            let disk = disks.remove(at: sourceRow)
            disks.insert(disk, at: destinationRow)

        default:
            return
        }
    }
    
    func removeMedia(at row: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge:
            cartridges.remove(at: row)
            
        case .disks:
            disks.remove(at: row)
            
        default:
            return
        }
    }
    
    func renameMedia(name: String, at: Int, sectionType: GameViewItemMedia.SectionType) {
        // TODO
        return
    }
    
    func replaceMedia(mediaItem: MediaItem, sectionType: GameViewItemMedia.SectionType) {
        return
    }
}
