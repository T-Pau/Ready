/*
 MfiInputDevice.swift -- MfI Controller
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

import Foundation
import C64UIComponents

class MfiInputDevice: InputDevice {
    var controller: GCController?
    
    override var playerIndex: Int? {
        didSet {
            if let index = playerIndex {
                controller?.playerIndex = GCControllerPlayerIndex(rawValue: index - 1) ?? .index1
            }
            else {
                controller?.playerIndex = .indexUnset
            }
        }
    }
    
    static var deadZone = 0.1
    static var coneAngle = 67.5
    
    init(controller: GCController) {
        self.controller = controller
        super.init(identifier: "\(ObjectIdentifier(controller))", name: "", supportedModes: [ .joystick ])
        fullName = controller.vendorName ?? "MfI Controller"
        if let vendorName = controller.vendorName, let product = MfiInputDevice.products[vendorName] {
            name = product.name
            if !product.iconNames.isEmpty {
                icon = UIImage(named: product.iconNames[0])
            }
        }
        else {
            name = "Controller" // TODO
        }
        if icon == nil {
            icon = UIImage(named: "MfI Controller")
        }
        
        if let gamepad = controller.extendedGamepad {
            gamepad.valueChangedHandler = { gamepad, _ in
                var buttons = JoystickButtons()

                buttons.update(dpad: gamepad.dpad)
                buttons.update(thumbstick: gamepad.leftThumbstick)
                buttons.fire = gamepad.buttonA.isPressed
                
                self.update(buttons: buttons)
            }
        }
        else if let gamepad = controller.microGamepad {
            gamepad.valueChangedHandler = { gamepad, _ in
                var buttons = JoystickButtons()
                
                buttons.update(dpad: gamepad.dpad)
                buttons.fire = gamepad.buttonA.isPressed
                
                self.update(buttons: buttons)
            }
        }
    }
    
    override func disconnect() {
        controller = nil
        super.disconnect()
    }
    
    private var previousButtons = JoystickButtons()
    
    private func update(buttons: JoystickButtons) {
        if buttons != previousButtons {
            previousButtons = buttons
            delegate?.inputDevice(self, joystickChanged: buttons)
        }
    }

    private struct Product {
        var name: String
        var iconNames: [String]
    }
    
    private static let products = [
        "Mad Catz C.T.R.L.i": Product(name: "C.T.R.L.i", iconNames: [
            "Mad Catz C.T.R.L.i Black",
            "Mad Catz C.T.R.L.i Blue",
            "Mad Catz C.T.R.L.i Red"
        ]),
        "Nimbus" : Product(name: "Nimubs", iconNames: [ "Steel Series Nimbus" ]),
        "Steel Series Stratus" : Product(name: "Stratus", iconNames: [ "Steel Series Stratus White" ])
    ]
}

extension JoystickButtons {
    mutating func update(dpad: GCControllerDirectionPad) {
        up = dpad.up.isPressed
        down = dpad.down.isPressed
        left = dpad.left.isPressed
        right = dpad.right.isPressed
    }
    
    mutating func update(thumbstick: GCControllerDirectionPad) {
        let x = Double(thumbstick.xAxis.value)
        let y = Double(thumbstick.yAxis.value)
        
        let distance = sqrt(abs(x) * abs(x) + abs(y) * abs(y))
        var angle: Double
        
        if (distance == 0) {
            angle = 0
        }
        else {
            angle = acos(x / distance) * 180 / Double.pi
            if y < 0 {
                angle = 360 - angle
            }
        }
        
        if distance >= MfiInputDevice.deadZone {
            if angle < MfiInputDevice.coneAngle || angle > 360 - MfiInputDevice.coneAngle {
                right = true
            }
            if  angle > 90 - MfiInputDevice.coneAngle && angle < 90 + MfiInputDevice.coneAngle {
                up = true
            }
            if angle > 180 - MfiInputDevice.coneAngle && angle < 180 + MfiInputDevice.coneAngle {
                left = true
            }
            if angle > 270 - MfiInputDevice.coneAngle && angle < 270 + MfiInputDevice.coneAngle {
                down = true
            }
        }
    }
}
