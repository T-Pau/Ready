/*
 SingularAdapterStatusViewController.swift
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
import C64UIComponents
import Emulator

class SingularAdapterStatusViewController: UIViewController {
    @IBOutlet weak var iconView: UIImageView!
    @IBOutlet weak var titleLabel: UILabel!
    @IBOutlet weak var tableView: UITableView!
    @IBOutlet weak var subtitleLabel: UILabel!
    
    var singularAdapter: UserPortModule?
    var selectedRow: Int?
    
    var selectedValue: MachineConfigOld.SingularAdapterMode? {
        get {
            guard let row = selectedRow else { return nil }
            return MachineConfigOld.SingularAdapterMode.allCases[row]
        }
        set {
            if let value = newValue {
                selectedRow = MachineConfigOld.SingularAdapterMode.allCases.firstIndex(of: value)
            }
            else {
                selectedRow = nil
            }
        }
    }
    
    var changeCallback: ((_: MachineConfigOld.SingularAdapterMode) -> ())?
    
    private var rowHeight = CGFloat(44) // TODO: don't hardcode
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        iconView.image = singularAdapter?.icon
        
        titleLabel.text = singularAdapter?.fullName

        let height = rowHeight * CGFloat(MachineConfigOld.SingularAdapterMode.allCases.count) + tableView.frame.minY
        
        preferredContentSize = CGSize(width: 400, height: min(height, 600.0))

    }
}

extension SingularAdapterStatusViewController: UITableViewDelegate, UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return MachineConfigOld.SingularAdapterMode.allCases.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Disk", for: indexPath)

        let value = MachineConfigOld.SingularAdapterMode.allCases[indexPath.row]
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
