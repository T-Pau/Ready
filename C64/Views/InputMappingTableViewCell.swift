/*
 InputMappingTableViewCell.swift
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

class InputMappingTableViewCell: UITableViewCell {
    @IBOutlet weak var portTitleLabel: UILabel!
    @IBOutlet weak var portSubtitleLabel: UILabel!
    @IBOutlet weak var portIcon: UIImageView!
    @IBOutlet weak var deviceTitleLabel: UILabel!
    @IBOutlet weak var deviceSubtitleLabel: UILabel!
    @IBOutlet weak var deviceIcon: UIImageView!
    
    override func awakeFromNib() {
        super.awakeFromNib()
        // Initialization code
    }

    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)

        // Configure the view for the selected state
    }

}

extension InputMappingTableViewCell {
    func configure(from inputPairing: InputPairing) {
        configure(fromPort: inputPairing.port)
        configure(fromDevice: inputPairing.device)
    }

    func configure(fromPort port: InputPort?) {
        if let port = port {
            portTitleLabel.text = port.name
            portSubtitleLabel.text = port.controller.name
            portIcon.image = port.icon
        }
        else {
            portTitleLabel.text = nil
            portSubtitleLabel.text = nil
            portIcon.image = nil
        }
    }
    
    func configure(fromDevice device: InputDevice?) {
        if let device = device {
            deviceTitleLabel.text = device.name
            deviceSubtitleLabel.text = "\(device.currentMode)"
            deviceIcon.image = device.icon
        }
        else {
            deviceTitleLabel.text = nil
            deviceSubtitleLabel.text = nil
            deviceIcon.image = nil
        }
    }
}
