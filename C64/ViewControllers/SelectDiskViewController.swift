/*
 SelectDiskViewController.swift
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
    var diskImages = [DiskImage]()
    var currentDiskImage: DiskImage?
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
            cell.mediaView?.item = diskImage as? MediaItem
            cell.accessoryType = diskImage.url == currentDiskImage?.url ? .checkmark : .none
        }
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        changeCallback?(diskImages[indexPath.row])
        dismiss(animated: true)
    }
}
