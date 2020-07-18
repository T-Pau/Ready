/*
 LedView.swift -- Drive LED
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
    
    public func setColor(name: String) {
        guard let dark = UIColor(named: "LED \(name) Off"),
            let light = UIColor(named: "LED \(name) On") else { return }
        darkColor = dark
        lightColor = light
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        isOpaque = false
    }
    
    override public init(frame: CGRect) {
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
