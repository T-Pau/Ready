/*
 StatusView.swift -- Top Level View for Status Bar Item
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
