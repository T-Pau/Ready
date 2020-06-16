/*
 InputMappingTableViewController.swift
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

class InputMappingTableViewController: UITableViewController, SeguePreparer {
    enum SegueType: String {
        case selectDevice
    }

    var inputMapping: InputMapping?
 
    // MARK: - Navigation

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }
        
        switch segueType {
        case .selectDevice:
            guard let destination = segue.destination as? SelectMachinePartViewController,
            let indexPath = tableView.indexPathForSelectedRow,
            let inputMapping = inputMapping else { return }
            let port = inputMapping.ports[indexPath.row]
            destination.dismissOnSelect = true
            destination.machineParts = MachinePartList(sections: [MachinePartSection(title: nil, parts: inputMapping.devices.filter({ $0.supports(inputType: port.inputType) }))])
            destination.isGrouped = false
            destination.selectedIdentifier = inputMapping[port]?.device.identifier
            destination.changeHandler = { sender in
                guard let identifier = destination.selectedIdentifier,
                 let device = inputMapping.devices.first(where: { $0.identifier == identifier }) else { return }
                inputMapping.pair(port: port, device: device, isManual: true)
                self.tableView.reloadData()
            }
        }
    }

    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension InputMappingTableViewController {
    // MARK: - Table view data source
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        // TODO
        return inputMapping?.ports.count ?? 0
    }
    
     override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let rawCell = tableView.dequeueReusableCell(withIdentifier: "Mapping", for: indexPath)
        guard let cell = rawCell as? InputMappingTableViewCell else { return rawCell }

        guard let port = inputMapping?.ports[indexPath.row] else { return cell }
        if let pairing = inputMapping?.mappedPorts[port] {
            cell.configure(from: pairing)
        }
        else {
            cell.configure(fromPort: port)
            cell.configure(fromDevice: nil)
        }
        
        return cell
     }
}
