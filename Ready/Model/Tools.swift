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
    var directoryURL: URL
    var mediaItems = [MediaItem]()
    
    var selectedCartridge: CartridgeImage? {
        return mediaItems.first(where: { $0.connector == .c64ExpansionPort }) as? CartridgeImage
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
        let mediaList = MediaList(directory: tools.directoryURL)
        
        tools.mediaItems = mediaList.mediaItems
        
        return tools
    }
    
    func save() {
        try? MediaList(mediaItems: mediaItems).save(directory: directoryURL)
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
            GameViewItemMedia.Section(type: .mixed, supportsMultiple: true, supportsReorder: true, addInFront: false, items: mediaItems)
        ])
    }
    
    var machine: MachineOld {
        let machine = MachineOld(specification: Defaults.standard.machineSpecification.appending(layer: MachineConfigOld()))

        machine.mediaItems = mediaItems
        
        return machine
    }
    
    func addMedia(mediaItem: MediaItem, at row: Int, sectionType: GameViewItemMedia.SectionType) {
        mediaItems.insert(mediaItem, at: row)
    }
    
    func moveMedia(from sourceRow: Int, to destinationRow: Int, sectionType: GameViewItemMedia.SectionType) {
        let item = mediaItems.remove(at: sourceRow)
        mediaItems.insert(item, at: destinationRow)
    }
    
    func removeMedia(at row: Int, sectionType: GameViewItemMedia.SectionType) {
        mediaItems.remove(at: row)
    }
    
    func renameMedia(name: String, at: Int, sectionType: GameViewItemMedia.SectionType) {
        // TODO
        return
    }
    
    func replaceMedia(mediaItem: MediaItem, sectionType: GameViewItemMedia.SectionType) {
        return
    }
}
