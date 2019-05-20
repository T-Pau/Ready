/*
 DiskImage.swift -- Access disk images
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

protocol DiskImage {
    static func image(from url: URL) -> DiskImage?
    static func image(from bytes: Data) -> DiskImage?

    var directoryTrack: UInt8 { get }
    var directorySector: UInt8 { get }
    var mediaType: DiskDrive.MediaType { get }
    var tracks: Int { get }
    var url: URL? { get set }

    var diskId: [UInt8]? { get }
    
    func save() throws

    func getBlock(track: UInt8, sector: UInt8) -> Data?
    
    func readDirectory() -> Directory?
    func readFreeBlocks() -> Int?
    func readFile(track: UInt8, sector: UInt8) -> Data?
    
    mutating func writeBLock(track: UInt8, sector: UInt8, data: Data)
}

extension DiskImage {
    static func image(from url: URL) -> DiskImage? {
        guard let bytes = FileManager.default.contents(atPath: url.path) else { return nil }
        guard var image = image(from: bytes) else { return StubImage(url: url) }
        image.url = url
        return image
    }

    static func image(from bytes: Data) -> DiskImage? {
        if let image = GxxImage(bytes: bytes) {
            return image
        }
        if let image = DxxImage(bytes: bytes) {
            return image
        }
        return nil
    }
    
    static func blankImage(url: URL, mediaType: DiskDrive.MediaType, namePETSCII: [UInt8], idPETSCII: [UInt8]) -> DiskImage? {
        var optinalImage: DiskImage?
        
        optinalImage = DxxImage(blank: mediaType, namePETSCII: namePETSCII, idPETSCII: idPETSCII)
        
        guard var image = optinalImage else { return nil }
        image.url = url
        do {
            try image.save()
        }
        catch {
            return nil
        }
        return image
    }

    func readDirectory() -> Directory? {
        var seenSectors = Set<(UInt16)>()
        var entries = [Directory.Entry]()
        
        var track = directoryTrack
        var sector = directorySector
        
        while (track != 0) {
            let id = UInt16(track) << 8 | UInt16(sector)
            if (seenSectors.contains(id)) {
                // loop detected
                break
            }
            seenSectors.insert(id)

            guard let bytes = getBlock(track: track, sector: sector) else { break }
            
            var offset = 0;
            while (offset < 0x100) {
                if bytes[offset+2] == 0 {
                    offset += 0x20
                    continue
                }
                entries.append(Directory.Entry(bytes: bytes.subdata(in: offset..<offset+0x20)))
                offset += 0x20
            }
            
            track = bytes[0]
            sector = bytes[1]
        }
        
        guard let bytes = getBlock(track: directoryTrack, sector: 0) else { return nil }
        
        guard let freeBlocks = readFreeBlocks() else { return nil }

        let name: [UInt8]
        let diskId: [UInt8]
        
        switch mediaType {
        case .doubleDensity3_5:
            name = [UInt8](bytes[0x04 ..< 0x14])
            diskId = [UInt8](bytes[0x16 ..< 0x1b])
            
        case .singleDensitySingleSided5_25:
            name = [UInt8](bytes[0x90..<0xa0])
            diskId = [UInt8](bytes[0xa2..<0xa7])

        default:
            return nil
        }

        let signature = String(bytes: bytes[0xad...0xbc], encoding: .ascii)
        let isGEOS = signature?.hasPrefix("GEOS format ") ?? false

        return Directory(diskNamePETASCII: name, diskIdPETASCII: diskId, freeBlocks: freeBlocks, entries: entries, isGEOS: isGEOS)
    }
    
    func readFile(track startTrack: UInt8, sector startSector: UInt8) -> Data? {
        var seenSectors = Set<(UInt16)>()
        
        var data = [UInt8]()
        
        var track = startTrack
        var sector = startSector
        
        while (track != 0) {
            let id = UInt16(track) << 8 | UInt16(sector)
            if (seenSectors.contains(id)) {
                // loop detected
                return nil
            }
            seenSectors.insert(id)
            
            guard let block = getBlock(track: track, sector: sector) else { return nil }
            
            track = block[0]
            sector = block[1]
            
            let length = Int(track == 0 ? sector : 255)
            
            data.append(contentsOf: block[2...length])
        }

        return Data(data)
    }
    
    func readFreeBlocks() -> Int? {
        switch mediaType {
        case .singleDensitySingleSided5_25:
            var freeBlocks = 0
        
            guard let bytes = getBlock(track: directoryTrack, sector: 0) else { return nil }

            for track in (1 ... 35) {
                if track == directoryTrack {
                    continue
                }
                freeBlocks += Int(bytes[track * 4])
            }
            
            // TODO: 40 tracks BAM (two variants)
            
            return freeBlocks
            
        case .doubleDensity3_5:
            var freeBlocks = 0
            
            for sector in (UInt8(1) ... UInt8(2)) {
                guard let bytes = getBlock(track: directoryTrack, sector: sector) else { return nil }
                for track in (1 ... 40) {
                    if track == directoryTrack && sector == 1 {
                        continue
                    }
                    
                    freeBlocks += Int(bytes[0x0a + track * 6])
                }
            }
            
            return freeBlocks
            
        default:
            return nil
        }
    }
    
    var diskId: [UInt8]? {
        guard let bytes = getBlock(track: directoryTrack, sector: 0) else { return nil }
        switch mediaType {
        case .singleDensitySingleSided5_25:
            return Array(bytes[0xa2 ... 0xa3])
            
        case .doubleDensity3_5:
            return Array(bytes[0x16 ... 0x17])
            
        default:
            return nil
        }
    }
}

struct Directory {
    struct Entry {
        var fileType: UInt8
        // SAVE-@ replacement
        var locked: Bool
        var closed: Bool
        var track: UInt8
        var sector: UInt8
        var namePETASCII: [UInt8]
        var postNamePETASCII: [UInt8]
        var blocks: UInt16
        // REL files
        var recordLength: UInt8
        var sideBlockTrack: UInt8
        var sideBlockSector: UInt8
        // GEOS stuff
        
        init(bytes: Data) {
            fileType = bytes[2] & 0xf
            // SAVE-@ replacment: bytes[0] & 0x20
            locked = (bytes[2] & 0x40) != 0
            closed = (bytes[2] & 0x80) != 0
            track = bytes[3]
            sector = bytes[4]
            let components = [UInt8](bytes[0x05 ... 0x14]).split(separator: 0xa0, maxSplits: 1, omittingEmptySubsequences: false)
            namePETASCII = [UInt8](components[0])
            if components.count > 1 {
                postNamePETASCII = [UInt8](components[1])
                postNamePETASCII.append(0xa0)
            }
            else {
                postNamePETASCII = [UInt8]()
            }
            sideBlockTrack = bytes[0x15]
            sideBlockSector = bytes[0x16]
            recordLength = bytes[0x17]
            // GEOS: 0x18-0x1d
            blocks = UInt16(bytes[0x1e]) | UInt16(bytes[0x1f]) << 8
        }
        
        var name: String { return String(bytes: namePETASCII, encoding: .ascii) ?? "XXX" } // TODO: filter non-ASCII characters
    }
    var diskNamePETASCII: [UInt8]
    var diskIdPETASCII: [UInt8]
    var freeBlocks: Int
    var entries: [Entry]
    var isGEOS: Bool
}

struct DxxImage : DiskImage {
    private struct DiskLayout {
        var trackOffsets: [Int]
        var totalSectors: Int
        var directoryTrack: UInt8
        var directorySector: UInt8
        var hasErrorMap: Bool
        var mediaType: DiskDrive.MediaType
        
        var fileSize: Int {
            return totalSectors * (hasErrorMap ? 257 : 256)
        }
        
        init(template: DiskLayoutTemplate, tracks: Int, hasErrorMap: Bool) {
            trackOffsets = [ 0 ] // track 0 doesn't exist

            var zoneIterator = template.zones.makeIterator()
            var offset = 0
            var size = 0
            
            var nextZone = zoneIterator.next()
            for track in (1 ... tracks) {
                trackOffsets.append(offset)
                if let zone = nextZone, track >= zone.firstTrack {
                    size = Int(zone.sectors)
                    nextZone = zoneIterator.next()
                }
                offset += size
            }
            totalSectors = offset
            
            self.directoryTrack = template.directoryTrack
            self.directorySector = template.directorySector
            self.mediaType = template.mediaType
            self.hasErrorMap = hasErrorMap
        }
        
        func getOffset(track: UInt8) -> Int {
            return trackOffsets[Int(track)]
        }
        
        func getSectors(track: UInt8) -> UInt8 {
            let track = Int(track)
            if track < trackOffsets.count - 1 {
                return UInt8(trackOffsets[track + 1] - trackOffsets[track])
            }
            else {
                return UInt8(totalSectors - trackOffsets[track])
            }
        }
    }

    var bytes: Data
    var url: URL?
    
    private var layout: DiskLayout
    
    static var bam1541 = Data([
        0x12, 0x01, 0x41, 0x00, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
        0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
        0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
        0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f,
        0x15, 0xff, 0xff, 0x1f, 0x15, 0xff, 0xff, 0x1f, 0x11, 0xfc, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07,
        0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07, 0x13, 0xff, 0xff, 0x07,
        0x13, 0xff, 0xff, 0x07, 0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03,
        0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03, 0x12, 0xff, 0xff, 0x03, 0x11, 0xff, 0xff, 0x01,
        0x11, 0xff, 0xff, 0x01, 0x11, 0xff, 0xff, 0x01, 0x11, 0xff, 0xff, 0x01, 0x11, 0xff, 0xff, 0x01,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x32, 0x41, 0xa0, 0xa0, 0xa0, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    ])
    
    static var bam1581 = Data([
        0x28, 0x03, 0x44, 0x00, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x20, 0xa0, 0x33, 0x44, 0xa0, 0xa0, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x28, 0x02, 0x44, 0xbb, 0x42, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x24, 0xf0, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0x44, 0xbb, 0x42, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff,
    ])
    
    init?(blank mediaType: DiskDrive.MediaType, namePETSCII: [UInt8], idPETSCII: [UInt8]) {
        DxxImage.initLayouts()
        
        guard let layout = DxxImage.layoutByMeidaType[mediaType] else { return nil }
        self.layout = layout
        bytes = Data(count: layout.fileSize)

        var bam: Data
        let nameOffset: Int

        switch mediaType {
        case .singleDensitySingleSided5_25:
            bam = DxxImage.bam1541
            nameOffset = 0x90
            
        case .doubleDensity3_5:
            bam = DxxImage.bam1581
            nameOffset = 0x04
            
        default:
            return nil
        }
        let name = namePETSCII.count <= 16 ? ArraySlice<UInt8>(namePETSCII) : namePETSCII[0..<16]
        var id: [UInt8]
        switch idPETSCII.count {
        case 0, 1, 2, 5:
            id = idPETSCII
        case 3:
            id = idPETSCII
            id.append(0xa0)
            id.append(0xa0)
        case 4:
            id = idPETSCII
            id.append(0xa0)
        default:
            id = Array(idPETSCII[0..<5])
        }
        bam.replaceSubrange(nameOffset ..< nameOffset + name.count, with: name)
        bam.replaceSubrange(nameOffset + 0x12 ..< nameOffset + 0x12 + id.count, with: id)
        guard let offset = getBlockOffset(track: directoryTrack, sector: 0) else { return nil }
        bytes.replaceSubrange(offset ..< offset + bam.count, with: bam)

        var block = Data(count: 256)
        block[1] = 0xff
        writeBLock(track: directoryTrack, sector: directorySector, data: block)
        block[0] = directoryTrack
        block[1] = directorySector
        
    }
    
    init?(bytes: Data) {
        DxxImage.initLayouts()
        
        guard let layout = DxxImage.layoutBySize[bytes.count] else { return nil }
        self.layout = layout
        self.bytes = bytes
    }
    
    func save() throws {
        guard let url = url else { return }
        try bytes.write(to: url)
    }
    
    var directoryTrack: UInt8 { return layout.directoryTrack }
    var directorySector: UInt8 { return layout.directorySector }
    var mediaType: DiskDrive.MediaType { return layout.mediaType }
    var tracks: Int { return layout.trackOffsets.count - 1 }
    
    func getBlock(track: UInt8, sector: UInt8) -> Data? {
        guard let offset = getBlockOffset(track: track, sector: sector) else { return nil }
        
        return bytes.subdata(in: offset*0x100..<(offset+1)*0x100)
    }
    
    mutating func writeBLock(track: UInt8, sector: UInt8, data: Data) {
        guard data.count == 256 else { return }
        guard let offset = getBlockOffset(track: track, sector: sector) else { return }

        bytes.replaceSubrange(offset*0x100..<(offset+1)*0x100, with: data)
    }
    
    func getBlockOffset(track: UInt8, sector: UInt8) -> Int? {
        guard track > 0 && track < layout.trackOffsets.count else { return nil }
        guard sector < layout.getSectors(track: track) else { return nil }
        
        return layout.getOffset(track: track) + Int(sector)
    }
    
    private struct TrackSizeZone {
        var firstTrack: UInt8
        var sectors: UInt8
    }

    private struct DiskLayoutTemplate {
        var zones: [TrackSizeZone]
        var tracks: [Int]
        var directoryTrack: UInt8
        var directorySector: UInt8
        var mediaType: DiskDrive.MediaType
    }
    private static var layoutBySize = [Int : DiskLayout]()
    private static var layoutByMeidaType = [DiskDrive.MediaType: DiskLayout]()
    private static var layoutTemplates: [DiskLayoutTemplate] = [
        // 1541
        DiskLayoutTemplate(zones: [
            TrackSizeZone(firstTrack: 1, sectors: 21),
            TrackSizeZone(firstTrack: 18, sectors: 19),
            TrackSizeZone(firstTrack: 25, sectors: 18),
            TrackSizeZone(firstTrack: 31, sectors: 17),
            ], tracks: [35, 40, 42], directoryTrack: 18, directorySector: 1, mediaType: .singleDensitySingleSided5_25),
        
        // 1571
        DiskLayoutTemplate(zones: [
            TrackSizeZone(firstTrack: 1, sectors: 21),
            TrackSizeZone(firstTrack: 18, sectors: 19),
            TrackSizeZone(firstTrack: 25, sectors: 18),
            TrackSizeZone(firstTrack: 31, sectors: 17),
            TrackSizeZone(firstTrack: 36, sectors: 21),
            TrackSizeZone(firstTrack: 53, sectors: 19),
            TrackSizeZone(firstTrack: 60, sectors: 18),
            TrackSizeZone(firstTrack: 66, sectors: 17)
            ], tracks: [70], directoryTrack: 18, directorySector: 1, mediaType: .singleDensityDoubleSided5_25),
        
        // 1581
        DiskLayoutTemplate(zones: [
            TrackSizeZone(firstTrack: 1, sectors: 40)
            ], tracks: [80, 81, 82, 83], directoryTrack: 40, directorySector: 3, mediaType: .doubleDensity3_5),
        
        // 8050
        DiskLayoutTemplate(zones: [
            TrackSizeZone(firstTrack: 1, sectors: 29),
            TrackSizeZone(firstTrack: 40, sectors: 27),
            TrackSizeZone(firstTrack: 54, sectors: 25),
            TrackSizeZone(firstTrack: 65, sectors: 23)
            ], tracks: [77], directoryTrack: 39, directorySector: 1, mediaType: .doubleDensitySingleSided5_25),
        
        // 8250
        DiskLayoutTemplate(zones: [
            TrackSizeZone(firstTrack: 1, sectors: 29),
            TrackSizeZone(firstTrack: 40, sectors: 27),
            TrackSizeZone(firstTrack: 54, sectors: 25),
            TrackSizeZone(firstTrack: 65, sectors: 23),
            TrackSizeZone(firstTrack: 78, sectors: 29),
            TrackSizeZone(firstTrack: 117, sectors: 27),
            TrackSizeZone(firstTrack: 131, sectors: 25),
            TrackSizeZone(firstTrack: 142, sectors: 23)
            ], tracks: [134], directoryTrack: 39, directorySector: 1, mediaType: .doubleDensityDoubleSided5_25)
    ]
    
    private static func initLayouts() {
        guard layoutBySize.isEmpty else { return }
        
        for template in layoutTemplates {
            for tracks in template.tracks {
                let layout = DiskLayout(template: template, tracks: tracks, hasErrorMap: false)
                let layoutWithErrorMap = DiskLayout(template: template, tracks: tracks, hasErrorMap: true)
                layoutBySize[layout.fileSize] = layout
                layoutBySize[layoutWithErrorMap.fileSize] = layoutWithErrorMap
                layoutByMeidaType[layout.mediaType] = layout
            }
        }
    }
}

class BitStream {
    private static var masks = [ 0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1 ]
    var bytes: Data
    var byteOffset = 0
    var bitOffset = 0
    
    init(bytes: Data) {
        self.bytes = bytes
        reset()
    }
    
    func reset() {
        byteOffset = 0
        bitOffset = 0
    }
    
    func end() -> Bool {
        return byteOffset == bytes.count
    }
    
    func peek(_ length: Int) -> Int? {
        var bitOffset = self.bitOffset
        var byteOffset = self.byteOffset

        guard byteOffset * 8 + bitOffset + length <= bytes.count * 8 else { return nil }
        
        var value = 0
        var n = length
        
        while (n > 0) {
            let bitsRemaining = min(n, 8-bitOffset)
            let currentBits = (Int(bytes[byteOffset]) & BitStream.masks[bitOffset]) >> (8 - (bitOffset + bitsRemaining))
            
            value = value << bitsRemaining | currentBits
            
            n -= bitsRemaining
            bitOffset += bitsRemaining
            if (bitOffset == 8) {
                bitOffset = 0
                byteOffset += 1
            }
        }
        
        return value
    }
    
    func get(_ length: Int) -> Int? {
        guard let value = peek(length) else { return nil }
        
        let newOffset = bitOffset + length
        
        bitOffset = newOffset % 8
        byteOffset += newOffset / 8
        
        return value
    }
    
    func rewind(_ length: Int) {
        let newOffset = max(byteOffset * 8 + bitOffset - length, 0)
        
        bitOffset = newOffset % 8
        byteOffset = newOffset / 8
    }
}

class GxxImage: DiskImage {
    var tracks: Int { return trackOffsets.count / 2 }
    
    var url: URL?
    
    var directoryTrack = UInt8(18)
    var directorySector = UInt8(1)
    var mediaType: DiskDrive.MediaType
    
    
    enum TrackData : Equatable {
        case NoData
        case NotRead
        case Sectors([Int: Data])
        case Error
        static func == (lhs: TrackData, rhs: TrackData) -> Bool {
            switch (lhs, rhs) {
            case (.NoData, .NoData), (.NotRead, .NotRead), (.Sectors(_), .Sectors(_)), (.Error, .Error):
                return true
            default:
                return false
            }
        }
    }

    var bytes: Data
    
    var trackData = [TrackData]()
    var trackOffsets = [Int]()
    
    init?(bytes: Data) {
        guard bytes.count >= 0x0c else { return nil } // too short for header

        let signature = String(bytes: bytes[0...7], encoding: .ascii)
        
        switch signature {
        case "GCR-1541":
            mediaType = .singleDensitySingleSided5_25
            
        case "GCR-1571":
            mediaType = .singleDensityDoubleSided5_25
            
        default:
            return nil
        }
        
        let numberOfHalftracks = Int(bytes[9])
        
        for track in 0..<numberOfHalftracks {
            let i = 0xc + track * 4
            let offset = Int(bytes[i]) | Int(bytes[i+1]) << 8 | Int(bytes[i+2]) << 16 | Int(bytes[i+3]) << 32
            trackOffsets.append(offset)
            trackData.append(offset == 0 ? .NoData : .NotRead)
        }
        
        self.bytes = bytes
    }
    
    func save() throws {
        guard let url = url else { return }
        try bytes.write(to: url)
    }

    func getBlock(track: UInt8, sector: UInt8) -> Data? {
        let halftrack = (Int(track)-1) * 2
        
        guard halftrack < trackData.count else { return nil }
        
        if trackData[halftrack] == TrackData.NotRead {
            trackData[halftrack] = readTrack(halftrack)
        }
        
        switch trackData[halftrack] {
        case .Error, .NoData, .NotRead:
            return nil
        case .Sectors(let sectors):
            if let data = sectors[Int(sector)] {
                return data
            }
            else {
                return nil
            }
        }
    }
    
    func writeBLock(track: UInt8, sector: UInt8, data: Data) {
        return
    }
    
    func readTrack(_ halftrack: Int) -> TrackData {
        let offset = trackOffsets[halftrack]
        guard offset > 0 && offset + 2 < bytes.count else { return .Error }
        let size = Int(bytes[offset]) | Int(bytes[offset+1]) << 8
        guard offset + size < bytes.count else { return .Error }
        
/*
        for i in (offset..<offset+size) {
            let byte = bytes[i]
            var mask = UInt8(0x80)
            while mask > 0 {
                print((byte & mask) == 0 ? "0" : "1", terminator: "")
                mask >>= 1
            }
        }
 */
        let bits = BitStream(bytes: bytes.subdata(in: offset..<offset+size))
        
        var sectors = [Int: Data]()
        
        while (findSync(bits)) {
//            print("header sync")
            guard let header = decodeGCR(bits, 6) else { break }
//            print(String(format: "header: %02x %02x %02x %02x %02x %02x %02x %02x", header[0], header[1], header[2], header[3], header[4], header[5]))
            
            guard header[0] == 0x08 && header[1] == header[2] ^ header[3] ^ header[4] ^ header[5] else { break }
            
            guard findSync(bits) else { break }
//            print("data sync")
            guard let data = decodeGCR(bits, 0x102) else { break }
//            print(String(format: "data %02x ...", data[0]))
            
            let sector = Int(header[2])
            
            guard data[0] == 0x07 else { continue }
            
            var eor: UInt8 = 0
            
            for i in 1...0x100 {
                eor ^= data[i]
            }
            
            // guard eor == data[0x101] else { continue }
            
            sectors[sector] = data.subdata(in: 1..<0x101)
        }
        
        return .Sectors(sectors)
    }
    
    func findSync(_ bits: BitStream) -> Bool {
        var n = 0
        
        while (true) {
            guard let bit = bits.get(1) else { return false }
            
            if (bit == 0) {
                if (n >= 10) {
                    bits.rewind(1)
                    return true
                }
                n = 0
            }
            else {
                n += 1
            }
        }
    }
    
    // See https://en.wikipedia.org/wiki/Group_coded_recording#Commodore
    
    static var GCR: [UInt8] = [
        /* 0x00 */
        0xff, 0xff, 0xff, 0xff,
        /* 0x04 */
        0xff, 0xff, 0xff, 0xff,
        /* 0x08 */
        0xff, 0x08, 0x00, 0x01,
        /* 0x0c */
        0xff, 0x0c, 0x04, 0x05,
        /* 0x10 */
        0xff, 0xff, 0x02, 0x03,
        /* 0x14 */
        0xff, 0x0f, 0x06, 0x07,
        /* 0x18 */
        0xff, 0x09, 0x0a, 0x0b,
        /* 0x1c */
        0xff, 0x0d, 0x0e, 0xff,
    ]
    func decodeGCR(_ bits: BitStream, _ n: Int) -> Data? {
        var data = Data(repeating: 0, count: n)
        
        for i in 0..<n {
            guard let highGCR = bits.get(5) else { return nil }
            guard let lowGCR = bits.get(5) else { return nil }
            
/*
            if GxxImage.GCR[highGCR] & 0xf0 != 0 {
                print(String(format: "invalid GCR at \(i).0: %02x", highGCR))
            }
            if GxxImage.GCR[lowGCR] & 0xf0 != 0 {
                print(String(format: "invalid GCR at \(i).1: %02x", lowGCR))
            }
 */

            guard GxxImage.GCR[lowGCR] & 0xf0 == 0 && GxxImage.GCR[highGCR] & 0xf0 == 0 else { return nil }
            data[i] = GxxImage.GCR[lowGCR] | GxxImage.GCR[highGCR] << 4
        }
        
        return data
    }
}

