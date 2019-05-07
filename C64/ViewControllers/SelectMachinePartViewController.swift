/*
 SelectMachinePartController.swift
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
