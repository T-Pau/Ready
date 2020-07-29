/*
 InputMappingTableViewController.swift
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
