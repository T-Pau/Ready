/*
 EmulatorDelegate.swift -- Delegate to Emulator Core
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

// TODO: move elsewhere
public enum DatasetteControlStatus: Int {
    case stop = 0
    case start = 1
    case forward = 2
    case rewind = 3
    case record = 4
    case reset = 5
    case resetCounter = 6
}


public protocol EmulatorDelegate {
    func updateDriveStatus(unit: Int, track: Double, led1Intensity: Double, led2Intensity: Double)
    
    func updateTapeStatus(controlStatus: DatasetteControlStatus, isMotorOn: Bool, counter: Double)
}
