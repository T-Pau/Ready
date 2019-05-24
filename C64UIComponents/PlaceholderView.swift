/*
 PlaceholderView.swift
 Copyright (C) 2019 Dieter Baron
 
 The author can be contacted at <dillo@nih.at>
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.
 
 3. The names of the authors may not be used to endorse or promote
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

@IBDesignable public class PlaceholderView: UIView {
    @IBInspectable public var color: UIColor = UIColor.black {
        didSet {
            label.textColor = color
            setNeedsDisplay()
        }
    }
    @IBInspectable public var text: String? {
        get {
            return label.text
        }
        set {
            label.text = newValue
            if let str = newValue as NSString? {
                var lines = 0
                str.enumerateLines { _,_ in
                    lines += 1
                }
                label.numberOfLines = lines
            }
            else {
                label.numberOfLines = 1
            }
            setNeedsDisplay()
        }
    }
    @IBInspectable public var frameWidth: CGFloat = 1
    @IBInspectable public var cornerRadius: CGFloat = 8
    @IBInspectable public var margin: CGFloat = 20

    private var label = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    
    public override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    public required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    
    private func setup() {
        isOpaque = false
        label.translatesAutoresizingMaskIntoConstraints = false
        label.textAlignment = .center
        addSubview(label)
        NSLayoutConstraint.activate([
            NSLayoutConstraint(item: label, attribute: .centerX, relatedBy: .equal, toItem: self, attribute: .centerX, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: label, attribute: .centerY, relatedBy: .equal, toItem: self, attribute: .centerY, multiplier: 1, constant: 0)
        ])
    }

    override public func draw(_ rect: CGRect) {
        guard frameWidth > 0, let context = UIGraphicsGetCurrentContext() else { return }

        let width = label.intrinsicContentSize.width + margin * 2
        let height = label.intrinsicContentSize.height + margin * 2

        let dashRect = CGRect(x: (bounds.width - width) / 2, y: (bounds.height - height) / 2, width: width, height: height)
        
        context.setStrokeColor(color.cgColor)
        context.setLineWidth(frameWidth)
        context.setLineDash(phase: 0, lengths: [10, 5])
        let roundedRect = UIBezierPath.init(roundedRect: dashRect, cornerRadius: cornerRadius)
        roundedRect.stroke()
    }
}
