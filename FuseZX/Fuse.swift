/*
 FuseZX.swift -- High Level Interface to fuse
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
import Emulator

import FuseC

@objc public class Fuse: NSObject {
    public var machine = Machine()
    public var delegate: EmulatorDelegate?
    public var imageView: UIImageView? {
        didSet {
            fuseThread?.imageView = imageView
        }
    }
    
    public override init() {
        fuseThread = FuseThread()
        super.init()
        //fuseThread?.delegate = self
    }
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
        // TODO: send event
        fuse_exiting = 1;
    }
    
    public func reset() {
        // TODO: implement
    }
    
    public func start() {
        fuseThread?.start()
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
