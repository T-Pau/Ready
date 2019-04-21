/*
 MouseView.swift -- Virtual Mouse (Trackpad)
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

@objc public protocol MouseViewDelegate {
    func mouseView(_ sender: MouseView, pressed button: Int)
    func mouseView(_ sender: MouseView, released button: Int)
    func mouseView(_ sender: MouseView, moved distance: CGPoint)
}

public class MouseView: UIView {
    @IBOutlet public var delegate: MouseViewDelegate?
    @IBInspectable public var sensitivity: CGFloat = 1.0
    
    private var mouseRecognizer: MouseGestureRecognizer!

    private var wasButtonPressed = [ false, false ]

    public required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    
    public override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }

    private func setup() {
        isUserInteractionEnabled = true
        isMultipleTouchEnabled = true
        
        mouseRecognizer = MouseGestureRecognizer(target: self, action: #selector(mouse(_:)))
        addGestureRecognizer(mouseRecognizer)
    }

    @objc func mouse(_ sender: MouseGestureRecognizer) {
        forward(button: 1, state: sender.leftButton)
        forward(button: 2, state: sender.rightButton)
        sender.clearClicked()

        let distance = sender.velocity
        if (distance.x != 0 || distance.y != 0) {
            delegate?.mouseView(self, moved: distance)
        }
    }
    
    private func forward(button: Int, state: MouseGestureRecognizer.ButtonState) {
        switch state {
        case .clicked:
            clicked(button)
            
        case .pressed:
            if !wasButtonPressed[button - 1] {
                delegate?.mouseView(self, pressed: button)
                wasButtonPressed[button - 1] = true
            }
            
        case .released:
            if wasButtonPressed[button - 1] {
                delegate?.mouseView(self, released: button)
                wasButtonPressed[button - 1] = false
            }
        }
    }

    private func clicked(_ button: Int) {
        delegate?.mouseView(self, pressed: button)
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + .milliseconds(40)) {
            self.delegate?.mouseView(self, released: button)
        }
    }
}
