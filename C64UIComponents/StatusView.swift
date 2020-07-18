/*
 StatusView.swift -- Top Level View for Status Bar Item
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

@IBDesignable open class StatusView: UIControl {
    public var stackView = UIStackView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    
    required public init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    @IBInspectable public var borderWidth: CGFloat = 1
    @IBInspectable public var borderColor: UIColor = UIColor.black
    @IBInspectable public var textColor: UIColor? = UIColor.black
    @IBInspectable public var xMarginWidth: CGFloat = 4
    @IBInspectable public var yMarginWidth: CGFloat = 0

    public override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    private func setup() {
        isOpaque = false
        stackView.translatesAutoresizingMaskIntoConstraints = false
        stackView.isOpaque = false
        
        addSubview(stackView)
        
        let views = [
            "view": stackView
        ]
        let metrics = [
            "xInset": borderWidth + xMarginWidth,
            "yInset": borderWidth + yMarginWidth
        ]
        
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "H:|-xInset-[view]-xInset-|", options: [], metrics: metrics, views: views))
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "V:|-yInset-[view]-yInset-|", options: [], metrics: metrics, views: views))
    }
    
    open override func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }
        
        context.setStrokeColor(borderColor.cgColor)
        context.setLineWidth(borderWidth)
        let border = bounds.insetBy(dx: borderWidth / 2, dy: borderWidth / 2)
        context.stroke(border)
    }
}
