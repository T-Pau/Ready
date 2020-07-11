/*
 KeyboardSymbols.swift -- Mapping Keys to their Symbols
 Copyright (C) 2020 Dieter Baron
 
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

public struct KeyboardSymbols {
    public enum Symbol: Hashable {
        case char(Character)
        case key(UIKeyboardHIDUsage)
    }

    public struct ModifiedSymbol: Hashable {
        public var symbol: Symbol
        public var modifierFlags: UIKeyModifierFlags
        
        public init(symbol: Symbol, modifiers: UIKeyModifierFlags = []) {
            self.symbol = symbol
            self.modifierFlags = modifiers
        }
    
        public init(key: UIKey) {
            if key.characters.count == 1, let char = key.characters.first, char >= " " {
                symbol = .char(char)
                modifierFlags = key.modifierFlags.subtracting([.shift, .alphaShift, .alternate])
            }
            else if key.charactersIgnoringModifiers.count == 1, let char = key.charactersIgnoringModifiers.first, (char >= "a" && char <= "z") || char == "`" || char == "\"" {
                symbol = .char(char)
                modifierFlags = key.modifierFlags.subtracting([.shift, .alphaShift, .alternate])
                if char >= "a" && char <= "z" && (key.modifierFlags.contains(.shift) || key.modifierFlags.contains(.alphaShift)), let uppercaseChar = char.uppercased().first {
                        symbol = .char(uppercaseChar)
                }
            }
            else if key.characters.count == 2, let char = key.characters.dropFirst().first, char >= " " {
                symbol = .char(char)
                modifierFlags = key.modifierFlags.subtracting([.shift, .alphaShift, .alternate])
            }
            else {
                symbol = .key(key.keyCode)
                modifierFlags = key.modifierFlags.subtracting(.alphaShift)
            }
        }
        
        public func hash(into hasher: inout Hasher) {
            hasher.combine(symbol)
            hasher.combine(modifierFlags.rawValue)
        }
    }
    
    public enum MappedSymbol {
        case key(Key, shift: Bool?)
        case modifier(Key)
        case command(ModifiedSymbol)
    }
    
    public struct KeySymbols {
        public var normal: Symbol
        public var shifted: Symbol?
        
        public init(normal: Symbol, shifted: Symbol? = nil) {
            self.normal = normal
            self.shifted = shifted
        }
        
        public init(both: Symbol) {
            self.normal = both
            self.shifted = both
        }
    }
    
    public var modifierMap: [UIKeyboardHIDUsage: Key]
    public var keyMap: [Key: KeySymbols]
    public var symbolRemap: [ModifiedSymbol: ModifiedSymbol]
    public var shiftKey: Key
    
    public init(modifierMap: [UIKeyboardHIDUsage: Key], keyMap: [Key: KeySymbols], symbolRemap: [ModifiedSymbol: ModifiedSymbol] = [:], shiftKey: Key? = nil) {
        self.modifierMap = modifierMap
        self.keyMap = keyMap
        self.symbolRemap = symbolRemap
        self.shiftKey = shiftKey ?? modifierMap[.keyboardRightShift] ?? Key.ShiftRight
    }
    
    public static let atariXl = KeyboardSymbols(modifierMap: [
            .keyboardLeftShift: .ShiftLeft,
            .keyboardRightShift: .ShiftRight,
            .keyboardLeftControl: .Control,
            .keyboardRightControl: .Control
        ],
        keyMap: [
            .Escape: KeySymbols(both: .key(.keyboardEscape)),
            .Char("1"): KeySymbols(normal: .char("1"), shifted: .char("!")),
            .Char("2"): KeySymbols(normal: .char("2"), shifted: .char("\"")),
            .Char("3"): KeySymbols(normal: .char("3"), shifted: .char("#")),
            .Char("4"): KeySymbols(normal: .char("4"), shifted: .char("$")),
            .Char("5"): KeySymbols(normal: .char("5"), shifted: .char("%")),
            .Char("6"): KeySymbols(normal: .char("6"), shifted: .char("&")),
            .Char("7"): KeySymbols(normal: .char("7"), shifted: .char("'")),
            .Char("8"): KeySymbols(normal: .char("8"), shifted: .char("@")),
            .Char("9"): KeySymbols(normal: .char("9"), shifted: .char("(")),
            .Char("0"): KeySymbols(normal: .char("0"), shifted: .char(")")),
            .Char("<"): KeySymbols(normal: .char("<")), // TODO: clear
            .Char(">"): KeySymbols(normal: .char(">")), // TODO: insert
            .Delete: KeySymbols(both: .key(.keyboardDeleteOrBackspace)),
            
            .Tab: KeySymbols(both: .key(.keyboardTab)),
            .Char("q"): KeySymbols(normal: .char("q"), shifted: .char("Q")),
            .Char("w"): KeySymbols(normal: .char("w"), shifted: .char("W")),
            .Char("e"): KeySymbols(normal: .char("e"), shifted: .char("E")),
            .Char("r"): KeySymbols(normal: .char("r"), shifted: .char("R")),
            .Char("t"): KeySymbols(normal: .char("t"), shifted: .char("T")),
            .Char("y"): KeySymbols(normal: .char("y"), shifted: .char("Y")),
            .Char("u"): KeySymbols(normal: .char("u"), shifted: .char("U")),
            .Char("i"): KeySymbols(normal: .char("i"), shifted: .char("I")),
            .Char("o"): KeySymbols(normal: .char("o"), shifted: .char("O")),
            .Char("p"): KeySymbols(normal: .char("p"), shifted: .char("P")),
            .Char("-"): KeySymbols(normal: .char("-"), shifted: .char("_")), // TOOD: control: cursor up
            .Char("="): KeySymbols(normal: .char("="), shifted: .char("|")), // TOOD: control: cursor down
            .Return: KeySymbols(both: .key(.keyboardReturnOrEnter)),
            
            .Char("a"): KeySymbols(normal: .char("a"), shifted: .char("A")),
            .Char("s"): KeySymbols(normal: .char("s"), shifted: .char("S")),
            .Char("d"): KeySymbols(normal: .char("d"), shifted: .char("D")),
            .Char("f"): KeySymbols(normal: .char("f"), shifted: .char("F")),
            .Char("g"): KeySymbols(normal: .char("g"), shifted: .char("G")),
            .Char("h"): KeySymbols(normal: .char("h"), shifted: .char("H")),
            .Char("j"): KeySymbols(normal: .char("j"), shifted: .char("J")),
            .Char("k"): KeySymbols(normal: .char("k"), shifted: .char("K")),
            .Char("l"): KeySymbols(normal: .char("l"), shifted: .char("L")),
            .Char(";"): KeySymbols(normal: .char(";"), shifted: .char(":")),
            .Char("+"): KeySymbols(normal: .char("+"), shifted: .char("\\")), // TODO: control: cursor left
            .Char("*"): KeySymbols(normal: .char("*"), shifted: .char("^")), // TODO: control: cursor right
            // TODO: caps
            
            .Char("z"): KeySymbols(normal: .char("z"), shifted: .char("Z")),
            .Char("x"): KeySymbols(normal: .char("x"), shifted: .char("X")),
            .Char("c"): KeySymbols(normal: .char("c"), shifted: .char("C")),
            .Char("v"): KeySymbols(normal: .char("v"), shifted: .char("V")),
            .Char("b"): KeySymbols(normal: .char("b"), shifted: .char("B")),
            .Char("n"): KeySymbols(normal: .char("n"), shifted: .char("N")),
            .Char("m"): KeySymbols(normal: .char("m"), shifted: .char("M")),
            .Char(","): KeySymbols(normal: .char(","), shifted: .char("[")),
            .Char("."): KeySymbols(normal: .char("."), shifted: .char("]")),
            .Char("/"): KeySymbols(normal: .char("/"), shifted: .char("?")),
            // TODO: inverse video
            
            .Char(" "): KeySymbols(both: .char(" "))
        ],
        symbolRemap: [
            ModifiedSymbol(symbol: .char("`")): ModifiedSymbol(symbol: .key(.keyboardEscape)),
            ModifiedSymbol(symbol: .char("~")): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.shift]),
            ModifiedSymbol(symbol: .char("`"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: .control),
            ModifiedSymbol(symbol: .char("~"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.control, .shift])
        ])
    
    public static let c64 = KeyboardSymbols(modifierMap: [
            .keyboardLeftShift: .ShiftLeft,
            .keyboardRightShift: .ShiftRight,
            .keyboardLeftControl: .Control,
            .keyboardRightControl: .Control,
            UIKeyboardHIDUsage(rawValue: 669)!: .Commodore
        ],
        keyMap: [
            .ArrowLeft: KeySymbols(both: .key(.keyboardEscape)),
            .Char("1"): KeySymbols(normal: .char("1"), shifted: .char("!")),
            .Char("2"): KeySymbols(normal: .char("2"), shifted: .char("\"")),
            .Char("3"): KeySymbols(normal: .char("3"), shifted: .char("#")),
            .Char("4"): KeySymbols(normal: .char("4"), shifted: .char("$")),
            .Char("5"): KeySymbols(normal: .char("5"), shifted: .char("%")),
            .Char("6"): KeySymbols(normal: .char("6"), shifted: .char("&")),
            .Char("7"): KeySymbols(normal: .char("7"), shifted: .char("'")),
            .Char("8"): KeySymbols(normal: .char("8"), shifted: .char("(")),
            .Char("9"): KeySymbols(normal: .char("9"), shifted: .char(")")),
            .Char("0"): KeySymbols(normal: .char("0"), shifted: .char("0")),
            .Char("+"): KeySymbols(normal: .char("+")),
            .Char("-"): KeySymbols(normal: .char("-")),
            .Char("£"): KeySymbols(normal: .char("£")),
            .ClearHome: KeySymbols(both: .key(.keyboardHome)),
            .InsertDelete: KeySymbols(both: .key(.keyboardDeleteOrBackspace)),
                            
            .Char("q"): KeySymbols(normal: .char("q"), shifted: .char("Q")),
            .Char("w"): KeySymbols(normal: .char("w"), shifted: .char("W")),
            .Char("e"): KeySymbols(normal: .char("e"), shifted: .char("E")),
            .Char("r"): KeySymbols(normal: .char("r"), shifted: .char("R")),
            .Char("t"): KeySymbols(normal: .char("t"), shifted: .char("T")),
            .Char("y"): KeySymbols(normal: .char("y"), shifted: .char("Y")),
            .Char("u"): KeySymbols(normal: .char("u"), shifted: .char("U")),
            .Char("i"): KeySymbols(normal: .char("i"), shifted: .char("I")),
            .Char("o"): KeySymbols(normal: .char("o"), shifted: .char("O")),
            .Char("p"): KeySymbols(normal: .char("p"), shifted: .char("P")),
            .Char("@"): KeySymbols(normal: .char("@")),
            .Char("*"): KeySymbols(normal: .char("*")),
            .ArrowUp: KeySymbols(normal: .char("^"), shifted: .char("π")),
            .Restore: KeySymbols(normal: .char("\\"), shifted: .char("|")),
                            
            .RunStop: KeySymbols(both: .key(.keyboardTab)),
            .Char("a"): KeySymbols(normal: .char("a"), shifted: .char("A")),
            .Char("s"): KeySymbols(normal: .char("s"), shifted: .char("S")),
            .Char("d"): KeySymbols(normal: .char("d"), shifted: .char("D")),
            .Char("f"): KeySymbols(normal: .char("f"), shifted: .char("F")),
            .Char("g"): KeySymbols(normal: .char("g"), shifted: .char("G")),
            .Char("h"): KeySymbols(normal: .char("h"), shifted: .char("H")),
            .Char("j"): KeySymbols(normal: .char("j"), shifted: .char("J")),
            .Char("k"): KeySymbols(normal: .char("k"), shifted: .char("K")),
            .Char("l"): KeySymbols(normal: .char("l"), shifted: .char("L")),
            .Char(":"): KeySymbols(normal: .char(":"), shifted: .char("[")),
            .Char(";"): KeySymbols(normal: .char(";"), shifted: .char("]")),
            .Char("="): KeySymbols(normal: .char("="), shifted: .char("=")),
            .Return: KeySymbols(both: .key(.keyboardReturnOrEnter)),
                            
            .Char("z"): KeySymbols(normal: .char("z"), shifted: .char("Z")),
            .Char("x"): KeySymbols(normal: .char("x"), shifted: .char("X")),
            .Char("c"): KeySymbols(normal: .char("c"), shifted: .char("C")),
            .Char("v"): KeySymbols(normal: .char("v"), shifted: .char("V")),
            .Char("b"): KeySymbols(normal: .char("b"), shifted: .char("B")),
            .Char("n"): KeySymbols(normal: .char("n"), shifted: .char("N")),
            .Char("m"): KeySymbols(normal: .char("m"), shifted: .char("M")),
            .Char(","): KeySymbols(normal: .char(","), shifted: .char("<")),
            .Char("."): KeySymbols(normal: .char("."), shifted: .char(">")),
            .Char("/"): KeySymbols(normal: .char("/"), shifted: .char("?")),
            .CursorLeftRight: KeySymbols(normal: .key(.keyboardRightArrow), shifted: .key(.keyboardLeftArrow)),
            .CursorUpDown: KeySymbols(normal: .key(.keyboardDownArrow), shifted: .key(.keyboardUpArrow)),
                            
            .Char(" "): KeySymbols(both: .char(" ")),
                            
            .F1: KeySymbols(normal: .key(.keyboardF1), shifted: .key(.keyboardF2)),
            .F3: KeySymbols(normal: .key(.keyboardF3), shifted: .key(.keyboardF4)),
            .F5: KeySymbols(normal: .key(.keyboardF5), shifted: .key(.keyboardF6)),
            .F7: KeySymbols(normal: .key(.keyboardF7), shifted: .key(.keyboardF8)),
        ],
        symbolRemap: [
            ModifiedSymbol(symbol: .char("1"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF1)),
            ModifiedSymbol(symbol: .char("2"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF2)),
            ModifiedSymbol(symbol: .char("3"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF3)),
            ModifiedSymbol(symbol: .char("4"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF4)),
            ModifiedSymbol(symbol: .char("5"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF5)),
            ModifiedSymbol(symbol: .char("6"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF6)),
            ModifiedSymbol(symbol: .char("7"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF7)),
            ModifiedSymbol(symbol: .char("8"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF8)),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardHome)),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.shift, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: .shift),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.control, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: .control),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.control, .shift, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: [.control, .shift]),
            ModifiedSymbol(symbol: .char("`")): ModifiedSymbol(symbol: .key(.keyboardEscape)),
            ModifiedSymbol(symbol: .char("~")): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.shift]),
            ModifiedSymbol(symbol: .char("`"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: .control),
            ModifiedSymbol(symbol: .char("~"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.control, .shift])
        ])
    
    public static let plus4 = KeyboardSymbols(modifierMap: [
            .keyboardLeftShift: .Shift,
            .keyboardRightShift: .Shift,
            .keyboardLeftControl: .Control,
            .keyboardRightControl: .Control,
            UIKeyboardHIDUsage(rawValue: 669)!: .Commodore
        ],
        keyMap: [
            .F1: KeySymbols(normal: .key(.keyboardF1), shifted: .key(.keyboardF4)),
            .F2: KeySymbols(normal: .key(.keyboardF2), shifted: .key(.keyboardF5)),
            .F3: KeySymbols(normal: .key(.keyboardF3), shifted: .key(.keyboardF6)),
            .Help: KeySymbols(normal: .key(.keyboardF8), shifted: .key(.keyboardF7)),

            .Escape: KeySymbols(both: .key(.keyboardEscape)),
            .Char("1"): KeySymbols(normal: .char("1"), shifted: .char("!")),
            .Char("2"): KeySymbols(normal: .char("2"), shifted: .char("\"")),
            .Char("3"): KeySymbols(normal: .char("3"), shifted: .char("#")),
            .Char("4"): KeySymbols(normal: .char("4"), shifted: .char("$")),
            .Char("5"): KeySymbols(normal: .char("5"), shifted: .char("%")),
            .Char("6"): KeySymbols(normal: .char("6"), shifted: .char("&")),
            .Char("7"): KeySymbols(normal: .char("7"), shifted: .char("'")),
            .Char("8"): KeySymbols(normal: .char("8"), shifted: .char("(")),
            .Char("9"): KeySymbols(normal: .char("9"), shifted: .char(")")),
            .Char("0"): KeySymbols(normal: .char("0"), shifted: .char("^")),
            .Char("+"): KeySymbols(normal: .char("+")),
            .Char("-"): KeySymbols(normal: .char("-")),
            .Char("="): KeySymbols(normal: .char("=")),
            .ClearHome: KeySymbols(both: .key(.keyboardHome)),
            .InsertDelete: KeySymbols(both: .key(.keyboardDeleteOrBackspace)),
            
            .Char("q"): KeySymbols(normal: .char("q"), shifted: .char("Q")),
            .Char("w"): KeySymbols(normal: .char("w"), shifted: .char("W")),
            .Char("e"): KeySymbols(normal: .char("e"), shifted: .char("E")),
            .Char("r"): KeySymbols(normal: .char("r"), shifted: .char("R")),
            .Char("t"): KeySymbols(normal: .char("t"), shifted: .char("T")),
            .Char("y"): KeySymbols(normal: .char("y"), shifted: .char("Y")),
            .Char("u"): KeySymbols(normal: .char("u"), shifted: .char("U")),
            .Char("i"): KeySymbols(normal: .char("i"), shifted: .char("I")),
            .Char("o"): KeySymbols(normal: .char("o"), shifted: .char("O")),
            .Char("p"): KeySymbols(normal: .char("p"), shifted: .char("P")),
            .Char("@"): KeySymbols(normal: .char("@")),
            .Char("£"): KeySymbols(normal: .char("£")),
            .Char("*"): KeySymbols(normal: .char("*")),
            
            .RunStop: KeySymbols(both: .key(.keyboardTab)),
            .Char("a"): KeySymbols(normal: .char("a"), shifted: .char("A")),
            .Char("s"): KeySymbols(normal: .char("s"), shifted: .char("S")),
            .Char("d"): KeySymbols(normal: .char("d"), shifted: .char("D")),
            .Char("f"): KeySymbols(normal: .char("f"), shifted: .char("F")),
            .Char("g"): KeySymbols(normal: .char("g"), shifted: .char("G")),
            .Char("h"): KeySymbols(normal: .char("h"), shifted: .char("H")),
            .Char("j"): KeySymbols(normal: .char("j"), shifted: .char("J")),
            .Char("k"): KeySymbols(normal: .char("k"), shifted: .char("K")),
            .Char("l"): KeySymbols(normal: .char("l"), shifted: .char("L")),
            .Char(":"): KeySymbols(normal: .char(":"), shifted: .char("[")),
            .Char(";"): KeySymbols(normal: .char(";"), shifted: .char("]")),
            .Return: KeySymbols(both: .key(.keyboardReturnOrEnter)),
            
            .Char("z"): KeySymbols(normal: .char("z"), shifted: .char("Z")),
            .Char("x"): KeySymbols(normal: .char("x"), shifted: .char("X")),
            .Char("c"): KeySymbols(normal: .char("c"), shifted: .char("C")),
            .Char("v"): KeySymbols(normal: .char("v"), shifted: .char("V")),
            .Char("b"): KeySymbols(normal: .char("b"), shifted: .char("B")),
            .Char("n"): KeySymbols(normal: .char("n"), shifted: .char("N")),
            .Char("m"): KeySymbols(normal: .char("m"), shifted: .char("M")),
            .Char(","): KeySymbols(normal: .char(","), shifted: .char("<")),
            .Char("."): KeySymbols(normal: .char("."), shifted: .char(">")),
            .Char("/"): KeySymbols(normal: .char("/"), shifted: .char("?")),
            
            .Char(" "): KeySymbols(both: .char(" ")),
                                                    
            .CursorUp: KeySymbols(both: .key(.keyboardUpArrow)),
            .CursorLeft: KeySymbols(both: .key(.keyboardLeftArrow)),
            .CursorRight: KeySymbols(both: .key(.keyboardRightArrow)),
            .CursorDown: KeySymbols(both: .key(.keyboardDownArrow)),
        ],
        symbolRemap: [
            ModifiedSymbol(symbol: .char("1"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF1)),
            ModifiedSymbol(symbol: .char("2"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF2)),
            ModifiedSymbol(symbol: .char("3"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF3)),
            ModifiedSymbol(symbol: .char("4"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF4)),
            ModifiedSymbol(symbol: .char("5"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF5)),
            ModifiedSymbol(symbol: .char("6"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF6)),
            ModifiedSymbol(symbol: .char("7"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF7)),
            ModifiedSymbol(symbol: .char("8"), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardF8)),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: .command): ModifiedSymbol(symbol: .key(.keyboardHome)),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.shift, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: .shift),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.control, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: .control),
            ModifiedSymbol(symbol: .key(.keyboardDeleteOrBackspace), modifiers: [.control, .shift, .command]): ModifiedSymbol(symbol: .key(.keyboardHome), modifiers: [.control, .shift]),
            ModifiedSymbol(symbol: .char("`")): ModifiedSymbol(symbol: .key(.keyboardEscape)),
            ModifiedSymbol(symbol: .char("~")): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.shift]),
            ModifiedSymbol(symbol: .char("`"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: .control),
            ModifiedSymbol(symbol: .char("~"), modifiers: .control): ModifiedSymbol(symbol: .key(.keyboardEscape), modifiers: [.control, .shift])
    ])
    
    static var zxSpectrum = KeyboardSymbols(modifierMap: [
        .keyboardLeftShift: .Shift,
        .keyboardRightShift: .Shift,
        .keyboardLeftControl: .SymbolShift,
        .keyboardRightControl: .SymbolShift
    ], keyMap: [
        .Char("1"): KeySymbols(normal: .char("1"), shifted: nil),
        .Char("2"): KeySymbols(normal: .char("2"), shifted: nil),
        .Char("3"): KeySymbols(normal: .char("3"), shifted: nil),
        .Char("4"): KeySymbols(normal: .char("4"), shifted: nil),
        .Char("5"): KeySymbols(normal: .char("5"), shifted: .key(.keyboardLeftArrow)),
        .Char("6"): KeySymbols(normal: .char("6"), shifted: .key(.keyboardDownArrow)),
        .Char("7"): KeySymbols(normal: .char("7"), shifted: .key(.keyboardUpArrow)),
        .Char("8"): KeySymbols(normal: .char("8"), shifted: .key(.keyboardRightArrow)),
        .Char("9"): KeySymbols(normal: .char("9"), shifted: nil),
        .Char("0"): KeySymbols(normal: .char("0"), shifted: .key(.keyboardDeleteOrBackspace)),
        
        .Char("q"): KeySymbols(normal: .char("q"), shifted: .char("Q")),
        .Char("w"): KeySymbols(normal: .char("w"), shifted: .char("W")),
        .Char("e"): KeySymbols(normal: .char("e"), shifted: .char("E")),
        .Char("r"): KeySymbols(normal: .char("r"), shifted: .char("R")),
        .Char("t"): KeySymbols(normal: .char("t"), shifted: .char("T")),
        .Char("y"): KeySymbols(normal: .char("y"), shifted: .char("Y")),
        .Char("u"): KeySymbols(normal: .char("u"), shifted: .char("U")),
        .Char("i"): KeySymbols(normal: .char("i"), shifted: .char("I")),
        .Char("o"): KeySymbols(normal: .char("o"), shifted: .char("O")),
        .Char("p"): KeySymbols(normal: .char("p"), shifted: .char("P")),
        
        .Char("a"): KeySymbols(normal: .char("a"), shifted: .char("A")),
        .Char("s"): KeySymbols(normal: .char("s"), shifted: .char("S")),
        .Char("d"): KeySymbols(normal: .char("d"), shifted: .char("D")),
        .Char("f"): KeySymbols(normal: .char("f"), shifted: .char("F")),
        .Char("g"): KeySymbols(normal: .char("g"), shifted: .char("G")),
        .Char("h"): KeySymbols(normal: .char("h"), shifted: .char("H")),
        .Char("j"): KeySymbols(normal: .char("j"), shifted: .char("J")),
        .Char("k"): KeySymbols(normal: .char("k"), shifted: .char("K")),
        .Char("l"): KeySymbols(normal: .char("l"), shifted: .char("L")),
        .Return: KeySymbols(both: .key(.keyboardReturnOrEnter)),
        
        .Char("z"): KeySymbols(normal: .char("z"), shifted: .char("Z")),
        .Char("x"): KeySymbols(normal: .char("x"), shifted: .char("X")),
        .Char("c"): KeySymbols(normal: .char("c"), shifted: .char("C")),
        .Char("v"): KeySymbols(normal: .char("v"), shifted: .char("V")),
        .Char("b"): KeySymbols(normal: .char("b"), shifted: .char("B")),
        .Char("n"): KeySymbols(normal: .char("n"), shifted: .char("N")),
        .Char("m"): KeySymbols(normal: .char("m"), shifted: .char("M")),
        .Char(" "): KeySymbols(normal: .char(" "), shifted: .key(.keyboardTab))
    ])
}
