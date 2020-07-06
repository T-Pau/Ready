/*
 LicensesViewController.swift
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

class LicensesViewController: UIViewController {
    struct Section {
        var title: String?
        
        var rows: [String]
    }
    
    let sections = [
        Section(title: "Application and Emulators", rows: [
            "Ready",
            "Fuse",
            "Vice",
            "GNU General Public License"
        ]),
        Section(title: "Keyboard Images", rows: [
            "Japanese C64",
            "PET",
            "Ultimax",
            "VIC-1001",
            "ZX Spectrum+"
        ]),
        Section(title: "3rd Party Software", rows: [
            "CwlMutex",
            "IDEDOS",
            "Multiset",
            "ringbuffer",
            "SearchTextField",
        ]),
        Section(title: nil, rows: [
            "Sepcial Thanks"
        ])
    ]
    
    func component(for indexPath: IndexPath) -> String {
        return sections[indexPath.section].rows[indexPath.row]
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension LicensesViewController: UITableViewDataSource, UITableViewDelegate {
    func numberOfSections(in tableView: UITableView) -> Int {
        return sections.count
    }
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return sections[section].rows.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Basic", for: indexPath)

        cell.textLabel?.text = component(for: indexPath)
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let controller = UIStoryboard(name: "Main", bundle: nil).instantiateViewController(withIdentifier: "License") as? LicenseViewController else { return }
        
        controller.component = component(for: indexPath)

        navigationController?.pushViewController(controller, animated: true)
        
        tableView.deselectRow(at: indexPath, animated: true)
    }
}
