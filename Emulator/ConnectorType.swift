/*
 ConnectorType.swift -- Type of Connector
 Copyright (C) 2019 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import Foundation
import RetroMedia

// This is a list of all connectors used by supported devices.
// It is used to filter the list of devices that can be connected to a port.

public enum ConnectorType: Hashable {
    case atariJoystick
    case atariJoystickAnalog
    case audioJackTape
    case c64ExpansionPort
    case c64JoystickLightpen
    case c64UserPort
    case computer // used for computers, which are the root of the device tree
    case commodoreIEC
    case commodoreTapePort
    case floppy3_5DoubleDensityDoubleSidedCmd // CMD FD
    case floppy3_5DoubleDensityDoubleSidedCommodore // 1581
    case floppy3_5ExtendedDensityDoubleSidedCmd // CMD FD
    case floppy3_5HighDensityDoubleSidedCmd // CMD FD-4000
    case floppy5_25SingleDensityDoubleSidedCommodore // 1571
    case floppy5_25SingleDensitySingleSidedCommodore // 1541, 1551, 1570
    case ide
    case kempstonMouse
    case programCommodore
    case plus4TapePort
    case plus4ExpansionPort
    case atariSio
    case sdCard
    case spectrumExpansionPort
    case supernesJoystick
    case tapeCommodore
    case tapeSpectrum
    case vic20ExpansionPort
    case videoComponent
    case videoRGBi
    case videoVGA
        
    case none // used for dummy parts
    
    case media(_ type: MediaType)
}
