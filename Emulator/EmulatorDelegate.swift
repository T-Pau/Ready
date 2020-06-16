//
//  ViceThreadDelegate.swift
//  C64
//
//  Created by Dieter Baron on 14.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
