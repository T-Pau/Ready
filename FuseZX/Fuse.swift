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

struct FuseEvent: Event {
    enum EventType {
        case press(key: Key)
        case release(key: Key)
    }
    var event: EventType
    var delay: Int = 0
}

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
        fuseThread?.delegate = self
    }
    
    private var eventQueue = EventQueue<FuseEvent>()
    
    func fuseValue(for key: Key) -> keyboard_key_name? {
        switch key {
        case .Char(" "):
            return KEYBOARD_space
        case .Char("0"):
            return KEYBOARD_0
        case .Char("1"):
            return KEYBOARD_1
        case .Char("2"):
            return KEYBOARD_2
        case .Char("3"):
            return KEYBOARD_3
        case .Char("4"):
            return KEYBOARD_4
        case .Char("5"):
            return KEYBOARD_5
        case .Char("6"):
            return KEYBOARD_6
        case .Char("7"):
            return KEYBOARD_7
        case .Char("8"):
            return KEYBOARD_8
        case .Char("9"):
            return KEYBOARD_9
        case .Char("a"):
            return KEYBOARD_a
        case .Char("b"):
            return KEYBOARD_b
        case .Char("c"):
            return KEYBOARD_c
        case .Char("d"):
            return KEYBOARD_d
        case .Char("e"):
            return KEYBOARD_e
        case .Char("f"):
            return KEYBOARD_f
        case .Char("g"):
            return KEYBOARD_g
        case .Char("h"):
            return KEYBOARD_h
        case .Char("i"):
            return KEYBOARD_i
        case .Char("j"):
            return KEYBOARD_j
        case .Char("k"):
            return KEYBOARD_k
        case .Char("l"):
            return KEYBOARD_l
        case .Char("m"):
            return KEYBOARD_m
        case .Char("n"):
            return KEYBOARD_n
        case .Char("o"):
            return KEYBOARD_o
        case .Char("p"):
            return KEYBOARD_p
        case .Char("q"):
            return KEYBOARD_q
        case .Char("r"):
            return KEYBOARD_r
        case .Char("s"):
            return KEYBOARD_s
        case .Char("t"):
            return KEYBOARD_t
        case .Char("u"):
            return KEYBOARD_u
        case .Char("v"):
            return KEYBOARD_v
        case .Char("w"):
            return KEYBOARD_w
        case .Char("x"):
            return KEYBOARD_x
        case .Char("y"):
            return KEYBOARD_y
        case .Char("z"):
            return KEYBOARD_z
        case .Return:
            return KEYBOARD_Enter
        case .Shift:
            return KEYBOARD_Caps
        case .SymbolShift:
            return KEYBOARD_Symbol

        default:
            return nil
        }
    }
}

extension Fuse: FuseThreadDelegate {
    public func handleEvents() -> Bool {
        eventQueue.process() { event in
            switch event.event {
            case .press(let key):
                if let keyName = fuseValue(for: key) {
                    keyboard_press(keyName)
                }
                
            case .release(let key):
                if let keyName = fuseValue(for: key) {
                    keyboard_release(keyName)
                }
            }
        }
        
        return true
    }
}

extension Fuse: Emulator {
    public func release(key: Key, delayed: Int) {
        eventQueue.send(event: FuseEvent(event: .release(key: key), delay: delayed))
    }
    
    public func press(key: Key, delayed: Int) {
        eventQueue.send(event: FuseEvent(event: .press(key: key), delay: delayed))
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
