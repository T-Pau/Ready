/*
 VirtualControlsView.swift
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

class VirtualControlsView: UIView {
    @IBInspectable var topOffset: CGFloat = 0 {
        didSet {
            lightPenView.topOffset = topOffset
        }
    }
    
    var currentMode = Controller.InputType.none {
        didSet {
            view(for: oldValue)?.isHidden = true
            view(for: currentMode)?.isHidden = false
        }
    }
    var joystickView = JoystickView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var mouseView = MouseView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var lightPenView = LightPenView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var paddleView = PaddleView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))

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
        setup(view: lightPenView)
        lightPenView.topOffset = topOffset
        setup(view: paddleView)
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
            return lightPenView
            
        case .mouse:
            return mouseView
            
        case .paddle:
            return paddleView
        }
    }
}
