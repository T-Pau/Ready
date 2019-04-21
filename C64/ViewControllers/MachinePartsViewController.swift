/*
 MachinePartViewController.swift
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

class MachinePartsViewController: UIViewController, SeguePreparer {
    enum SegueType: String {
        case selectPart
    }
    
    @IBOutlet weak var screenView: MachinePartView!
    @IBOutlet weak var cassetteView: MachinePartView!
    @IBOutlet weak var userPortView: MachinePartView!
    @IBOutlet weak var computerView: MachinePartView!
    @IBOutlet weak var expansionPortView: MachinePartView!
    @IBOutlet var driveViews: [MachinePartView]!
    @IBOutlet var controlPortViews: [MachinePartView]!
    @IBOutlet weak var userPortJoystick1View: MachinePartView!
    @IBOutlet weak var userPortJoystick2View: MachinePartView!
    @IBOutlet weak var cartridge2View: MachinePartView!
    
    var machineSpecification = MachineSpecification(layers: [])
    var machine: Machine?
    var isMachineRunning = false
    
    var changeHandler: ((MachineSpecification) -> ())?
    
    private struct PartInfo {
        var view: MachinePartView!
        var key: MachineConfig.Key
        var iconWidth: CGFloat
        var editableWhenRunning: Bool
    }
    
    private var partInfos = [PartInfo]()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let narrow = CGFloat(40)
        let wide = CGFloat(97)

        partInfos = [
            PartInfo(view: cassetteView, key: .cassetteDrive, iconWidth: narrow, editableWhenRunning: false),
            PartInfo(view: computerView, key: .computer, iconWidth: wide, editableWhenRunning: false),
            PartInfo(view: controlPortViews[0], key: .controlPort1, iconWidth: narrow, editableWhenRunning: true),
            PartInfo(view: controlPortViews[1], key: .controlPort2, iconWidth: narrow, editableWhenRunning: true),
            PartInfo(view: driveViews[0], key: .diskDrive8, iconWidth: wide, editableWhenRunning: false),
            PartInfo(view: driveViews[1], key: .diskDrive9, iconWidth: wide, editableWhenRunning: false),
            PartInfo(view: driveViews[2], key: .diskDrive10, iconWidth: wide, editableWhenRunning: false),
            PartInfo(view: driveViews[3], key: .diskDrive11, iconWidth: wide, editableWhenRunning: false),
            PartInfo(view: userPortView, key: .userPort, iconWidth: narrow, editableWhenRunning: false),
            PartInfo(view: userPortJoystick1View, key: .userPortJoystick1, iconWidth: narrow, editableWhenRunning: true),
            PartInfo(view: userPortJoystick2View, key: .userPortJoystick2, iconWidth: narrow, editableWhenRunning: true),
            PartInfo(view: expansionPortView, key: .expansionPort, iconWidth: narrow, editableWhenRunning: false)
            // TODO: screen, cartridges
        ]
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        for partInfo in partInfos {
            if isMachineRunning && !partInfo.editableWhenRunning {
                partInfo.view.insideColor = UIColor.white.withAlphaComponent(0.33)
            }
            else {
                partInfo.view.insideColor = UIColor.white
            }
        }
        
        updateSpecification()
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        changeHandler?(machineSpecification)
    }
    
    // MARK: - Actions
    
    @IBAction func cancel(_ sender: Any) {
        dismiss(animated: true)
    }

    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
        
    @IBAction func selectPart(_ sender: Any) {
        guard let view = sender as? MachinePartView,
            let partInfo = partInfo(for: view) else { return }

        if isMachineRunning && !partInfo.editableWhenRunning {
            return
        }

        performSegue(withIdentifier: SegueType.selectPart.rawValue, sender: sender)
    }

    // MARK: - Navigation

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }

        switch segueType {
        case .selectPart:
            guard let view = sender as? MachinePartView,
                let partInfo = partInfo(for: view),
                let destination = segue.destination as? SelectMachinePartViewController else { return }
            
            destination.machineParts = machineSpecification.partList(for: partInfo.key, machine: machine)
            destination.selectedIdentifier = machineSpecification.topLayerIdentifier(for: partInfo.key)
            destination.iconWidth = partInfo.iconWidth
            destination.changeHandler = { sender in
                var identifier = destination.selectedIdentifier
                if identifier == "default" {
                    identifier = nil
                }
                self.machineSpecification.set(string: identifier, for: partInfo.key)
                self.update(partInfo: partInfo)
            }
        }
    }

    // MARK: - Utility Functions

    func updateSpecification() {
        for partInfo in partInfos {
            update(partInfo: partInfo)
        }
        let ports = machineSpecification.computer.ports
        driveViews[0].isHidden = !ports.contains(.diskDrive8)
        driveViews[1].isHidden = !ports.contains(.diskDrive9)
        driveViews[2].isHidden = !ports.contains(.diskDrive10)
        driveViews[3].isHidden = !ports.contains(.diskDrive11)
        cassetteView.isHidden = !ports.contains(.cassetteDrive)
        userPortView.isHidden = !ports.contains(.userPort)
        switch machineSpecification.userPortModule.joystickPorts {
        case 0:
            userPortJoystick1View.isHidden = true
            userPortJoystick2View.isHidden = true
            
        case 1:
            userPortJoystick1View.isHidden = false
            userPortJoystick2View.isHidden = true

        default:
            userPortJoystick1View.isHidden = false
            userPortJoystick2View.isHidden = false
        }
    }
    
    private func update(partInfo: PartInfo) {
        partInfo.view.configure(from: machineSpecification.part(for: partInfo.key, machine: machine))
        partInfo.view.boldLabel = !machineSpecification.isDefault(key: partInfo.key)
    }
}

extension MachinePartView {
    func configure(from part: MachinePart?, prefix: String? = nil) {
        let fullPrefix: String
        if let prefix = prefix {
            fullPrefix = "\(prefix): "
        }
        else {
            fullPrefix = ""
        }
        
        if let part = part {
            image = part.icon
            label = "\(fullPrefix)\(part.name)"
        }
        else {
            image = nil
            label = "\(fullPrefix)none"
        }
    }
}

extension MachinePartsViewController {
    private func partInfo(for key: MachineConfig.Key) -> PartInfo? {
        return partInfos.first(where: { $0.key == key })
    }
    
    private func partInfo(for view: MachinePartView) -> PartInfo? {
        return partInfos.first(where: { $0.view == view })
    }
}
