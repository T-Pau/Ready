/*
 PaddleView.swift -- Virtual Paddle
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

public protocol PaddleViewDelegate {
    func paddleView(_ sender: PaddleView, changedPosition: Double)
    func paddleView(_ sender: PaddleView, changedButton: Bool)
}

public class PaddleView: UIView {
    public var delegate: PaddleViewDelegate?
    
    private var paddleGestureRecoginzer: PaddleGestureRecognizer!
    
    private var oldPosition = 0.5
    private var oldButton = false
    
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
        
        paddleGestureRecoginzer = PaddleGestureRecognizer(target: self, action: #selector(paddle(_:)))
        addGestureRecognizer(paddleGestureRecoginzer)
    }
    
    @objc func paddle(_ sender: PaddleGestureRecognizer) {
        let newPosition = paddleGestureRecoginzer.position
        let newButton = paddleGestureRecoginzer.isbuttonPressed
        
        if newPosition != oldPosition {
            delegate?.paddleView(self, changedPosition: newPosition)
            oldPosition = newPosition
        }
        
        if newButton != oldButton {
            delegate?.paddleView(self, changedButton: newButton)
            oldButton = newButton
        }
    }
}
