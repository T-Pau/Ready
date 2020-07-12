/*
 ConnectorType.swift -- Type of Connector
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
    case commodoreCassette
    case commodoreIEC
    case commodoreTapePort
    case ide
    case kempstonMouse
    case plus4TapePort
    case plus4ExpansionPort
    case atariSio
    case spectrumExpansionPort
    case vic20ExpansionPort
    case videoComponent
    case videoRGBi
        
    case none // used for dummy parts
    
    case media(_ type: MediaType)
}
