/*
 JoystickButtons.swift -- Buttons a Joystick Can Have
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

import Foundation

public struct JoystickButtons: Equatable {
    public var up: Bool
    public var down: Bool
    public var left: Bool
    public var right: Bool
    public var fire: Bool
    public var fire2: Bool
    public var fire3: Bool
    
    public init() {
        up = false
        down = false
        left = false
        right = false
        fire = false
        fire2 = false
        fire3 = false
    }
}
