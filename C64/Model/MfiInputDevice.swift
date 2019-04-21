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
                
                buttons.up = gamepad.dpad.up.isPressed || gamepad.leftThumbstick.up.isPressed
                buttons.down = gamepad.dpad.down.isPressed || gamepad.leftThumbstick.down.isPressed
                buttons.left = gamepad.dpad.left.isPressed || gamepad.leftThumbstick.left.isPressed
                buttons.right = gamepad.dpad.right.isPressed || gamepad.leftThumbstick.right.isPressed
                buttons.fire = gamepad.buttonA.isPressed
                
                self.update(buttons: buttons)
            }
        }
        else if let gamepad = controller.microGamepad {
            gamepad.valueChangedHandler = { gamepad, _ in
                var buttons = JoystickButtons()
                
                buttons.up = gamepad.dpad.up.isPressed
                buttons.down = gamepad.dpad.down.isPressed
                buttons.left = gamepad.dpad.left.isPressed
                buttons.right = gamepad.dpad.right.isPressed
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
        "Steel Series Nimbus" : Product(name: "Nimubs", iconNames: [ "Steel Series Nimbus" ]),
        "Steel Series Stratus" : Product(name: "Stratus", iconNames: [ "Steel Series Stratus White" ])
    ]
}
