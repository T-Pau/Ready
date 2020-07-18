/*
 JoystickGestureRecognizer.swift -- Recognize Light Pen Gestures
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

import Emulator

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
