/*
 DiskDirectoryTableViewCell.swift
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

class DiskDirectoryTableViewCell: UITableViewCell {
    @IBOutlet weak var lineImageView: UIImageView!
    var aspectConstraint: NSLayoutConstraint?
    
    override func awakeFromNib() {
        super.awakeFromNib()
        lineImageView.layer.magnificationFilter = .nearest
    }
    
    var lineImage: UIImage? {
        get {
            return lineImageView.image
        }
        set {
            lineImageView.image = newValue
            if let constraint = aspectConstraint {
                NSLayoutConstraint.deactivate([constraint])
                aspectConstraint = nil
            }
            if let image = newValue {
                if let view = lineImageView {
                    let constraint = NSLayoutConstraint(item: view, attribute: .width, relatedBy: .equal, toItem: view, attribute: .height, multiplier: image.size.width / image.size.height, constant: 1)
                    NSLayoutConstraint.activate([constraint])
                    aspectConstraint = constraint
                }
            }
        }
    }
}
