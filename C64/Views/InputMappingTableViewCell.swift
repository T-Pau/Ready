/*
 InputMappingTableViewCell.swift
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
