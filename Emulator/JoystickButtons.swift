//
//  JoystickButtons.swift
//  EmulatorInterface
//
//  Created by Dieter Baron on 14.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
