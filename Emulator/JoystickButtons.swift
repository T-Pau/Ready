/*
 JoystickButtons.swift -- Buttons a Joystick Can Have
 Copyright (C) 2020 Dieter Baron
 
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

import Foundation

public struct JoystickButtons: Equatable {
    public var up: Bool
    public var down: Bool
    public var left: Bool
    public var right: Bool
    public var fire: Bool
    public var fire2: Bool
    public var fire3: Bool

    public var a: Bool
    public var b: Bool {
        get { return fire }
        set { fire = newValue }
    }
    public var x: Bool {
        get { return fire3 }
        set { fire3 = newValue }
    }
    public var y: Bool {
        get { return fire2 }
        set { fire2 = newValue }
    }
    public var l: Bool
    public var r: Bool
    public var select: Bool
    public var start: Bool
    
    public init() {
        up = false
        down = false
        left = false
        right = false
        fire = false
        fire2 = false
        fire3 = false
        a = false
        l = false
        r = false
        select = false
        start = false
    }
    
    public func toString() -> String {
        var pressed = [String]()
        
        if (up) {
            pressed.append("up")
        }
        if (down) {
            pressed.append("down")
        }
        if (left) {
            pressed.append("left")
        }
        if (right) {
            pressed.append("right")
        }

        if (a) {
            pressed.append("a")
        }
        if (b) {
            pressed.append("b")
        }
        if (x) {
            pressed.append("x")
        }
        if (y) {
            pressed.append("y")
        }
        if (l) {
            pressed.append("l")
        }
        if (r) {
            pressed.append("r")
        }

        if (select) {
            pressed.append("select")
        }
        if (start) {
            pressed.append("start")
        }
        
        return pressed.joined(separator: ", ")
    }
}
