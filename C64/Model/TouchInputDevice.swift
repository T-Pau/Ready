/*
 TouchInputDevice.swift -- Touch on iPad Screen Input Device
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
import C64UIComponents
import Emulator

class TouchInputDevice: InputDevice {
    var controllerView: VirtualControlsView
    
    init(view: VirtualControlsView) {
        controllerView = view
        
        super.init(identifier: "Touch", name: "Touch", iconName: "Touch", priority: MachinePartLowPriority, supportedModes: [ .joystick, .lightGun, .lightPen, .mouse, .paddle ])
        
        controllerView.mouseView.delegate = self
        controllerView.mouseView.sensitivity = 1.25
        controllerView.joystickView.delegate = self
        controllerView.lightPenView.delegate = self
        controllerView.paddleView.delegate = self
    }
    
    override var currentMode: Controller.InputType {
        get { return controllerView.currentMode }
        set { controllerView.currentMode = newValue }
    }
}

extension TouchInputDevice: MouseViewDelegate {
    func mouseView(_ sender: MouseView, pressed button: Int) {
        delegate?.inputDevice(self, mouseButtonPressed: button)
    }
    
    func mouseView(_ sender: MouseView, released button: Int) {
        delegate?.inputDevice(self, mouseButtonReleased: button)
    }
    
    func mouseView(_ sender: MouseView, moved distance: CGPoint) {
        delegate?.inputDevice(self, mouseMoved: distance)
    }
}

extension TouchInputDevice: JoystickViewDelegate {
    func joystickView(_ sender: JoystickView, changed buttons: JoystickButtons) {
        delegate?.inputDevice(self, joystickChanged: buttons)
    }
}

extension TouchInputDevice: LightPenViewDelegate {
    func lightPenView(_ sender: LightPenView, changed position: CGPoint?, size: CGSize, button1: Bool, button2: Bool) {
        delegate?.inputDevice(self, lightPenMoved: position, size: size, button1: button1, button2: button2)
    }
}

extension TouchInputDevice: PaddleViewDelegate {
    func paddleView(_ sender: PaddleView, changedPosition: Double) {
        let adjustedPosition = max(0, min((changedPosition - 0.5) * max(1.05, deviceConfig.sensitivity) + 0.5, 1))

        delegate?.inputDevice(self, paddleMoved: adjustedPosition)
    }
    
    func paddleView(_ sender: PaddleView, changedButton: Bool) {
        delegate?.inputDevice(self, paddleChangedButton: changedButton)
    }
}
