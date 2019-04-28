/*
 LightPenGestureRecognizer.swift -- Recognize Joystick Gestures
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

public class LightPenGestureRecognizer: UIGestureRecognizer {
    public var position: CGPoint? {
        return penTouch?.location(in: view)
    }

    private var penTouch: UITouch?
    
    override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        guard penTouch == nil else { return }
        guard touches.count == 1 else { return }
        
        penTouch = Array(touches)[0]
        
        updateState()
    }
    
    override public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        guard let touch = penTouch else { return }
        
        if touches.contains(touch) {
            penTouch = nil
            updateState()
        }
        
    }
    
    override public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
        touchesEnded(touches, with: event)
    }
    
    override public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
        guard let touch = penTouch else { return }
        
        if touches.contains(touch) {
            updateState()
        }
    }
    
    private func updateState() {
        if penTouch != nil {
            if state == .possible {
                state = .began
            }
            state = .changed
        }
        else {
            state = .ended
        }
    }
}
