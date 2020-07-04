/*
 SingularAdapterStatusView.swift -- Singular 4-Player Adapter Status
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

@IBDesignable class SingularAdapterStatusView: StatusView {
    public var mode = MachineConfigOld.SingularAdapterMode.cga {
        didSet {
            let onIndex: Int?
            switch mode {
            case .cga:
                onIndex = 0
            case .hs:
                onIndex = 1
            case .bba:
                onIndex = 2
            case .off:
                onIndex = nil
            }
            
            for index in (0 ..< ledViews.count) {
                ledViews[index].intensity = index == onIndex ? 1 : 0
            }
        }
    }
    
    public var radiusFraction = CGFloat(0.5)
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setupViews()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupViews()
    }
    
    private var ledViews = [
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0)),
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0)),
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    ]
    
    private func setupViews() {
        backgroundColor = UIColor(named: "Singular Adapter Case")
        stackView.spacing = 12
        
        ledViews[0].setColor(name: "Singular Adapter CGA")
        ledViews[1].setColor(name: "Singular Adapter H&S")
        ledViews[2].setColor(name: "Singular Adapter BBA")
        
        var constraints = [NSLayoutConstraint]()
        for ledView in ledViews {
            ledView.isRound = true
            ledView.radiusFraction = 1
            ledView.translatesAutoresizingMaskIntoConstraints = false
            constraints.append(NSLayoutConstraint(item: ledView, attribute: .width, relatedBy: .equal, toItem: ledView, attribute: .height, multiplier: radiusFraction, constant: 1))
            stackView.addArrangedSubview(ledView)
        }
        
        let contentHuggingConstraint = NSLayoutConstraint(item: self, attribute: .width, relatedBy: .lessThanOrEqual, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: 8)
        contentHuggingConstraint.priority = .defaultLow
        constraints.append(contentHuggingConstraint)
        
        NSLayoutConstraint.activate(constraints)
    }
}
