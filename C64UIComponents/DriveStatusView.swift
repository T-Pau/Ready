/*
 DriveStatusView.swift -- Disk Drive Status
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

@IBDesignable public class DriveStatusView: StatusView {
    public var unitLabel = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    public var trackView = DriveTrackView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    public var ledViews = [LedView]()

    @IBInspectable public var numberOfLeds: Int = 2
    @IBInspectable public var unit: Int = 8 {
        didSet {
            unitLabel.text = "\(unit)"
        }
    }
    
    override public var textColor: UIColor? {
        didSet {
            unitLabel.textColor = textColor
            trackView.color = textColor ?? UIColor.black
        }
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setupViews()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupViews()
    }
    
    private func setupViews() {
        stackView.spacing = 8
        
        unitLabel.text = "\(unit)"
        unitLabel.translatesAutoresizingMaskIntoConstraints = false
        unitLabel.font = UIFont.systemFont(ofSize: 17)
        stackView.addArrangedSubview(unitLabel)
        
        trackView.translatesAutoresizingMaskIntoConstraints = false
        stackView.addArrangedSubview(trackView)
        
        var constraints = [NSLayoutConstraint]()
        for _ in (0 ..< numberOfLeds) {
            let ledView = LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
            ledView.radiusFraction = 0.6
            ledView.translatesAutoresizingMaskIntoConstraints = false
            constraints.append(NSLayoutConstraint(item: ledView, attribute: .width, relatedBy: .equal, toItem: ledView, attribute: .height, multiplier: 1, constant: 1))
            stackView.addArrangedSubview(ledView)
            ledViews.append(ledView)
        }
        NSLayoutConstraint.activate(constraints)
    }
}
