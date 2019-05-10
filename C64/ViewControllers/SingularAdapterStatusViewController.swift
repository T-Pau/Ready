/*
 SingularAdapterStatusViewController.swift
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
import C64UIComponents


class SingularAdapterStatusViewController: UIViewController {
    @IBOutlet weak var iconView: UIImageView!
    @IBOutlet weak var titleLabel: UILabel!
    @IBOutlet weak var tableView: UITableView!
    @IBOutlet weak var subtitleLabel: UILabel!
    
    var singularAdapter: UserPortModule?
    var selectedRow: Int?
    
    var selectedValue: MachineConfig.SingularAdapterMode? {
        get {
            guard let row = selectedRow else { return nil }
            return MachineConfig.SingularAdapterMode.allCases[row]
        }
        set {
            if let value = newValue {
                selectedRow = MachineConfig.SingularAdapterMode.allCases.firstIndex(of: value)
            }
            else {
                selectedRow = nil
            }
        }
    }
    
    var changeCallback: ((_: MachineConfig.SingularAdapterMode) -> ())?
    
    private var rowHeight = CGFloat(44) // TODO: don't hardcode
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        iconView.image = singularAdapter?.icon
        
        titleLabel.text = singularAdapter?.fullName

        let height = rowHeight * CGFloat(MachineConfig.SingularAdapterMode.allCases.count) + tableView.frame.minY
        
        preferredContentSize = CGSize(width: 400, height: min(height, 600.0))

    }
}

extension SingularAdapterStatusViewController: UITableViewDelegate, UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return MachineConfig.SingularAdapterMode.allCases.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Disk", for: indexPath)

        let value = MachineConfig.SingularAdapterMode.allCases[indexPath.row]
        cell.textLabel?.text = value.description
        cell.accessoryType = selectedValue == value ? .checkmark : .none
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.row != selectedRow {
            selectedRow = indexPath.row
            if let value = selectedValue {
                
                changeCallback?(value)
            }
        }
        dismiss(animated: true)
    }
}
