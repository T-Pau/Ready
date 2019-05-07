/*
 MediaItem.swift -- Configure MediaView from Images
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
import CoreData

import C64UIComponents

extension DiskImage where Self: MediaItem {
    var displayIcon: UIImage? {
        return mediaType.is5_25Inch ? UIImage(named: "Floppy 5.25") : UIImage(named: "Floppy 3.5")
    }
    var displayTitle: String? {
        return url?.lastPathComponent
    }
    var displaySubtitle: String? {
        guard let directory = readDirectory() else { return nil }
        guard let diskTitle = String(bytes: directory.diskNamePETASCII, encoding: .isoLatin1),
            let diskId = String(bytes: directory.diskIdPETASCII, encoding: .isoLatin1) else { return nil }
        return "0\"\(diskTitle)\" \(diskId)"
    }
    var subtitleIsPETASCII: Bool {
        return true
    }
}

extension DxxImage: MediaItem { }
extension GxxImage: MediaItem { }
extension StubImage: MediaItem { }

extension CartridgeImage: MediaItem {
    var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    var displaySubtitle: String? {
        if isCrt {
            return title
        }
        else {
            if bytes.count % 1024 == 0 {
                let size = bytes.count / 1024
                return "\(size) kilobyte"
            }
        }
        return nil
    }
    
    var subtitleIsPETASCII: Bool {
        return false
    }
    
    var displayIcon: UIImage? {
        return icon
    }
}

extension TapeImage where Self: MediaItem {
    var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    var displaySubtitle: String? {
        return name
    }
    
    var subtitleIsPETASCII: Bool {
        return false
    }
    
    var displayIcon: UIImage? {
        return UIImage(named: "Tape")
    }
}

extension RamExpansionUnit: MediaItem {
    var displayTitle: String? {
        return url?.lastPathComponent ?? fullName
    }
    
    var displaySubtitle: String? {
        return url != nil ? fullName : variantName
    }
    
    var subtitleIsPETASCII: Bool {
        return false
    }
    
    var displayIcon: UIImage? {
        return icon
    }
}

extension T64Image: MediaItem { }

extension TapImage: MediaItem { }

extension ProgramFile: MediaItem {
    var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    var displaySubtitle: String? {
        return nil
    }
    
    var subtitleIsPETASCII: Bool {
        return false
    }
    
    var displayIcon: UIImage? {
        return UIImage(named: "File")
    }
}

extension MediaItem {
    var mediaType: C64FileType.MediaType {
        if self as? DiskImage != nil {
            return .disk
        }
        else if self as? CartridgeImage != nil {
            return .cartridge
        }
        else if self as? TapeImage != nil {
            return .tape
        }
        else {
            return .programFile
        }
    }

    var typeIdentifier: String? {
        guard let pathExtension = url?.pathExtension else { return nil }
        return C64FileType.init(pathExtension: pathExtension)?.typeIdentifier
    }
    
    static func loadMediaItem(from url: URL) -> MediaItem? {
        guard let fileType = C64FileType(pathExtension: url.pathExtension) else { return nil }

        switch fileType.type {
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
        case .tape:
            return TapImage.image(from: url) as? MediaItem
        }
    }
}

extension Game {
    convenience init?(name: String, insertInto context: NSManagedObjectContext, mediaItems: [MediaItem], move: Bool) {
        self.init(name: name, insertInto: context)
        
        var hadTypes = Set<C64FileType.MediaType>()
        
        let fileManager = FileManager.default
        
        for item in mediaItems {
            let type = item.mediaType
            
            if type != .disk && hadTypes.contains(type) {
                continue
            }
            hadTypes.insert(type)
            
            guard let sourceUrl = item.url else { continue }
            do {
                let destinationUrl = try uniqeName(directory: directoryURL, name: sourceUrl.lastPathComponent, pathExtension: sourceUrl.pathExtension)
                let fileName = destinationUrl.lastPathComponent
                if move {
                    try fileManager.moveItem(at: sourceUrl, to: destinationUrl)
                }
                else {
                    try fileManager.copyItem(at: sourceUrl, to: destinationUrl)
                }
                
                
                switch type {
                case .cartridge:
                    cartridgeFile = fileName
                    if let cartridge = item as? CartridgeImage, let eepromSourceUrl = cartridge.eepromUrl {
                        do {
                            let eepromDestinationUrl = try uniqeName(directory: directoryURL, name: eepromSourceUrl.lastPathComponent, pathExtension: eepromSourceUrl.pathExtension)
                            if move {
                                try fileManager.moveItem(at: eepromSourceUrl, to: eepromDestinationUrl)
                            }
                            else {
                                try fileManager.copyItem(at: eepromSourceUrl, to: eepromDestinationUrl)
                            }
                            cartridgeEEPROM = eepromDestinationUrl.lastPathComponent
                        }
                        catch {
                            cartridgeFile = nil
                            try fileManager.removeItem(at: destinationUrl)
                        }
                    }

                case .disk:
                    addToDisks(Disk(fileName: fileName, insertInto: context))
                    
                case .ideDisk:
                    addToIdeDisks(Disk(fileName: fileName, insertInto: context))
                    break
                    
                case .programFile:
                    programFile = fileName
                    
                case .ramExpansionUnit:
                    ramExpansionFile = fileName
                    
                case .tape:
                    tapeFile = fileName
                }
            }
            catch { }
        }
    }
}
