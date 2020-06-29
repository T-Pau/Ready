/*
 AppIconViewController.swift
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
import Emulator

class AppIconViewController: UIViewController {
    struct Row {
        var name: String
        var iconName: String
        var defaultIcon = false
    }
    
    struct Section {
        var title: String?
        var rows: [Row]
    }
    
    let sections: [Section] = [
        Section(title: nil, rows: [
            Row(name: "Commodore 64", iconName: "App Icon C64", defaultIcon: true),
            Row(name: "Commodore SX-64", iconName: "App Icon SX-64"),
            Row(name: "Commodore C16, Plus/4", iconName: "App Icon C16"),
            Row(name: "Commodore VIC-20", iconName: "App Icon VIC-20"),
            // Row(name: "Sinclair ZX Spectrum 16k, 48k", iconName: "App Icon ZX-48")
        ])
    ]
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension AppIconViewController: UITableViewDelegate, UITableViewDataSource {
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
        let cell = tableView.dequeueReusableCell(withIdentifier: "AppIcon", for: indexPath)
        
        if let cell = cell as? AppIconCell {
            let row = rowFor(indexPath: indexPath)
            cell.titleLabel?.text = row.name
            cell.iconView?.image = UIImage(named: row.iconName + " Preview")
            let currentIconName = UIApplication.shared.alternateIconName
            if row.defaultIcon && currentIconName == nil || row.iconName == currentIconName {
                cell.accessoryType = .checkmark
            }
            else {
                cell.accessoryType = .none
            }
        }
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let row = rowFor(indexPath: indexPath)
        tableView.deselectRow(at: indexPath, animated: true)
        UIApplication.shared.setAlternateIconName(row.defaultIcon ? nil : row.iconName) { error in
            if error == nil {
                tableView.reloadData()
            }
        }
    }
    
    private func rowFor(indexPath: IndexPath) -> Row {
        return sections[indexPath.section].rows[indexPath.row]
    }
}
