/*
 Keyboard.swift -- Layout of Virtual Keyboards
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

import Foundation


struct Keyboard: Codable {
    var imageName: String
    var lockIsShift: Bool
    
    var rows: [CGFloat]
    
    var topHalfLeft: CGFloat
    var topHalfRight: CGFloat
    var bottomHalfLeft: CGFloat
    var bottomHalfRight: CGFloat
    
    var functionKeysLeft: CGFloat
    var functionKeysRight: CGFloat
    
    var spaceLeft: CGFloat
    var spaceRight: CGFloat
    
    var ctrlRight: CGFloat
    var restoreLeft: CGFloat
    var returnLeft: CGFloat
    var leftShiftLeft: CGFloat
    var leftShiftRight: CGFloat
    var rightShiftLeft: CGFloat
    var rightShiftRight: CGFloat

    private static var keyboards = [
        "C64 Keyboard": Keyboard(imageName: "C64 Keyboard",
                                 lockIsShift: true,
                                 rows: [ 69, 257, 446, 635, 824, 1012 ],
                                 topHalfLeft: 100,
                                 topHalfRight: 3034,
                                 bottomHalfLeft: 49,
                                 bottomHalfRight: 2984,
                                 functionKeysLeft: 3115,
                                 functionKeysRight: 3415,
                                 spaceLeft: 544,
                                 spaceRight: 2207,
                                 ctrlRight: 330,
                                 restoreLeft: 2778,
                                 returnLeft: 2681,
                                 leftShiftLeft: 283,
                                 leftShiftRight: 559,
                                 rightShiftLeft: 2341,
                                 rightShiftRight: 2681),
        "C64 Keyboard Japanese": Keyboard(imageName: "C64 Keyboard Japanese",
                                 lockIsShift: false,
                                 rows: [ 70, 263, 457, 650, 843, 1027 ],
                                 topHalfLeft: 97,
                                 topHalfRight: 3148,
                                 bottomHalfLeft: 56,
                                 bottomHalfRight: 3098,
                                 functionKeysLeft: 3219,
                                 functionKeysRight: 3532,
                                 spaceLeft: 573,
                                 spaceRight: 2288,
                                 ctrlRight: 370,
                                 restoreLeft: 2869,
                                 returnLeft: 2723,
                                 leftShiftLeft: 231,
                                 leftShiftRight: 524,
                                 rightShiftLeft: 2436,
                                 rightShiftRight: 2723),
        "C64C Keyboard": Keyboard(imageName: "C64C Keyboard",
                                  lockIsShift: true,
                                  rows: [ 69, 257, 446, 635, 824, 1012 ],
                                  topHalfLeft: 100,
                                  topHalfRight: 3034,
                                  bottomHalfLeft: 49,
                                  bottomHalfRight: 2984,
                                  functionKeysLeft: 3115,
                                  functionKeysRight: 3415,
                                  spaceLeft: 544,
                                  spaceRight: 2207,
                                  ctrlRight: 330,
                                  restoreLeft: 2778,
                                  returnLeft: 2681,
                                  leftShiftLeft: 283,
                                  leftShiftRight: 559,
                                  rightShiftLeft: 2341,
                                  rightShiftRight: 2681),
        "C64C New Keyboard": Keyboard(imageName: "C64C New Keyboard",
                                  lockIsShift: true,
                                  rows: [ 50, 233, 402, 571, 746, 917 ],
                                  topHalfLeft: 97,
                                  topHalfRight: 2853,
                                  bottomHalfLeft: 63,
                                  bottomHalfRight: 2805,
                                  functionKeysLeft: 2952,
                                  functionKeysRight: 3222,
                                  spaceLeft: 540,
                                  spaceRight: 2076,
                                  ctrlRight: 348,
                                  restoreLeft: 2603,
                                  returnLeft: 2469,
                                  leftShiftLeft: 215,
                                  leftShiftRight: 477,
                                  rightShiftLeft: 2215,
                                  rightShiftRight: 2476),
        "Max Keyboard": Keyboard(imageName: "Max Keyboard",
                                  lockIsShift: true,
                                  rows: [ 32, 217, 400, 588, 780, 972 ],
                                  topHalfLeft: 39,
                                  topHalfRight: 3058,
                                  bottomHalfLeft: 39,
                                  bottomHalfRight: 3058,
                                  functionKeysLeft: 3116,
                                  functionKeysRight: 3408,
                                  spaceLeft: 506,
                                  spaceRight: 2393,
                                  ctrlRight: 323,
                                  restoreLeft: 2762,
                                  returnLeft: 2671,
                                  leftShiftLeft: 230,
                                  leftShiftRight: 517,
                                  rightShiftLeft: 2390,
                                  rightShiftRight: 2671),
        "PET Style Keyboard": Keyboard(imageName: "PET Style Keyboard",
                                 lockIsShift: true,
                                 rows: [ 44, 194, 338, 485, 631, 772 ],
                                 topHalfLeft: 66,
                                 topHalfRight: 2388,
                                 bottomHalfLeft: 37,
                                 bottomHalfRight: 2351,
                                 functionKeysLeft: 2441,
                                 functionKeysRight: 2673,
                                 spaceLeft: 434,
                                 spaceRight: 1744,
                                 ctrlRight: 275,
                                 restoreLeft: 2183,
                                 returnLeft: 2071,
                                 leftShiftLeft: 175,
                                 leftShiftRight: 391,
                                 rightShiftLeft: 1853,
                                 rightShiftRight: 2071),
        "SX64 Keyboard": Keyboard(imageName: "SX64 Keyboard",
                                  lockIsShift: true,
                                  rows: [ 69, 269, 469, 669, 869, 1060 ],
                                  topHalfLeft: 90,
                                  topHalfRight: 3229,
                                  bottomHalfLeft: 90,
                                  bottomHalfRight: 3229,
                                  functionKeysLeft: 3313,
                                  functionKeysRight: 3625,
                                  spaceLeft: 763,
                                  spaceRight: 2538,
                                  ctrlRight: 395,
                                  restoreLeft: 2917,
                                  returnLeft: 2837,
                                  leftShiftLeft: 286,
                                  leftShiftRight: 580,
                                  rightShiftLeft: 2543,
                                  rightShiftRight: 2837),
        "VIC-20 Keyboard": Keyboard(imageName: "VIC-20 Keyboard",
                                 lockIsShift: true,
                                 rows: [ 69, 257, 446, 635, 824, 1012 ],
                                 topHalfLeft: 100,
                                 topHalfRight: 3034,
                                 bottomHalfLeft: 49,
                                 bottomHalfRight: 2984,
                                 functionKeysLeft: 3115,
                                 functionKeysRight: 3415,
                                 spaceLeft: 544,
                                 spaceRight: 2207,
                                 ctrlRight: 330,
                                 restoreLeft: 2778,
                                 returnLeft: 2681,
                                 leftShiftLeft: 283,
                                 leftShiftRight: 559,
                                 rightShiftLeft: 2341,
                                 rightShiftRight: 2681),
    ]
    
    static func keyboard(named name: String) -> Keyboard? {
        return keyboards[name]
    }
}
