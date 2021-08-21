/*
 SelectDiskViewController.swift
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

class SelectDiskViewController: UIViewController {
    @IBOutlet weak var iconView: UIImageView!
    @IBOutlet weak var unitLabel: UILabel!
    @IBOutlet weak var titleLabel: UILabel!
    @IBOutlet weak var tableView: UITableView!
    @IBOutlet weak var subtitleLabel: UILabel!
    @IBOutlet weak var statusLabel: UILabel!
    
    var drive: DiskDrive?
    var unit: Int?
    var diskImages = [MediaItem]()
    var currentDiskImage: MediaItem?
    var status: String?
    
    var changeCallback: ((_: DiskImage?) -> ())?
    
    private var rowHeight: CGFloat = 55 // TODO: don't hardcode row height

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        let height = rowHeight * CGFloat(diskImages.count) + tableView.frame.minY
        
        preferredContentSize = CGSize(width: 430.0, height: min(height, 600.0))

        iconView.image = drive?.icon
        
        if let unit = unit {
            unitLabel.text = "Drive \(unit):"
        }
        else {
            unitLabel.text = nil
        }
        titleLabel.text = drive?.fullName
        if let variantName = drive?.variantName {
            subtitleLabel.text = "(\(variantName))"
        }
        else {
            subtitleLabel.text = nil
        }

        if let currentDiskImage = currentDiskImage,
            let index = diskImages.firstIndex(where: { $0.url == currentDiskImage.url }) {
            tableView.scrollToRow(at: IndexPath(row: index, section: 0), at: .middle, animated: false)
        }
        statusLabel.text = status
    }
}

extension SelectDiskViewController: UITableViewDelegate, UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return diskImages.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Disk", for: indexPath)
        
        if let cell = cell as? MediaTableViewCell {
            let diskImage = diskImages[indexPath.row]
            cell.mediaView?.item = diskImage
            cell.accessoryType = diskImage.url == currentDiskImage?.url ? .checkmark : .none
        }
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        changeCallback?(diskImages[indexPath.row] as? DiskImage)
        dismiss(animated: true)
    }
}
