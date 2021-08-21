/*
 BiosSettingsViewController.swift
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
import C64UIComponents
import Emulator

class BiosSettingsViewController: UIViewController {
    struct Section {
        var title: String?
        var rows: [Row]
    }
    
    struct Row {
        var name: String
        var defaultsKey: Defaults.Key
    }
    
    let sections = [
        Section(title: "JiffyDOS", rows: [
            Row(name: "Commodore 64", defaultsKey: .BiosJiffyDosC64),
            Row(name: "Commodore 1541", defaultsKey: .BiosJiffyDos1541),
            Row(name: "Commodore 1541-II", defaultsKey: .BiosJiffyDos1541II),
            Row(name: "Commodore 1571", defaultsKey: .BiosJiffyDos1571),
            Row(name: "Commodore 1581", defaultsKey: .BiosJiffyDos1581)
        ]),
        Section(title: "Device ROM", rows: [
            Row(name: "CMD FD-2000", defaultsKey: .BiosCmdFd2000),
            Row(name: "CMD FD-4000", defaultsKey: .BiosCmdFd4000),
            Row(name: "CMD HD", defaultsKey: .BiosCmdHd),
            Row(name: "CMD RAMLink", defaultsKey: .BiosCmdRamlink)
        ])
    ]
    
    var mediaViews = NSMapTable<MediaView, NSIndexPath>(keyOptions: [.weakMemory], valueOptions: [.strongMemory])
    
    func row(for indexPath: IndexPath) -> Row {
        return sections[indexPath.section].rows[indexPath.row]
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension BiosSettingsViewController: UITableViewDataSource, UITableViewDelegate {
    func numberOfSections(in tableView: UITableView) -> Int {
        return sections.count
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return sections[section].rows.count
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return sections[section].title
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let genericCell = tableView.dequeueReusableCell(withIdentifier: "Media", for: indexPath)
        
        guard let cell = genericCell as? MediaTableViewCell else { return genericCell }
        
        let row = self.row(for: indexPath)
        
        cell.titleLabel?.text = row.name
        cell.mediaView?.item = CartridgeImage.init(directory: Defaults.biosURL, file: Defaults.standard.stringValue(for: row.defaultsKey))
        cell.mediaView?.acceptableDropTypes = C64FileType.MediaType.cartridge.typeIdentifiers
        cell.mediaView?.dropDelegate = self
        cell.mediaView?.textAlignment = .right
        cell.mediaView?.showIcon = false
        mediaViews.setObject(indexPath as NSIndexPath, forKey: cell.mediaView)
        return cell
    }
}

extension BiosSettingsViewController: MediaViewDropDelegate {
    func mediaView(_ sender: MediaView, dropped item: UIDragItem, of typeIdentifier: String) {
        guard let indexPath = mediaViews.object(forKey: sender) as IndexPath? else { return }
        guard let type = C64FileType(typeIdentifier: typeIdentifier) else { return }

        let row = self.row(for: indexPath)
        
        
        item.itemProvider.loadDataRepresentation(forTypeIdentifier: typeIdentifier) { data, error in
            guard let data = data else { return }
            DispatchQueue.main.async {
                
                let oldFileName = Defaults.standard.stringValue(for: row.defaultsKey)
                let fileName: URL
                let directory = Defaults.biosURL
                do {
                    try ensureDirectory(directory)

                    fileName = try uniqueName(directory: directory, name: item.itemProvider.suggestedName ?? "unknown", pathExtension: type.pathExtension)
                    try data.write(to: fileName)

                    Defaults.standard.set(value: fileName.lastPathComponent, for: row.defaultsKey)

                    if let oldFileName = oldFileName {
                        try FileManager.default.removeItem(at: directory.appendingPathComponent(oldFileName))
                    }
                }
                catch {
                    return
                    
                }
                sender.item = CartridgeImage(url: fileName)
            }
        }
    }
}
