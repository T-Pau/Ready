/*
 Emulator.swift -- Interface to Emulator Core
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
import UIKit


@objc open class Emulator: NSObject {
    public var machine = MachineOld()
    open var imageView: UIImageView?
    public var delegate: EmulatorDelegate?
    
    private var eventQueue = EventQueue()
    
    private var pressedKeys = Multiset<Key>()
    private var joystickState = [JoystickButtons](repeating: JoystickButtons(), count: 10)

    // Call this function once per frame on emulator thread to process pending events. handle(event:) is called for each event.
    @objc public func handleEvents() -> Bool {
        return eventQueue.process(handler: handle)
    }
        
    // Override this in implementation. It is called on the emulator thread.
    open func handle(event: Event) -> Bool {
        return true
    }
    
    // trigger cartridge freeze function
    open func freeze() {
        send(event: .freeze)
    }
    
    // press key, delayed by given number of frames
    open func press(key: Key, delayed: Int = 0) {
        pressedKeys.add(key)
        if pressedKeys.count(for: key) == 1 {
            send(event: .key(key, pressed: true), delay: delayed)
        }
    }
    
    // quit emulation
    open func quit() {
        send(event: .quit)
    }
    
    // release key, delayed by given number of frames
    open func release(key: Key, delayed: Int = 0) {
        pressedKeys.remove(key)
        if pressedKeys.count(for: key) == 0 {
            send(event: .key(key, pressed: false), delay: delayed)
        }
    }

    // reset machine
    open func reset() {
        send(event: .reset)
    }
    
    open func start() {
        
    }
    
    open func attach(drive: Int, image: DiskImage?) {
        
    }

    open func joystick(_ index: Int, buttons: JoystickButtons) {
        guard index > 0 && index <= joystickState.count else { return }
        guard buttons != joystickState[index] else { return }
        
        send(event: .joystick(port: index, buttons: buttons, oldButtons: joystickState[index]))
        joystickState[index] = buttons
    }
    
    open func mouse(moved distance: CGPoint) {
    
    }
    
    open func mouse(setX x: Int32) {
        
    }

    open func mouse(setY y: Int32) {
        
    }

    open func mouse(pressed button: Int) {
        send(event: .mouseButton(button: button, pressed: true))
    }

    open func mouse(release button: Int) {
        send(event: .mouseButton(button: button, pressed: false))
    }

    open func lightPen(moved position: CGPoint?, size: CGSize, button1: Bool, button2: Bool, isKoalaPad: Bool) {
        
    }

    open func setResource(name: MachineOld.ResourceName, value: MachineOld.ResourceValue) {
        
    }

    open func setResourceNow(name: MachineOld.ResourceName, value: MachineOld.ResourceValue) {
        
    }

    open func set(borderMode: MachineConfigOld.BorderMode) {
        
    }
    
    public func send(event: Event, delay: Int = 0) {
        eventQueue.send(event: event, delay: delay)
    }
}

extension Emulator: RendererDelegate {
    @objc public func update(_ image: UIImage?) {
        DispatchQueue.main.async {
            self.imageView?.image = image
        }
    }
}
