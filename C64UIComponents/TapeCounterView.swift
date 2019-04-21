/*
 TapeCounterView.swift -- Tape Counter
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

@IBDesignable public class TapeCounterView: UIView {
    public var font: UIFont = UIFont.systemFont(ofSize: 17) {
        didSet { updateSize() }
    }
    @IBInspectable public var textColor: UIColor = UIColor.black {
        didSet { setNeedsDisplay() }
    }
    @IBInspectable public var counterColor: UIColor = UIColor.clear
    @IBInspectable public var counter: Double = 0 {
        didSet { updatePosition() }
    }
    @IBInspectable public var digits: Int = 3 {
        didSet {
            updateSize()
            updatePosition()
        }
    }
    @IBInspectable public var lineSpacing: CGFloat = 1 {
        didSet { updateSize() }
    }
    @IBInspectable public var charSpacing: CGFloat = 1 {
        didSet { updateSize() }
    }

    private var digitSize = CGSize(width: 0, height: 0)
    private var counterSize = CGSize(width: 0, height: 0)
    
    private struct Position {
        var digit: Int
        var fraction: CGFloat
    }
    
    private var positions = [Position]()
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        isOpaque = false
        updateSize()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        isOpaque = false
        updateSize()
    }
    
    override public var intrinsicContentSize: CGSize {
        return CGSize(width: counterSize.width + 2, height: counterSize.height + 2)
    }
    
    override public func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }

        let counterRect = CGRect(x: bounds.midX - counterSize.width / 2, y: bounds.midY - counterSize.height / 2, width: counterSize.width, height: counterSize.height)
        
        context.setFillColor(counterColor.cgColor)
        context.fill(counterRect)
        
        context.clip(to: counterRect)

        var x = bounds.midX - (digitSize.width * (CGFloat(digits) - 1) * charSpacing + digitSize.width) / 2
        let y = bounds.midY - digitSize.height / 2
                
        for position in positions {
            let str = NSString(string: "\(position.digit)")
            var currentY = y - digitSize.height * position.fraction * lineSpacing
            str.draw(at: CGPoint(x: x, y: currentY), withAttributes: [.font: font, .foregroundColor: textColor])
            if (position.fraction != 0) {
                currentY += digitSize.height * lineSpacing
                let nextDigit = (position.digit + 1) % 10
                let str = NSString(string: "\(nextDigit)")
                str.draw(at: CGPoint(x: x, y: currentY), withAttributes: [.font: font, .foregroundColor: textColor])
            }
            x += digitSize.width * charSpacing
            
        }
        
        context.resetClip()
        context.setStrokeColor(UIColor.black.cgColor)
        context.setLineWidth(0.5)
        context.stroke(counterRect)
    }

    private func updatePosition() {
        var (_, fraction) = modf(counter)
        var integral = Int(counter)
        
        positions = [Position](repeating: Position(digit: 0, fraction: 0), count: digits)
        
        for place in (0 ..< digits).reversed() {
            let digit = integral % 10
            integral /= 10

            positions[place] = Position(digit: digit, fraction: CGFloat(fraction))

            if digit != 9 {
                fraction = 0
            }
        }
        
        setNeedsDisplay()
    }
    
    private func updateSize() {
        let str = NSString(stringLiteral: "0")
        
        digitSize = str.boundingRect(with: bounds.size, options: [], attributes: [.font: font], context: nil).size

        counterSize = CGSize(width: digitSize.width * CGFloat(digits) * charSpacing, height: digitSize.height * lineSpacing)
        invalidateIntrinsicContentSize()
        setNeedsDisplay()
    }
}
