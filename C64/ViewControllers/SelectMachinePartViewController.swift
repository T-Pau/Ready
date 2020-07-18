/*
 SelectMachinePartController.swift
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

class SelectMachinePartViewController: UIViewController {
    @IBOutlet weak var tableView: UITableView!

    var machineParts = MachinePartList(sections: [])
    var selectedIdentifier: String?
    var isGrouped = true
    
    var changeHandler: ((SelectMachinePartViewController) -> ())?
    
    var iconWidth: CGFloat = 39
    var dismissOnSelect = false

    var selectedIndexPath: IndexPath? {
        guard let identifier = selectedIdentifier else { return nil }
        
        for (section, machinePartSection) in machineParts.sections.enumerated() {
            if let row = machinePartSection.parts.firstIndex(where: { $0.identifier == identifier }) {
                return IndexPath(row: row, section: section)
            }
        }
        
        return nil
    }
    
    override func viewWillAppear(_ animated: Bool) {
        if !isGrouped {
            tableView.sectionHeaderHeight = 0.0
        }
        if let indexPath = selectedIndexPath {
            tableView.scrollToRow(at: indexPath, at: .middle, animated: false)
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        changeHandler?(self)
    }
}

extension SelectMachinePartViewController: UITableViewDelegate, UITableViewDataSource {
    func numberOfSections(in tableView: UITableView) -> Int {
        return machineParts.sections.count
    }

    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return machineParts.sections[section].parts.count
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return machineParts.sections[section].title
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let rawCell = tableView.dequeueReusableCell(withIdentifier: "MachinePart", for: indexPath)
        guard let cell = rawCell as? MachinePartTableViewCell else { return rawCell }
        
        cell.configure(from: machineParts[indexPath])
        cell.iconWidthConstraint.constant = iconWidth
        cell.accessoryType = indexPath == selectedIndexPath ? .checkmark : .none
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        var changedIndexPaths = [IndexPath]()

        if indexPath != selectedIndexPath {
            changedIndexPaths.append(indexPath)
            if let oldIndexPath = selectedIndexPath {
                changedIndexPaths.append(oldIndexPath)
            }
            selectedIdentifier = machineParts[indexPath].identifier
        }
            
        if dismissOnSelect {
            navigationController?.popViewController(animated: true)
        }
        else {
            tableView.reloadRows(at: changedIndexPaths, with: .automatic)
            tableView.deselectRow(at: indexPath, animated: true)
        }
    }
}
