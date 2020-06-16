//
//  Emulator.swift
//  EmulatorInterface
//
//  Created by Dieter Baron on 14.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import Foundation
import CoreGraphics

public protocol Emulator: class {
    // trigger cartridge freeze function
    func freeze()
    
    // press key, delayed by given number of frames
    func press(key: Key, delayed: Int)
    
    // quit emulation
    func quit()
    
    // release key, delayed by given number of frames
    func release(key: Key, delayed: Int)

    // reset machine
    func reset()

    func joystick(_ index: Int, buttons: JoystickButtons)
    
    func mouse(moved distance: CGPoint)
    
    func mouse(setX x: Int32)

    func mouse(setY y: Int32)

    func mouse(pressed button: Int)

    func mouse(release button: Int)

    func lightPen(moved position: CGPoint?, size: CGSize, button1: Bool, button2: Bool, isKoalaPad: Bool)

    func setResource(name: Machine.ResourceName, value: Machine.ResourceValue)

    func setResourceNow(name: Machine.ResourceName, value: Machine.ResourceValue)

    func set(borderMode: MachineConfig.BorderMode)
}


public extension Emulator {
    func press(key: Key) {
        press(key: key, delayed: 0)
    }
    
    func release(key: Key) {
        release(key: key, delayed: 0)
    }
}
