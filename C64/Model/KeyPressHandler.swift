//
//  KeyPressHandler.swift
//  C64
//
//  Created by Dieter Baron on 06.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

import UIKit

protocol KeyPressTranslatorDelegate {
    func press(key: Key);
    func release(key: Key);
}

enum Symbol: Hashable {
    case char(Character)
    case key(UIKeyboardHIDUsage)
}

struct ModifiedSymbol: Hashable {
    var symbol: Symbol
    var modifierFlags: UIKeyModifierFlags
    
    init(symbol: Symbol, modifiers: UIKeyModifierFlags = []) {
        self.symbol = symbol
        self.modifierFlags = modifiers
    }
    
    init(key: UIKey) {
        if key.characters.count == 1, let char = key.characters.first, char >= " " {
            symbol = .char(char)
            modifierFlags = key.modifierFlags.subtracting([.shift, .alphaShift, .alternate])
        }
        else if key.charactersIgnoringModifiers.count == 1, let char = key.charactersIgnoringModifiers.first, char >= "a" && char <= "z" {
            if key.modifierFlags.contains(.shift) || key.modifierFlags.contains(.alphaShift), let uppercaseChar = char.uppercased().first {
                symbol = .char(uppercaseChar)
            }
            else {
                symbol = .char(char)
            }
            modifierFlags = key.modifierFlags.subtracting([.shift, .alphaShift, .alternate])
        }
        else {
            symbol = .key(key.keyCode)
            modifierFlags = key.modifierFlags.subtracting(.alphaShift)
        }
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(symbol)
        hasher.combine(modifierFlags.rawValue)
    }
}

enum MappedSymbol {
    case key(Key, shift: Bool?)
    case modifier(Key)
    case command(ModifiedSymbol)
}

struct KeySymbols {
    var normal: Symbol
    var shifted: Symbol?
    
    init(normal: Symbol, shifted: Symbol? = nil) {
        self.normal = normal
        self.shifted = shifted
    }
    
    init(both: Symbol) {
        self.normal = both
        self.shifted = both
    }
}

class KeyPressTranslator {
    var delegate: KeyPressTranslatorDelegate?
    
    var modifierMap: [UIKeyboardHIDUsage: Key]
    var keyMap: [Key: KeySymbols]
    var symbolRemap: [ModifiedSymbol: ModifiedSymbol]
    var shiftKey: Key
    
    var symbolToKey = [Symbol: Key]()

    var pressedKeys = Set<Key>()
    var forcedShiftKeys = Set<Key>()
    
    init(modifierMap: [UIKeyboardHIDUsage: Key], keyMap: [Key: KeySymbols], symbolRemap: [ModifiedSymbol: ModifiedSymbol] = [:], shiftKey: Key? = nil) {
        self.modifierMap = modifierMap
        self.keyMap = keyMap
        self.symbolRemap = symbolRemap
        self.shiftKey = shiftKey ?? modifierMap[.keyboardLeftShift] ?? Key.ShiftLeft
        
        for entry in keyMap {
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

        for uipress in presses {
            guard let mappedSymbol = map(press: uipress) else { continue }
            
            switch mappedSymbol {
            case .modifier(let key):
                if key.isShift {
                    if !forcedShiftKeys.isEmpty {
                        forcedShiftKeys.removeAll()
                        release(key: shiftKey)
                    }
                    newPressedShiftKeys.insert(key)
                }
                else {
                    press(key: key)
                }

            case .key(let key, shift: let shift):
                if let shift = shift {
                    forceShift = shift
                    if shift {
                        shiftedKeys.insert(key)
                    }
                }
                press(key: key)
                
            case .command(_):
                break // ignore keyboard shortcuts
            }
        }
        
        if forceShift == true {
            if !forcedShiftKeys.isEmpty {
                forcedShiftKeys.formUnion(shiftedKeys)
            }
            else if newPressedShiftKeys.isEmpty && !isShiftPressed {
                press(key: shiftKey)
                forcedShiftKeys.formUnion(shiftedKeys)
            }
        }
        else if forceShift == false {
            newPressedShiftKeys.removeAll()
            forcedShiftKeys.removeAll()
            releaseAllShiftKeys()
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
                if key == shiftKey {
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
            release(key: shiftKey)
        }
        
        return presses
    }
    
    func map(press: UIPress) -> MappedSymbol? {
        guard let uikey = press.key else { return nil }
        
        if let modifierKey = modifierMap[uikey.keyCode] {
            return .modifier(modifierKey)
        }
        
        var modifiedSymbol = ModifiedSymbol(key: uikey)
        modifiedSymbol = symbolRemap[modifiedSymbol] ?? modifiedSymbol
        
        guard !modifiedSymbol.modifierFlags.contains(.command) else { return .command(modifiedSymbol) }

        let symbol = modifiedSymbol.symbol
        guard let key = symbolToKey[symbol] else { return nil }
        guard let keySymbols = keyMap[key] else { return nil }
        
        var shift: Bool?
        if keySymbols.normal != keySymbols.shifted {
            shift = symbol == keySymbols.shifted
        }
        
        return .key(key, shift: shift)
    }

    func press(key: Key) {
        guard !pressedKeys.contains(key) else { return }
        pressedKeys.insert(key)
        delegate?.press(key: key)
    }
    
    func release(key: Key) {
        guard pressedKeys.contains(key) else { return }
        pressedKeys.remove(key)
        delegate?.release(key: key)
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
