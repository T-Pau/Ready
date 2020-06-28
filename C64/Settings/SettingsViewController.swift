/*
 SettingsViewController.swift
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

class SettingsViewController: UIViewController, SeguePreparer {
    enum SegueType: String {
        case showAbout
        case showAppIcon
        case showBiosSettings
        case showGeneralSettings
        case showLicenses
        case showMachineConfiguration
        case showReleaseNotes
    }
    
    struct Row {
        var name: String
        var segueIdentifier: SegueType
    }
    
    struct Section {
        var title: String?
        var rows: [Row]
    }

    let sections: [Section] = [
        Section(title: nil, rows: [
            Row(name: "App Icon", segueIdentifier: .showAppIcon),
            Row(name: "General", segueIdentifier: .showGeneralSettings),
            Row(name: "Default Machine Configuration", segueIdentifier: .showMachineConfiguration),
            Row(name: "BIOS", segueIdentifier: .showBiosSettings)
        ]),
        Section(title: nil, rows: [
            Row(name: "About", segueIdentifier: .showAbout),
            Row(name: "Release Notes", segueIdentifier: .showReleaseNotes)
        ]),
        Section(title: nil, rows: [
            Row(name: "Licenses", segueIdentifier: .showLicenses)
        ])
    ]
    
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }

        switch segueType {
        case .showMachineConfiguration:
            guard let destination = segue.destination as? MachinePartsViewController else { return }
            destination.machineSpecification = Defaults.standard.machineSpecification
            destination.changeHandler = { machineSpecification in
                do {
                    try machineSpecification.save()
                }
                catch { }
            }
            
        default:
            break
        }
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension SettingsViewController: UITableViewDelegate, UITableViewDataSource {
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
        let cell = tableView.dequeueReusableCell(withIdentifier: "Basic", for: indexPath)
        
        cell.textLabel?.text = rowFor(indexPath: indexPath).name
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let segueType = rowFor(indexPath: indexPath).segueIdentifier
        
        performSegue(withIdentifier: segueType.rawValue, sender: self)
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    private func rowFor(indexPath: IndexPath) -> Row {
        return sections[indexPath.section].rows[indexPath.row]
    }
}
