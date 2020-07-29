/*
 LicensesViewController.swift
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

class LicensesViewController: UIViewController {
    struct Section {
        var title: String?
        
        var rows: [String]
    }
    
    let sections = [
        Section(title: "Application and Emulators", rows: [
            "Ready",
            "Atari800",
            "Fuse",
            "Vice",
            "x16-emulator",
            "GNU General Public License"
        ]),
        Section(title: "Keyboard Images", rows: [
            "Japanese C64",
            "PET",
            "Ultimax",
            "VIC-1001",
            "X16",
            "ZX Spectrum+",
            "ZX Spectrum +2"
        ]),
        Section(title: "3rd Party Software", rows: [
            "CwlMutex",
            "IDEDOS",
            "Multiset",
            "SearchTextField",
        ]),
        Section(title: nil, rows: [
            "Special Thanks"
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
