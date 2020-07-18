/*
 MouseView.swift -- Virtual Mouse (Trackpad)
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
