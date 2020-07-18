/*
 InputDevice.swift -- Physical Input Device
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

public protocol InputDeviceDelegate {
    func inputDeviceDidDisconnect(_ inputDevice: InputDevice)
    
    func inputDevice(_ device: InputDevice, joystickChanged buttons: JoystickButtons)
    
    func inputDevice(_ device: InputDevice, mouseMoved distance: CGPoint)
    func inputDevice(_ device: InputDevice, mouseButtonPressed: Int)
    func inputDevice(_ device: InputDevice, mouseButtonReleased: Int)
    
    func inputDevice(_ device: InputDevice, lightPenMoved position: CGPoint?, size: CGSize, button1: Bool, button2: Bool)
    
    func inputDevice(_ device: InputDevice, paddleMoved position: Double)
    func inputDevice(_ device: InputDevice, paddleChangedButton: Bool)
}

open class InputDevice: MachinePart {
    open var identifier: String
    open var name: String
    open var fullName: String
    open var icon: UIImage?
    open var priority: Int
    open var playerIndex: Int?
    open var connector: ConnectorType { .none }
    
    open var supportedModes: Set<Controller.InputType>

    open var deviceConfig = Controller.DeviceConfig()
    open var currentMode = Controller.InputType.none

    public var delegate: InputDeviceDelegate?

    public init(identifier: String, name: String, fullName: String? = nil, iconName: String? = nil, priority: Int = MachinePartNormalPriority, supportedModes: Set<Controller.InputType>) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        if let iconName = iconName {
            icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.supportedModes = supportedModes
    }
    
    open func supports(inputType: Controller.InputType) -> Bool {
        return supportedModes.contains(inputType)
    }
    
    open func disconnect() {
        delegate?.inputDeviceDidDisconnect(self)
    }
}

extension InputDevice: Hashable {
    public static func == (lhs: InputDevice, rhs: InputDevice) -> Bool {
        return ObjectIdentifier(lhs) == ObjectIdentifier(rhs)
    }
    
    public func hash(into hasher: inout Hasher) {
        ObjectIdentifier(self).hash(into: &hasher)
    }
}