struct StubImage: DiskImage {
    var directoryTrack = UInt8(18)
    var directorySector = UInt8(1)
    var mediaType: DiskDrive.MediaType
    var tracks: Int
    var url: URL?
    
    private struct Layout {
        var mediaType: DiskDrive.MediaType
        var tracks: Int
    }
    
    private static let layouts = [
        "d1m": Layout(mediaType: .cmdDoubleDensity3_5, tracks: 81),
        "d2m": Layout(mediaType: .cmdHighDensity3_5, tracks: 81),
        "d4m": Layout(mediaType: .cmdExtendedDensity3_5, tracks: 81),
        "d64": Layout(mediaType: .singleDensitySingleSided5_25, tracks: 35),
        // "d67": Layout(mediaType: .singleDensitySingleSided5_25, tracks: 35), // not used on C64
        // "d71": Layout(mediaType: .singleDensityDoubleSided5_25, tracks: 70), // not used on C64
        // "d80": Layout(mediaType: .doubleDensitySingleSided5_25, tracks: 77), // not used on C64
        "d81": Layout(mediaType: .doubleDensity3_5, tracks: 80),
        // "d82": Layout(mediaType: .doubleDensityDoubleSided5_25, tracks: 134), // not used on C64
        "g64": Layout(mediaType: .singleDensitySingleSided5_25, tracks: 35),
        // "g71": Layout(mediaType: .singleDensityDoubleSided5_25, tracks: 70), // not used on C64
        "p64": Layout(mediaType: .singleDensitySingleSided5_25, tracks: 35)
    ]
    
    init?(url: URL) {
        guard let layout = StubImage.layouts[url.pathExtension.lowercased()] else { return nil }
        self.mediaType = layout.mediaType
        self.tracks = layout.tracks
        self.url = url
    }
    
    func save() throws {
        return
    }

    func getBlock(track: UInt8, sector: UInt8) -> Data? {
        return nil
    }
    
    func writeBLock(track: UInt8, sector: UInt8, data: Data) {
        return
    }
    
    func readFreeBlocks() -> Int? {
        return nil
    }
}
