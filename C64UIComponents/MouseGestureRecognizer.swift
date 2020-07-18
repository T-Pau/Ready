/*
 MouseGestureRecognizer.swift -- Recognize Trackpad Gestures
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

public class MouseGestureRecognizer: UIGestureRecognizer {
    public enum ButtonState {
        case clicked
        case pressed
        case released
    }
    public var position = CGPoint(x: 0, y: 0)
    public var velocity = CGPoint(x: 0, y: 0)
    public var leftButton = ButtonState.released {
        didSet {
            changed = true
        }
    }
    public var rightButton = ButtonState.released {
        didSet {
            changed = true
        }
    }

    private struct Finger: Hashable {
        var touch: UITouch
        var startLocation: CGPoint
        var startTime: Date
        
        init(touch: UITouch) {
            self.touch = touch
            startLocation = touch.location(in: nil)
            self.startTime = Date()
        }
        
        func hash(into hasher: inout Hasher) {
            hasher.combine(touch)
        }
    }
    
    private var tapFingers = Set<Finger>() // Touches that can be taps
    private var dragFingers = Set<Finger>() // Touches that are drags
    private var dragCount = 0 // logical number of touches in current drag

    private var released = Set<Date>() // recently released tap touches (for collating multi-finger taps)
    
    private var changed = false
    
    private static let tapTimeout = 0.2
    
    public func clearClicked() {
        if leftButton == .clicked {
            leftButton = .released
        }
        if rightButton == .clicked {
            rightButton = .released
        }
    }
    
    private func updateTouches() {
        velocity = CGPoint(x: 0, y: 0)
        
        for finger in tapFingers {
            if -finger.startTime.timeIntervalSinceNow > MouseGestureRecognizer.tapTimeout {
                dragFingers.insert(finger)
                tapFingers.remove(finger)
            }
        }
        for date in released {
            if -date.timeIntervalSinceNow > MouseGestureRecognizer.tapTimeout {
                released.remove(date)
            }
        }
        
        changed = false
    }
    
    override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        updateTouches()
        
        for touch in touches {
            tapFingers.insert(Finger(touch: touch))
        }
        
        if dragCount == 0 && tapFingers.count == 3 && dragFingers.isEmpty {
            dragFingers = tapFingers
            tapFingers.removeAll()
            leftButton = .pressed
        }

        updateState()
    }
    
    override public func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
        updateTouches()
        
        if dragCount == 0 {
            dragFingers.formUnion(tapFingers)
            tapFingers.removeAll()
            
            dragCount = dragFingers.count
            
            switch dragCount {
            case 2:
                rightButton = .pressed
                
            case 3:
                leftButton = .pressed
                
            default:
                break
            }
            NSLog("\(dragCount) finger drag began")
        }
        else {
            for touch in touches {
                if let finger = tapFingers.first(where: { $0.touch == touch }) {
                    // TODO: only if moved enough?
                    NSLog("tap finger moved")
                    tapFingers.remove(finger)
                    dragFingers.insert(finger)
                }
            }
        }
        
        if dragCount > 0 && dragCount < 4 {
            velocity = CGPoint(x: 0, y: 0)
            for touch in touches {
                let location = touch.location(in: nil)
                let previousLocation = touch.previousLocation(in: nil)
                
                velocity.x += location.x - previousLocation.x
                velocity.y += location.y - previousLocation.y
            }
            velocity.x /= CGFloat(touches.count)
            velocity.y /= CGFloat(touches.count)
            
            if velocity.x != 0 && velocity.y != 0 {
                position.x += velocity.x
                position.y += velocity.y
                
                changed = true
            }
        }
        
        updateState()
    }
    
    override public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        updateTouches()

        for finger in tapFingers {
            if touches.contains(finger.touch) {
                tapFingers.remove(finger)
                released.insert(finger.startTime)
            }
        }

        if tapFingers.isEmpty {
            switch released.count {
            case 1:
                if leftButton == .released {
                    leftButton = .clicked
                }
                
            case 2:
                if rightButton == .released {
                    rightButton = .clicked
                }

            default:
                break
            }
            released.removeAll()
        }

        for finger in dragFingers {
            if touches.contains(finger.touch) {
                dragFingers.remove(finger)
            }
        }
        
        if dragFingers.isEmpty {
            switch dragCount {
            case 2:
                rightButton = .released
            case 3:
                leftButton = .released
            default:
                break
            }
            dragCount = 0
        }
        
        updateState()
    }
    
    override public func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
        touchesEnded(touches, with: event)
    }
    
    private func updateState() {
        if tapFingers.isEmpty && dragFingers.isEmpty {
            state = .ended
        }
        else if state == .possible {
            state = .began
        }
        else if changed {
            state = .changed
        }
        
        changed = false
    }
}
