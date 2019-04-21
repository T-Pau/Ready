/*
 VirtualControlsView.swift
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

class VirtualControlsView: UIView {
    var currentMode = Controller.InputType.none {
        didSet {
            view(for: oldValue)?.isHidden = true
            view(for: currentMode)?.isHidden = false
        }
    }
    var joystickView = JoystickView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var mouseView = MouseView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))

    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        
        setup()
    }

    override init(frame: CGRect) {
        super.init(frame: frame)
        
        setup()
    }
    
    private func setup() {
        setup(view: joystickView)
        setup(view: mouseView)
    }
    
    private func setup(view: UIView) {
        addSubview(view)
        view.isHidden = true
        view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            NSLayoutConstraint(item: self, attribute: .left, relatedBy: .equal, toItem: view, attribute: .left, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .right, relatedBy: .equal, toItem: view, attribute: .right, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .top, relatedBy: .equal, toItem: view, attribute: .top, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .bottom, relatedBy: .equal, toItem: view, attribute: .bottom, multiplier: 1, constant: 0),
        ])
    }
    
    private func view(for mode: Controller.InputType) -> UIView? {
        switch mode {
        case .none:
            return nil
            
        case .joystick:
            return joystickView
            
        case .lightGun, .lightPen:
            return nil

        case .mouse:
            return mouseView
            
        case .paddle:
            return nil
        }
    }
}
