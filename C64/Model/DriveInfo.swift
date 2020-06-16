/*
 DriveInfo.swift -- Configure DriveStatusView
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
import C64UIComponents
import Emulator

extension DriveStatusView {
    func configureFrom(drive: DiskDrive) {
        for index in (0 ..< numberOfLeds) {
            if index < drive.leds.count {
                ledViews[index].isRound = drive.leds[index].isRound
                ledViews[index].darkColor = drive.leds[index].darkColor
                ledViews[index].lightColor = drive.leds[index].lightColor
                ledViews[index].isHidden = false
            }
            else {
                ledViews[index].isHidden = true
            }
        }
        
        backgroundColor = drive.caseColor
        textColor = drive.textColor
        trackView.isDoubleSided = drive.isDoubleSided

        configureFrom(image: drive.image)
    }
    
    func configureFrom(image: DiskImage?) {
        if let image = image {
            trackView.tracks = image.tracks
            if trackView.isDoubleSided && !image.mediaType.isDoubleSided {
                // single sided disk in double sided drive: only first side is used
                trackView.tracks *= 2
            }
        }
        else {
            trackView.tracks = 35
        }
        trackView.currentTrack = 1
    }
}
