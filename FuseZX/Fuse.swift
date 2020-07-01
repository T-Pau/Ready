//
//  Fuse.swift
//  Fuse
//
//  Created by Dieter Baron on 30.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import UIKit
import Emulator

import FuseC

@objc public class Fuse: NSObject {
    public var machine = Machine()
    public var delegate: EmulatorDelegate?
    public var imageView: UIImageView?
}

extension Fuse: Emulator {
    public func release(key: Key, delayed: Int) {
        // TODO: implement
    }
    
    public func press(key: Key, delayed: Int) {
        // TODO: implement
    }
    
    public func freeze() {
        // TODO: implement
    }
    
    public func quit() {
        // TODO: implement
    }
    
    public func reset() {
        // TODO: implement
    }
    
    public func start() {
        // TODO: implement
    }
    
    public func attach(drive: Int, image: DiskImage?) {
        // TODO: implement
    }
    
    public func joystick(_ index: Int, buttons: JoystickButtons) {
        // TODO: implement
    }
    
    public func mouse(moved distance: CGPoint) {
        // TODO: implement
    }
    
    public func mouse(setX x: Int32) {
        // TODO: implement
    }
    
    public func mouse(setY y: Int32) {
        // TODO: implement
    }
    
    public func mouse(pressed button: Int) {
        // TODO: implement
    }
    
    public func mouse(release button: Int) {
        // TODO: implement
    }
    
    public func lightPen(moved position: CGPoint?, size: CGSize, button1: Bool, button2: Bool, isKoalaPad: Bool) {
        // TODO: implement
    }
    
    public func setResource(name: Machine.ResourceName, value: Machine.ResourceValue) {
        // TODO: implement
    }
    
    public func setResourceNow(name: Machine.ResourceName, value: Machine.ResourceValue) {
        // TODO: implement
    }
    
    public func set(borderMode: MachineConfig.BorderMode) {
        // TODO: implement
    }
}
