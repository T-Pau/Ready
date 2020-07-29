/*
 SettingsViewController.swift
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
