/*
 JoystickGestureRecognizer.swift -- Recognize Joystick Gestures
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

public struct JoystickButtons: Equatable {
    public var up: Bool
    public var down: Bool
    public var left: Bool
    public var right: Bool
    public var fire: Bool
    
    public init() {
        up = false
        down = false
        left = false
        right = false
        fire = false
    }
}

public class JoystickGestureRecognizer: UIGestureRecognizer {
    @IBInspectable public var buttonWidthRatio: CGFloat = 0.3
    @IBInspectable public var directionThreshold: CGFloat = 20

    public var previousButtons = JoystickButtons()
    public var currentButtons = JoystickButtons()
    private var newButtons = JoystickButtons()

    private var center: CGPoint?
    private var directionTouches = Set<UITouch>()
    private var buttonTouches = Set<UITouch>()
    
    override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        guard let view = view else { return }
        
        newButtons = currentButtons
        
        for touch in touches {
            let location = touch.location(in: view)
            if location.x > view.bounds.width * (1 - buttonWidthRatio) {
                buttonTouches.insert(touch)
            }
            else {
                directionTouches.insert(touch)
                if directionTouches.count == 1 {
                    center = location
                }
            }
        }
        
        if directionTouches.count != 1 {
            center = nil
        }
        
        updateState()
    }
    
    override public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        for touch in touches {
            if buttonTouches.contains(touch) {
                buttonTouches.remove(touch)
            }
            else if directionTouches.contains(touch) {
                directionTouches.remove(touch)
            }
        }
        
        if directionTouches.count != 1 {
            newButtons = JoystickButtons()
            center = nil
        }

        updateState()
    }
    
    override public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
        touchesEnded(touches, with: event)
    }
    
    override public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
        guard let view = view else { return }
        guard var newCenter = center else { return }
        guard directionTouches.count == 1 else { return }
        
        newButtons = JoystickButtons()
        
        for touch in touches {
            if directionTouches.contains(touch) {
                let location = touch.location(in: view)
                
                if location.x < newCenter.x - directionThreshold {
                    newButtons.left = true
                    newCenter.x = min(newCenter.x, location.x + 2 * directionThreshold)
                }
                else if location.x > newCenter.x + directionThreshold {
                    newButtons.right = true
                    newCenter.x = max(newCenter.x, location.x - 2 * directionThreshold)
                }

                if location.y < newCenter.y - directionThreshold {
                    newButtons.up = true
                    newCenter.y = min(newCenter.y, location.y + 2 * directionThreshold)
                }
                else if location.y > newCenter.y + directionThreshold {
                    newButtons.down = true
                    newCenter.y = max(newCenter.y, location.y - 2 * directionThreshold)
                }

                center = newCenter
                updateState()
            }
        }
    }
    
    private func updateState() {
        newButtons.fire = buttonTouches.count == 1

        if newButtons != currentButtons  || state == .possible {
            previousButtons = currentButtons
            currentButtons = newButtons
            
            if state == .possible {
                state = .began
            }
            else if buttonTouches.isEmpty && directionTouches.isEmpty {
                state = .ended
            }
            else {
                state = .changed
            }
        }
    }
}
