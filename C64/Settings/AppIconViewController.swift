/*
 AppIconViewController.swift
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
            Row(name: "Atari 600XL, 800XL", iconName: "App Icon XL"),
            Row(name: "Commander X16", iconName: "App Icon X16"),
            Row(name: "Commodore 64", iconName: "App Icon C64", defaultIcon: true),
            Row(name: "Commodore SX-64", iconName: "App Icon SX-64"),
            Row(name: "Commodore 16, Plus/4", iconName: "App Icon C16"),
            Row(name: "Commodore VIC-20", iconName: "App Icon VIC-20"),
            Row(name: "Sinclair ZX Spectrum 16k, 48k", iconName: "App Icon ZX-48"),
            Row(name: "Sinclair ZX Spectrum 128k, +2", iconName: "App Icon ZX-128")
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
