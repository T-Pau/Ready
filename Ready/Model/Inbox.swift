/*
 Inbox.swift
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

class Inbox {
    typealias ChangeHandler = (_ inbox: Inbox, _ updates: TableViewUpdates) -> Void
    
    var directoryURL: URL
    var changeHandler: ChangeHandler?
    
    struct Item {
        var media: MediaItem
        var date: Date
    }
    var items = [Item]()
    
    private var directoryWatcher: DirectoryWatcher?
    
    init() {
        directoryURL = Defaults.inboxURL
        items = getFiles()
        
        directoryWatcher = DirectoryWatcher(url: directoryURL, changeHandler: handleChanges(_:))
    }
    
    func handleChanges(_ directoryWatcher: DirectoryWatcher) {
        let oldItems = items
        items = getFiles()
        
        guard let changeHandler = changeHandler else { return }

        var updates = TableViewUpdates()
        
        switch (oldItems.isEmpty, items.isEmpty) {
        case (true, true):
            break

        case (true, false):
            updates.insert(section: 0)
            
        case (false, true):
            updates.delete(section: 0)
            
        case (false, false):
            var oldMap = [URL: Int]()
            for (index, item) in oldItems.enumerated() {
                guard let url = item.media.url else { continue }
                oldMap[url] = index
            }
            
            for (index, item) in items.enumerated() {
                guard let url = item.media.url else { continue }
                
                if let oldIndex = oldMap[url] {
                    if oldIndex != index {
                        updates.move(row: IndexPath(row: oldIndex, section: 0), to: IndexPath(row: index, section: 0))
                    }
                    else if let oldCartidge = oldItems[oldIndex].media as? CartridgeImage, let newCartridge = items[index].media as? CartridgeImage {
                        if oldCartidge.eepromUrl != newCartridge.eepromUrl {
                            updates.reload(row: IndexPath(row: index, section: 0))
                        }
                    }
                    oldMap.removeValue(forKey: url)
                }
                else {
                    updates.insert(row: IndexPath(row: index, section: 0))
                }
            }
            
            for index in oldMap.values {
                updates.delete(row: IndexPath(row: index, section: 0))
            }
        }
        
        if !updates.isEmpty {
            changeHandler(self, updates)
        }
    }
    
    struct Gmod2Part {
        var index: Int
        var isFlash : Bool
    }

    private func getFiles() -> [Item] {
        var newItems = [Item]()
        
        do {
            try ensureDirectory(directoryURL)
            
            var gmod2parts = [String: Gmod2Part]()
            
            let files = try FileManager.default.contentsOfDirectory(at: directoryURL, includingPropertiesForKeys: [.contentModificationDateKey], options: [.skipsPackageDescendants, .skipsHiddenFiles])
            
            for file in files {
                if let gmod2 = Gmod2(suggestedName: file.lastPathComponent) {
                    if let part = gmod2parts[gmod2.name] {
                        let flashName: String
                        let eepromName: String
                        
                        if part.isFlash != gmod2.isFlash {
                            if part.isFlash {
                                guard let name = newItems[part.index].media.url?.lastPathComponent else { break }
                                flashName = name
                                eepromName = file.lastPathComponent
                            }
                            else {
                                guard let name = newItems[part.index].media.url?.lastPathComponent else { break }
                                flashName = file.lastPathComponent
                                eepromName = name
                            }
                            if let mediaItem = CartridgeImage(directory: directoryURL, file: flashName, eeprom: eepromName) {
                                newItems[part.index].media = mediaItem
                                continue
                            }
                        }
                    }
                    else {
                        gmod2parts[gmod2.name] = Gmod2Part(index: newItems.count, isFlash: gmod2.isFlash)
                    }
                }

                guard let mediaItem = CartridgeImage.loadMediaItem(from: file) else { continue }
                let date = try file.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate ?? Date()
                newItems.append(Item(media: mediaItem, date: date))
            }
        }
        catch {
            return []
        }
        
        return newItems.sorted(by: { $0.date > $1.date })
    }
}

extension Inbox: GameViewItem {
    var type: GameViewItemType {
        return .inbox
    }
    
    var title: String {
        return "Inbox"
    }
    
    var media: GameViewItemMedia {
        return GameViewItemMedia(sections: [
            GameViewItemMedia.Section(type: .mixed, supportsMultiple: true, supportsReorder: false, addInFront: true, items: items.map({ $0.media }))
        ])
    }
    
    var machine: MachineOld {
        let machine = MachineOld(specification: Defaults.standard.machineSpecification.appending(layer: MachineConfigOld()))

        machine.mediaItems = items.map({ $0.media })
        
        return machine
    }
    
    func addMedia(mediaItem: MediaItem, at index: Int, sectionType: GameViewItemMedia.SectionType) {
        // file already created, will be picked up by directory watcher
    }
    
    func moveMedia(from sourceRow: Int, to destinationRow: Int, sectionType: GameViewItemMedia.SectionType) {
    }
    
    func removeMedia(at index: Int, sectionType: GameViewItemMedia.SectionType) {
        if let url = items[index].media.url {
            do {
                try FileManager.default.removeItem(at: url)

            }
            catch { }
        }
        // removal will be picked up by directory watcher
    }
    
    func renameMedia(name: String, at: Int, sectionType: GameViewItemMedia.SectionType) {
        // TODO
        return
    }
    
    func replaceMedia(mediaItem: MediaItem, sectionType: GameViewItemMedia.SectionType) {
        return
    }
    
    func save() throws {
        return
    }
    
    
}
