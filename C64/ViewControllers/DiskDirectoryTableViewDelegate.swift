/*
 DiskDirectoryTableViewDelegate -- UITableViewDelegate for Disk Directory Listing
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

@objc class DiskDirectoryTableViewDelegate: NSObject, UITableViewDelegate, UITableViewDataSource  {
    var disk: DiskImage? {
        didSet {
            updateDirectory()
            if !directoryLines.isEmpty {
                tableView?.scrollToRow(at: IndexPath(row: 0, section: 0), at: .top, animated: false)
            }
        }
    }
    var chargen: Chargen? {
        didSet {
            tableView?.reloadData()
        }
    }
    var tableView: UITableView? {
        didSet {
            tableView?.delegate = self
            tableView?.dataSource = self
        }
    }

    private static let typeString: [String] = [
        "DEL", "SEQ", "PRG", "USR",
        "REL", "???", "???", "???",
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
        
        line = Chargen.petsciiToScreen(line)
        for i in (2 ..< line.count) {
            line[i] = line[i] ^ 0x80
        }
        return line
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

        return Chargen.petsciiToScreen(line)
    }
    
    private func getBlocksFreeLine(directory: Directory) -> [UInt8] {
        return Chargen.petsciiToScreen("\(directory.freeBlocks) BLOCKS FREE.".unicodeScalars.map({ UInt8($0.value) }))
    }
}
