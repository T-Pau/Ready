/*
 PaddleGestureRecognizer.swift -- Recognize Paddle Gestures
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

public class PaddleGestureRecognizer: UIGestureRecognizer {
    @IBInspectable public var paddleHeight: CGFloat = 128

    public var position = 0.5
    public var isbuttonPressed: Bool {
        return !buttonTouches.isEmpty
    }
    
    private var paddleTouch: UITouch?
    private var buttonTouches = Set<UITouch>()
    
    override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        guard let view = view else { return }
        
        for touch in touches {
            let location = touch.location(in: view)
            
            if location.y > view.bounds.height - paddleHeight && paddleTouch == nil {
                paddleTouch = touch
                updatePaddle()
            }
            else {
                buttonTouches.insert(touch)
            }
        }
        
        updateState()
    }
    
    override public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        for touch in touches {
            if touch == paddleTouch {
                paddleTouch = nil
            }
            else {
                buttonTouches.remove(touch)
            }
        }
        updateState()
    }
    
    override public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
        touchesEnded(touches, with: event)
    }
    
    override public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
        for touch in touches {
            if touch == paddleTouch {
                updatePaddle()
                updateState()
            }
        }
    }
    
    private func updateState() {
        if paddleTouch != nil || !buttonTouches.isEmpty {
            if state == .possible {
                state = .began
            }
            else {
                state = .changed
            }
        }
        else {
            state = .ended
        }
    }
    
    private func updatePaddle() {
        guard let touch = paddleTouch, let view = view else { return }
        
        position = Double(touch.location(in: view).x / view.bounds.width)
    }
}
