/*
 DiskDirectoryTableViewDelegate -- UITableViewDelegate for Disk Directory Listing
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

import UIKit
import Emulator

@objc class DiskDirectoryTableViewDelegate: NSObject, UITableViewDelegate, UITableViewDataSource  {
    var disk: DiskImage? {
        didSet {
            updateDirectory()
            if !directoryLines.isEmpty {
                tableView?.scrollToRow(at: IndexPath(row: 0, section: 0), at: .top, animated: false)
            }
        }
    }
    var useUppercase = true
    var isGEOS = false
    var chargenUppercase: Chargen? {
        didSet {
            tableView?.reloadData()
        }
    }
    var chargenLowercase: Chargen? {
        didSet {
            tableView?.reloadData()
        }
    }
    
    var chargen: Chargen? {
        return useUppercase ? chargenUppercase : chargenLowercase
    }
    
    var tableView: UITableView? {
        didSet {
            tableView?.delegate = self
            tableView?.dataSource = self
        }
    }

    private static let typeString: [String] = [
        "DEL", "SEQ", "PRG", "USR",
        "REL", "CBM", "DIR", "???",
        "???", "???", "???", "???",
        "???", "???", "???", "???"
    ]

    let reuseIdentifier = "DirectoryLine"
    
    private var directoryLines = [[UInt8]]()
        
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return directoryLines.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let rawCell = tableView.dequeueReusableCell(withIdentifier: reuseIdentifier, for: indexPath)
        
        guard let cell = rawCell as? DiskDirectoryTableViewCell else { return rawCell }
        
        cell.lineImage = chargen?.render(line: directoryLines[indexPath.row])
        
        return cell
    }
    
    func updateDirectory() {
        directoryLines.removeAll()
        
        if let directory = disk?.readDirectory() {
            isGEOS = directory.isGEOS
            useUppercase = !directory.isGEOS
            directoryLines.append(getTitleLine(directory: directory))
            directoryLines.append(contentsOf: directory.entries.map( { getEntryLine(entry: $0) }))
            directoryLines.append(getBlocksFreeLine(directory: directory))
        }

        tableView?.reloadData()
    }
    
    private func getTitleLine(directory: Directory) -> [UInt8] {
        var line = [UInt8]()
        line.append(contentsOf: [ 0x30, 0x20, 0x22 ])
        line.append(contentsOf: directory.diskNamePETASCII)
        line.append(contentsOf: [ 0x22, 0x20 ])
        line.append(contentsOf: directory.diskIdPETASCII)
        
        return convert(line: line, invert: true)
    }
    
    private func getEntryLine(entry: Directory.Entry) -> [UInt8] {
        var line = [UInt8](repeating: 0x20, count: 5)
        for (index, char) in String(entry.blocks).unicodeScalars.enumerated() {
            line[index] = UInt8(char.value)
        }
        line.append(0x22)
        line.append(contentsOf: entry.namePETASCII)
        line.append(0x22)
        line.append(contentsOf: entry.postNamePETASCII)
        line.append(entry.closed ? 0x20 : 0x2a)
        line.append(contentsOf: DiskDirectoryTableViewDelegate.typeString[Int(entry.fileType)].unicodeScalars.map { UInt8($0.value) })
        if entry.locked {
            line.append(0x3c)
        }

        return convert(line: line)
    }
    
    private func getBlocksFreeLine(directory: Directory) -> [UInt8] {
        return convert(line: "\(directory.freeBlocks) BLOCKS FREE.".unicodeScalars.map({ UInt8($0.value) }))
    }
    
    private func convert(line: [UInt8], invert: Bool = false) -> [UInt8] {
        var converted = isGEOS ? Chargen.asciiToScreen(line) : Chargen.petsciiToScreen(line)
        if invert {
            for i in (2 ..< converted.count) {
                converted[i] = converted[i] ^ 0x80
            }
        }
        return converted
    }
}
