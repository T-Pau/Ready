/*
 EnumChooserViewController.swift -- Choose Enum Settings Value
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

class EnumChooserViewController: UIViewController {
    @IBInspectable var reuseIdentifier: String = "Cell"

    var choices = [Any]()
    var selectedRow: Int?

    var changeHandler: ((Int, Any) -> ())?
}

extension EnumChooserViewController: UITableViewDataSource, UITableViewDelegate {
    func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return choices.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: reuseIdentifier, for: indexPath)

        cell.textLabel?.text = "\(choices[indexPath.row])"
        cell.accessoryType = indexPath.row == selectedRow ? .checkmark : .none
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        var changedRows = [ indexPath ]
        
        if let oldSelectedRow = selectedRow {
            changedRows.append(IndexPath(row: oldSelectedRow, section: 0))
        }
        
        changeHandler?(indexPath.row, choices[indexPath.row])
        
        selectedRow = indexPath.row
        
        tableView.reloadRows(at: changedRows, with: .automatic)
        tableView.deselectRow(at: indexPath, animated: true)
    }
}
