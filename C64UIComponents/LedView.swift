/*
 LedView.swift -- Drive LED
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

@IBDesignable public class LedView: UIView {
    @IBInspectable public var isRound = false
    @IBInspectable public var darkColor = UIColor.black
    @IBInspectable public var lightColor = UIColor.white
    @IBInspectable public var radiusFraction: CGFloat = 0.8

    @IBInspectable public var intensity = 0.0 {
        didSet {
            if intensity != oldValue {
                setNeedsDisplay()
            }
        }
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        isOpaque = false
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        isOpaque = false
    }
    
    override public func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }
        
        let color = blend(darkColor, lightColor, CGFloat(intensity))
        
        context.setFillColor(color.cgColor)
        context.setStrokeColor(UIColor.black.cgColor)
        context.setLineWidth(0.5)

        if isRound {
            let radius = min(bounds.width, bounds.height) * radiusFraction
            let circle = CGRect(x: (bounds.width - radius) / 2, y: (bounds.height - radius) / 2, width: radius, height: radius)
            
            context.fillEllipse(in: circle)
            context.strokeEllipse(in: circle)
        }
        else {
            let height = bounds.width / 3
            let y = bounds.midY - height / 2
            
            context.fill(CGRect(x: bounds.minX, y: y, width: bounds.width, height: height))
            context.stroke(CGRect(x: bounds.minX - 0.25, y: y - 0.25, width: bounds.width + 0.5, height: height + 0.5))
        }
    }
    
    private func blend(_ a: UIColor, _ b: UIColor, _ factor: CGFloat) -> UIColor {
        var (r1, g1, b1, a1): (CGFloat, CGFloat, CGFloat, CGFloat) = (0, 0, 0, 0)
        var (r2, g2, b2, a2): (CGFloat, CGFloat, CGFloat, CGFloat) = (0, 0, 0, 0)
        
        a.getRed(&r1, green: &g1, blue: &b1, alpha: &a1)
        b.getRed(&r2, green: &g2, blue: &b2, alpha: &a2)
        
        return UIColor(red: r1 * (1 - factor) + r2 * factor, green: g1 * (1 - factor) + g2 * factor, blue: b1 * (1 - factor) + b2 * factor, alpha: a1 * (1 - factor) + a2 * factor)
    }
}
