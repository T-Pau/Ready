/*
 LightPenGestureRecognizer.swift -- Recognize Joystick Gestures
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

public class LightPenGestureRecognizer: UIGestureRecognizer {
    public var position: CGPoint? {
        return penTouch?.location(in: view)
    }
    public var button1: Bool {
        return buttonTouchesCount == 1 || penTouch?.force ?? 0 > 0.75
    }
    public var button2: Bool {
        return buttonTouchesCount == 2
    }
    
    private static var buttonTouchDelay = 0.1

    private var penTouch: UITouch?
    private var buttonTouches = Set<UITouch>()
    private var beginningButtonTouches = false
    
    private var buttonTouchesCount = 0
    
    override public func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        guard penTouch != nil || touches.count == 1 else { return }

        if penTouch == nil {
            penTouch = Array(touches)[0]
        }
        else if buttonTouches.isEmpty || beginningButtonTouches {
            buttonTouches = buttonTouches.union(touches)
            if !beginningButtonTouches {
                beginningButtonTouches = true
                DispatchQueue.main.asyncAfter(deadline: .now() + LightPenGestureRecognizer.buttonTouchDelay) {
                    guard self.beginningButtonTouches else { return }
                    self.buttonTouchesCount = self.buttonTouches.count
                    self.beginningButtonTouches = false
                    self.updateState()
                }
            }
        }

        updateState()
    }
    
    override public func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        var changed = false
        
        for touch in touches {
            if let touch = penTouch, touches.contains(touch) {
                penTouch = nil
                changed = true
            }
            else if buttonTouches.contains(touch) {
                buttonTouches.remove(touch)
                if buttonTouches.isEmpty {
                    buttonTouchesCount = 0
                    beginningButtonTouches = false
                    changed = true
                }
            }
        }
        
        if changed {
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
        if penTouch != nil || !buttonTouches.isEmpty {
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
}
