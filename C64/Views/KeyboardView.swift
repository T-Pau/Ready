/*
 KeyboardView.swift -- C64 Keyboard
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
import Emulator

protocol KeyboardViewDelegate {
    func pressed(key: Key)
    func released(key: Key)
}

class KeyboardView: UIView {
    var keyboardImageView = UIImageView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var shiftLockImageView = UIImageView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))

    var delegate: KeyboardViewDelegate?
    
    var keyboard: Keyboard? {
        didSet {
            updateLayout()
        }
    }
    
    var isShiftLockPressed = false { 
        didSet {
            shiftLockImageView.isHidden = !isShiftLockPressed
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    
    private func setup() {
        setup(view: keyboardImageView)
        setup(view: shiftLockImageView)
        shiftLockImageView.isHidden = true
    }
    
    private func setup(view: UIView) {
        addSubview(view)
        view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            NSLayoutConstraint(item: self, attribute: .left, relatedBy: .equal, toItem: view, attribute: .left, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .right, relatedBy: .equal, toItem: view, attribute: .right, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .top, relatedBy: .equal, toItem: view, attribute: .top, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .bottom, relatedBy: .equal, toItem: view, attribute: .bottom, multiplier: 1, constant: 0),
            ])
    }
    
    private struct Span {
        var left: CGFloat
        var right: CGFloat
        var keys: [Key]
        
        func hit(_ point: CGPoint) -> Key? {
            guard (point.x >= left && point.x < right) else { return nil }
            let idx = Int((point.x - left) * CGFloat(keys.count) / (right - left))
            return keys[idx]
        }
    }
    private struct Row {
        var top: CGFloat
        var bottom: CGFloat
        var spans: [Span]
        
        func hit(_ point: CGPoint) -> Key? {
            guard (point.y >= top && point.y < bottom) else { return nil }
            for span in spans {
                if let key = span.hit(point) {
                    return key
                }
            }
            
            return nil
        }
    }
    private struct Layout {
        var rows: [Row]
        
        func hit(_ point: CGPoint) -> Key? {
            for row in rows {
                if let key = row.hit(point) {
                    return key
                }
            }
            
            return nil
        }
    }
    
    private var layout = Layout(rows: [])
    
    func updateLayout() {
        guard let keyboard = keyboard else {
            layout = Layout(rows: [])
            return
        }
        keyboardImageView.image = UIImage(named: keyboard.imageName)
        shiftLockImageView.image = UIImage(named: keyboard.imageName + " ShiftLock")
        
        layout = Layout(rows: [
            Row(top: CGFloat(keyboard.rows[0]), bottom: CGFloat(keyboard.rows[1]), spans: [
                Span(left: keyboard.topHalfLeft, right: keyboard.topHalfRight, keys: [
                    .ArrowLeft, .Char("1"), .Char("2"), .Char("3"), .Char("4"), .Char("5"), .Char("6"), .Char("7"), .Char("8"), .Char("9"), .Char("0"), .Char("+"), .Char("-"), .Char("£"), .ClearHome, .InsertDelete
                ]),
                Span(left: CGFloat(keyboard.functionKeysLeft), right: CGFloat(keyboard.functionKeysRight), keys: [.F1])
            ]),
            Row(top: CGFloat(keyboard.rows[1]), bottom: CGFloat(keyboard.rows[2]), spans: [
                Span(left: keyboard.topHalfLeft, right: keyboard.ctrlRight, keys: [.Control]),
                Span(left: keyboard.ctrlRight, right: keyboard.restoreLeft, keys: [
                    .Char("q"), .Char("w"), .Char("e"), .Char("r"), .Char("t"), .Char("y"), .Char("u"), .Char("i"), .Char("o"), .Char("p"), .Char("@"), .Char("*"), .ArrowUp
                    ]),
                Span(left: keyboard.restoreLeft, right: keyboard.topHalfRight, keys: [.Restore]),
                Span(left: CGFloat(keyboard.functionKeysLeft), right: CGFloat(keyboard.functionKeysRight), keys: [.F3])
            ]),
            Row(top: CGFloat(keyboard.rows[2]), bottom: CGFloat(keyboard.rows[3]), spans: [
                Span(left: keyboard.bottomHalfLeft, right: keyboard.returnLeft, keys: [
                    .RunStop, .ShiftLock, .Char("a"), .Char("s"), .Char("d"), .Char("f"), .Char("g"), .Char("h"), .Char("j"), .Char("k"), .Char("l"), .Char(":"), .Char(";"), .Char("=")
                    ]),
                Span(left: keyboard.returnLeft, right: keyboard.bottomHalfRight, keys: [.Return]),
                Span(left: CGFloat(keyboard.functionKeysLeft), right: CGFloat(keyboard.functionKeysRight), keys: [.F5])
            ]),
            Row(top: CGFloat(keyboard.rows[3]), bottom: CGFloat(keyboard.rows[4]), spans: [
                Span(left: keyboard.bottomHalfLeft, right: keyboard.leftShiftLeft, keys: [.Commodore]),
                Span(left: keyboard.leftShiftLeft, right: keyboard.leftShiftRight, keys: [.ShiftLeft]),
                Span(left: keyboard.leftShiftRight, right: keyboard.rightShiftLeft, keys: [
                    .Char("z"), .Char("x"), .Char("c"), .Char("v"), .Char("b"), .Char("n"), .Char("m"), .Char(","), .Char("."), .Char("/"),
                    ]),
                Span(left: keyboard.rightShiftLeft, right: keyboard.rightShiftRight, keys: [.ShiftRight]),
                Span(left: keyboard.rightShiftRight, right: keyboard.bottomHalfRight, keys: [.CursorUpDown, .CursorLeftRight]),
                Span(left: CGFloat(keyboard.functionKeysLeft), right: CGFloat(keyboard.functionKeysRight), keys: [.F7])
                ]),
            Row(top: CGFloat(keyboard.rows[4]), bottom: CGFloat(keyboard.rows[5]), spans: [
                Span(left: CGFloat(keyboard.spaceLeft), right: CGFloat(keyboard.spaceRight), keys: [.Char(" ")])
            ])
        ])
    }
        
    private var keyTouches = [UITouch : Key]()

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        var unhandledTouches = Set<UITouch>()
        
        for touch in touches {
            var location = touch.location(in: self)
            location.x *= (keyboardImageView.image?.size.width ?? 0) / frame.width
            location.y *= (keyboardImageView.image?.size.height ?? 0) / frame.height
            
            if let key = layout.hit(location) {
                keyTouches[touch] = key
                delegate?.pressed(key: key)
            }
            else {
                unhandledTouches.insert(touch)
            }
        }
        
        if !unhandledTouches.isEmpty {
            super.touchesBegan(unhandledTouches, with: event)
        }
    }
    
    private func touchesReleased(_ touches: Set<UITouch>) -> Set<UITouch> {
        var unhandledTouches = Set<UITouch>()

        for touch in touches {
            if let key = keyTouches[touch] {
                keyTouches.removeValue(forKey: touch)
                delegate?.released(key: key)
            }
            else {
                unhandledTouches.insert(touch)
            }
        }
        
        return unhandledTouches
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        let unhandledTouches = touchesReleased(touches)
        
        if !unhandledTouches.isEmpty {
            super.touchesEnded(unhandledTouches, with: event)
        }
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        let unhandledTouches = touchesReleased(touches)
        
        if !unhandledTouches.isEmpty {
            super.touchesCancelled(unhandledTouches, with: event)
        }
    }
    
    override func touchesEstimatedPropertiesUpdated(_ touches: Set<UITouch>) {
        var unhandledTouches = Set<UITouch>()

        for touch in touches {
            if let _ = keyTouches[touch] {
            }
            else {
                unhandledTouches.insert(touch)
            }
        }

        if !unhandledTouches.isEmpty {
            super.touchesEstimatedPropertiesUpdated(unhandledTouches)
        }
    }
    
    @IBAction func tapped(_ sender: Any) {
        print("keyboard tapped")
    }
    
    /*
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        // Drawing code
    }
    */

}
