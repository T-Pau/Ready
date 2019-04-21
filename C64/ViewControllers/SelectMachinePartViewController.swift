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
    var machineParts = [MachinePart]()
    var selectedIdentifier: String?
    
    var changeHandler: ((SelectMachinePartViewController) -> ())?
    
    var iconWidth: CGFloat = 39

    var selectedRow: Int? {
        guard let identifier = selectedIdentifier else { return nil }
        
        return machineParts.firstIndex(where: { $0.identifier == identifier })
    }

    override func viewWillDisappear(_ animated: Bool) {
        changeHandler?(self)
    }
}

extension SelectMachinePartViewController: UITableViewDelegate, UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return machineParts.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let rawCell = tableView.dequeueReusableCell(withIdentifier: "MachinePart", for: indexPath)
        guard let cell = rawCell as? MachinePartTableViewCell else { return rawCell }
        
        cell.configure(from: machineParts[indexPath.row])
        cell.iconWidthConstraint.constant = iconWidth
        cell.accessoryType = indexPath.row == selectedRow ? .checkmark : .none
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard indexPath.row != selectedRow else { return }

        var changedIndexPaths = [ indexPath ]
        
        if let oldRow = selectedRow {
            changedIndexPaths.append(IndexPath(row: oldRow, section: 0))
        }
        selectedIdentifier = machineParts[indexPath.row].identifier
        
        tableView.reloadRows(at: changedIndexPaths, with: .automatic)
        tableView.deselectRow(at: indexPath, animated: true)
    }
}
