/*
 MediaItem.swift -- Configure MediaView from Images
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
import CoreData

import C64UIComponents
import Emulator

extension DiskImage where Self: MediaItem {
    public var displayIcon: UIImage? {
        return DiskDrive.is5_25(connector: connector) ? UIImage(named: "Floppy 5.25") : UIImage(named: "Floppy 3.5")
    }
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    public var displaySubtitle: String? {
        guard let directory = readDirectory() else { return nil }
        guard let diskTitle = String(bytes: directory.diskNamePETASCII, encoding: .isoLatin1),
            let diskId = String(bytes: directory.diskIdPETASCII, encoding: .isoLatin1) else { return nil }
        return "0\"\(diskTitle)\" \(diskId)"
    }
    public var subtitleIsPETASCII: Bool {
        return true
    }
}

extension DxxImage: MediaItem { }
extension GxxImage: MediaItem { }
extension StubImage: MediaItem { }

extension CartridgeImage: MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    public var displaySubtitle: String? {
        switch format {
        case .crt(_):
            return title

        default:
            if romSize % 1024 == 0 {
                let size = romSize / 1024
                return "\(size) kilobyte"
            }
        }
        return nil
    }
    
    public var subtitleIsPETASCII: Bool {
        return false
    }
    
    public var displayIcon: UIImage? {
        return icon
    }
}

extension TapeImage where Self: MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    public var displaySubtitle: String? {
        return name
    }
    
    public var subtitleIsPETASCII: Bool {
        return false
    }
    
    public var displayIcon: UIImage? {
        return UIImage(named: "Tape")
    }
}

extension RamExpansionUnit: MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent ?? fullName
    }
    
    public var displaySubtitle: String? {
        return url != nil ? fullName : variantName
    }
    
    public var subtitleIsPETASCII: Bool {
        return false
    }
    
    public var displayIcon: UIImage? {
        return icon
    }
}

extension T64Image: MediaItem { }

extension TapImage: MediaItem { }

extension SpectrumTapImage: MediaItem { }

extension SpectrumTZXImage: MediaItem { }

extension ProgramFile: MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    public var displaySubtitle: String? {
        return name
    }
    
    public var subtitleIsPETASCII: Bool {
        return true
    }
    
    public var displayIcon: UIImage? {
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

    public var typeIdentifier: String? {
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
        case .ramlink:
            return Ramlink(url: url)
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
                let destinationUrl = try uniqueName(directory: directoryURL, name: sourceUrl.lastPathComponent, pathExtension: sourceUrl.pathExtension)
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
                            let eepromDestinationUrl = try uniqueName(directory: directoryURL, name: eepromSourceUrl.lastPathComponent, pathExtension: eepromSourceUrl.pathExtension)
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
                    
                case .ramlink:
                    // TODO
                    break
                    
                case .tape:
                    tapeFile = fileName
                }
            }
            catch { }
        }
    }
}

extension IdeDiskImage: MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    public var displaySubtitle: String? {
        if size > 1024 * 1024 * 1024 {
            let value = Int((Double(size) / (1024 * 1024 * 1024)).rounded(.up))
            return "\(value)gb"
        }
        else {
            let value = Int((Double(size) / (1024 * 1024)).rounded(.up))
            return "\(value)mb"
        }
    }
    
    public var subtitleIsPETASCII: Bool {
        return false
    }
    
    public var displayIcon: UIImage? {
        switch ideType {
        case .cd:
            return UIImage(named: "Compact Disc")
            
        case .compactFlash:
            return UIImage(named: "Compact Flash Card")
            
        case .hardDisk:
            return UIImage(named: "IDE Hard Disk")
            
        case .sdCard:
            return UIImage(named: "SD Card")
        }
    }
    
    public var connector: ConnectorType { return .ide }
}


extension Ramlink : MediaItem {
    public var displayTitle: String? {
        return url?.lastPathComponent
    }
    
    public var displaySubtitle: String? {
        return "RAMLink (\(size / (1024 * 1024)) megabyte)"
    }
    
    public var subtitleIsPETASCII: Bool {
        return false
    }
    
    public var displayIcon: UIImage? {
        return icon
    }
    
    var mediaType: C64FileType.MediaType {
        return .cartridge
    }
}

