/*
 Emulator.swift -- Interface to Emulator Core
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

import Foundation
import UIKit


@objc open class Emulator: NSObject {
    public var machine = MachineOld()
    open var imageViews = [UIImageView?]()
    public var delegate: EmulatorDelegate?
    
    public var emulatorThread: EmulatorThread?
    
    public var screens = ["Screen"] {
        didSet {
            if (oldValue.count < screens.count) {
                for _ in [oldValue.count ..< screens.count] {
                    let renderer = Renderer()
                    renderer.delegate = self
                    emulatorThread?.renderers.add(renderer)
                }
            }
            else if (oldValue.count > screens.count) {
                for _ in [screens.count ..< oldValue.count] {
                    emulatorThread?.renderers.removeLastObject()
                }
            }
        }
    }
    
    private var eventQueue = EventQueue()
    
    private var pressedKeys = Multiset<Key>()
    private var joystickState = [JoystickButtons](repeating: JoystickButtons(), count: 10)

    public init(emulatorThread: EmulatorThread?) {
        self.emulatorThread = emulatorThread
        super.init()
        emulatorThread?.delegate = self
    }

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
        emulatorThread?.renderer.borderMode = RendererBorderMode(rawValue: RendererBorderMode.RawValue(machine.specification.borderMode.cValue))
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
        emulatorThread?.borderMode = borderMode.cValue
    }
    
    public func send(event: Event, delay: Int = 0) {
        eventQueue.send(event: event, delay: delay)
    }
}

extension Emulator: RendererDelegate {
    @objc public func renderer(_ renderer: Renderer, update image: UIImage?) {
        guard let index = emulatorThread?.renderers.index(of: renderer), index < imageViews.count else { return }
        DispatchQueue.main.async {
            self.imageViews[index]?.image = image
        }
    }
}
