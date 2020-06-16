/*
 InputDevice.swift -- Physical Input Device
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

