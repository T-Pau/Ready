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

@objc public class Fuse: Emulator {
    public override init() {
        fuseThread = FuseThread()
        super.init()
        fuseThread?.delegate = self
    }
    
    private var pressedKeys = Multiset<Key>()
    
    func fuseName(for model: Computer.ViceModel) -> String? {
        switch model {
        case .zxSpectrum16k:
            return "16"
            
        case .zxSpectrum48k:
            return "48"
            
        case .zxSpectrum48kNtsc:
            return "48_ntsc"
            
        case .zxSpectrum128k:
            return "128"
            
        default:
            return nil
        }
    }
    
    func fuseKeys(for key: Key) -> [keyboard_key_name]? {
        switch key {
        case .Char(" "):
            return [KEYBOARD_space]
        case .Char("0"):
            return [KEYBOARD_0]
        case .Char("1"):
            return [KEYBOARD_1]
        case .Char("2"):
            return [KEYBOARD_2]
        case .Char("3"):
            return [KEYBOARD_3]
        case .Char("4"):
            return [KEYBOARD_4]
        case .Char("5"):
            return [KEYBOARD_5]
        case .Char("6"):
            return [KEYBOARD_6]
        case .Char("7"):
            return [KEYBOARD_7]
        case .Char("8"):
            return [KEYBOARD_8]
        case .Char("9"):
            return [KEYBOARD_9]
        case .Char("a"):
            return [KEYBOARD_a]
        case .Char("b"):
            return [KEYBOARD_b]
        case .Char("c"):
            return [KEYBOARD_c]
        case .Char("d"):
            return [KEYBOARD_d]
        case .Char("e"):
            return [KEYBOARD_e]
        case .Char("f"):
            return [KEYBOARD_f]
        case .Char("g"):
            return [KEYBOARD_g]
        case .Char("h"):
            return [KEYBOARD_h]
        case .Char("i"):
            return [KEYBOARD_i]
        case .Char("j"):
            return [KEYBOARD_j]
        case .Char("k"):
            return [KEYBOARD_k]
        case .Char("l"):
            return [KEYBOARD_l]
        case .Char("m"):
            return [KEYBOARD_m]
        case .Char("n"):
            return [KEYBOARD_n]
        case .Char("o"):
            return [KEYBOARD_o]
        case .Char("p"):
            return [KEYBOARD_p]
        case .Char("q"):
            return [KEYBOARD_q]
        case .Char("r"):
            return [KEYBOARD_r]
        case .Char("s"):
            return [KEYBOARD_s]
        case .Char("t"):
            return [KEYBOARD_t]
        case .Char("u"):
            return [KEYBOARD_u]
        case .Char("v"):
            return [KEYBOARD_v]
        case .Char("w"):
            return [KEYBOARD_w]
        case .Char("x"):
            return [KEYBOARD_x]
        case .Char("y"):
            return [KEYBOARD_y]
        case .Char("z"):
            return [KEYBOARD_z]
        case .Return:
            return [KEYBOARD_Enter]
        case .Shift:
            return [KEYBOARD_Caps]
        case .SymbolShift:
            return [KEYBOARD_Symbol]

        case .Char("."):
            return [KEYBOARD_Symbol, KEYBOARD_m]
        case .Char(";"):
            return [KEYBOARD_Symbol, KEYBOARD_o]
        case .Char(","):
            return [KEYBOARD_Symbol, KEYBOARD_n]
        case .Char("\""):
            return [KEYBOARD_Symbol, KEYBOARD_p]
        case .Break:
            return [KEYBOARD_Caps, KEYBOARD_space]
        case .CursorLeft:
            return [KEYBOARD_Caps, KEYBOARD_5]
        case .CursorDown:
            return [KEYBOARD_Caps, KEYBOARD_6]
        case .CursorUp:
            return [KEYBOARD_Caps, KEYBOARD_7]
        case .CursorRight:
            return [KEYBOARD_Caps, KEYBOARD_8]
        case .Delete:
            return [KEYBOARD_Caps, KEYBOARD_0]
        case .Edit:
            return [KEYBOARD_Caps, KEYBOARD_1]
        case .ExtendedMode:
            return [KEYBOARD_Caps, KEYBOARD_Symbol]
        case .Graphics:
            return [KEYBOARD_Caps, KEYBOARD_9]
        case .InverseVideo:
            return [KEYBOARD_Caps, KEYBOARD_4]
        case .ShiftLock:
            return [KEYBOARD_Caps, KEYBOARD_2]
        case .TrueVideo:
            return [KEYBOARD_Caps, KEYBOARD_3]

        default:
            return nil
        }
    }
    
    override public func handle(event: Event) -> Bool {
        switch event {
        case .key(let key, let pressed):
            if let keyNames = fuseKeys(for: key) {
                if pressed {
                    pressedKeys.add(key)
                    if pressedKeys.count(for: key) == 1 {
                        for keyName in keyNames {
                            keyboard_press(keyName)
                        }
                    }
                }
                else {
                    pressedKeys.remove(key)
                    if pressedKeys.count(for: key) == 0 {
                        for keyName in keyNames {
                            keyboard_release(keyName)
                        }
                    }
                }
            }
            
        case .quit:
            fuse_exiting = 1;
            
        default:
            break
        }
        
        return true
    }
    
    override public func start() {
        guard let modelName = fuseName(for: machine.specification.computer.viceMachineModel) else { return } // TODO: close view
        var args = [
            "fuse",
            "--no-interface2", "--no-zxprinter",
//            "--no-traps", "--no-fastload", "--no-accelerate-loader",
            "--machine", modelName
        ]
        if let tapeImage = machine.tapeImages.first, let url = tapeImage.url {
            args.append("--tape")
            args.append(url.path)
        }
        fuseThread?.args = args
        fuseThread?.start()
    }
    
    public override func set(borderMode: MachineConfigOld.BorderMode) {
        fuseThread?.newBorderMode = borderMode.cValue
    }
}

