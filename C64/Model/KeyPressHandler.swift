//
//  KeyPressHandler.swift
//  C64
//
//  Created by Dieter Baron on 06.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import UIKit
import Emulator

protocol KeyPressTranslatorDelegate {
    func press(key: Key, delayed: Int)
    func release(key: Key, delayed: Int)
}

extension KeyPressTranslatorDelegate {
    func press(key: Key) {
        press(key: key, delayed: 0)
    }
    
    func release(key: Key) {
        release(key: key, delayed: 0)
    }
}


class KeyPressTranslator {
    var delegate: KeyPressTranslatorDelegate?
    
    var keyboardSymbols: KeyboardSymbols
    
    var symbolToKey = [KeyboardSymbols.Symbol: Key]()

    var pressedKeys = Set<Key>()
    var forcedShiftKeys = Set<Key>()
    
    let forcedShiftDelay = 1
    
    init(keyboardSymbols: KeyboardSymbols) {
        self.keyboardSymbols = keyboardSymbols
        for entry in keyboardSymbols.keyMap {
            symbolToKey[entry.value.normal] = entry.key
            if let shifted = entry.value.shifted {
                symbolToKey[shifted] = entry.key
            }
        }
    }
    
    func pressesBegan(_ presses: Set<UIPress>, with event: UIPressesEvent?) -> Set<UIPress> {
        var newPressedShiftKeys = Set<Key>()
        var forceShift: Bool?
        var shiftedKeys = Set<Key>()
        var newPressedKeys = Set<Key>()

        for uipress in presses {
            guard let mappedSymbol = map(press: uipress) else { continue }
            
            switch mappedSymbol {
            case .modifier(let key):
                if key.isShift {
                    if !forcedShiftKeys.isEmpty {
                        forcedShiftKeys.removeAll()
                        release(key: keyboardSymbols.shiftKey)
                    }
                    newPressedShiftKeys.insert(key)
                }
                else {
                    newPressedKeys.insert(key)
                }

            case .key(let key, shift: let shift):
                if let shift = shift {
                    forceShift = shift
                    if shift {
                        shiftedKeys.insert(key)
                    }
                }
                newPressedKeys.insert(key)

            case .command(_):
                break // ignore keyboard shortcuts
            }
        }
        
        var delayed = false

        if forceShift == true {
            if !forcedShiftKeys.isEmpty {
                forcedShiftKeys.formUnion(shiftedKeys)
            }
            else if newPressedShiftKeys.isEmpty && !isShiftPressed {
                press(key: keyboardSymbols.shiftKey)
                forcedShiftKeys.formUnion(shiftedKeys)
                delayed = true
            }
        }
        else if forceShift == false {
            newPressedShiftKeys.removeAll()
            forcedShiftKeys.removeAll()
            delayed = isShiftPressed
            releaseAllShiftKeys()
        }

        for key in newPressedKeys {
            press(key: key, delayed: delayed ? forcedShiftDelay : 0)
        }
        for key in newPressedShiftKeys {
            press(key: key)
        }
        
        return presses
    }

    func pressesEnded(_ presses: Set<UIPress>, with event: UIPressesEvent?) -> Set<UIPress> {
        var releaseForcedShift = false

        for uipress in presses {
            guard let mappedSymbol = map(press: uipress) else { continue }
            
            switch mappedSymbol {
            case .modifier(let key):
                if key == keyboardSymbols.shiftKey {
                    forcedShiftKeys.removeAll()
                }
                release(key: key)
                
            case .key(let key, shift: _):
                if forcedShiftKeys.contains(key) {
                    forcedShiftKeys.remove(key)
                    if forcedShiftKeys.isEmpty {
                        releaseForcedShift = true
                    }
                }
                release(key: key)
                
            case .command(_):
                break // ignore keyboard shortcuts
            }
        }

        if releaseForcedShift {
            release(key: keyboardSymbols.shiftKey)
        }
        
        return presses
    }
    
    func map(press: UIPress) -> KeyboardSymbols.MappedSymbol? {
        guard let uikey = press.key else { return nil }
        
        if let modifierKey = keyboardSymbols.modifierMap[uikey.keyCode] {
            return .modifier(modifierKey)
        }
        
        var modifiedSymbol = KeyboardSymbols.ModifiedSymbol(key: uikey)
        modifiedSymbol = keyboardSymbols.symbolRemap[modifiedSymbol] ?? modifiedSymbol
        
        guard !modifiedSymbol.modifierFlags.contains(.command) else { return .command(modifiedSymbol) }

        let symbol = modifiedSymbol.symbol
        guard let key = symbolToKey[symbol] else { return nil }
        guard let keySymbols = keyboardSymbols.keyMap[key] else { return nil }
        
        var shift: Bool?
        if keySymbols.normal != keySymbols.shifted {
            shift = symbol == keySymbols.shifted
        }
        
        return .key(key, shift: shift)
    }

    func press(key: Key, delayed: Int = 0) {
        guard !pressedKeys.contains(key) else { return }
        pressedKeys.insert(key)
        delegate?.press(key: key, delayed: delayed)
    }
    
    func release(key: Key, delayed: Int = 0) {
        guard pressedKeys.contains(key) else { return }
        pressedKeys.remove(key)
        delegate?.release(key: key, delayed: delayed)
    }
    
    var isShiftPressed: Bool {
        for key in pressedKeys {
            if key.isShift {
                return true
            }
        }
        return false
    }
    
    func releaseAllShiftKeys() {
        let pressedShiftKeys = pressedKeys.filter({ key in key.isShift })
        for key in pressedShiftKeys {
            delegate?.release(key: key)
        }
        pressedKeys.subtract(pressedShiftKeys)
    }
    
    func releaseAllKeys() {
        for key in pressedKeys {
            delegate?.release(key: key)
        }
        pressedKeys.removeAll()
    }
}
