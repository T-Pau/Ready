/*
 BiosSettingsViewController.swift
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

import UIKit
import C64UIComponents

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
            Row(name: "CMD FD-2000", defaultsKey: .BiosFD2000),
            Row(name: "CMD FD-4000", defaultsKey: .BiosFD4000)
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
        cell.mediaView?.item = CartridgeImage.init(directory: AppDelegate.biosURL, file: Defaults.standard.stringValue(for: row.defaultsKey))
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
                let directory = AppDelegate.biosURL
                do {
                    try ensureDirectory(directory)

                    fileName = try uniqeName(directory: directory, name: item.itemProvider.suggestedName ?? "unknown", pathExtension: type.pathExtension)
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
