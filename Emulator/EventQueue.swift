/*
 EventQueue.swift -- Queue input events to emulator thread
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

public enum Event {
    case attach(unit: Int, image: DiskImage?)
    case freeze
    case joystick(port: Int, buttons: JoystickButtons)
    case key(_ key: Key, pressed: Bool)
    case mouseButton(button: Int, pressed: Bool)
    case playPause(_ running: Bool)
    case quit
    case reset
    case setResource(key: MachineOld.ResourceName, value: MachineOld.ResourceValue)
}


struct EventQueue {
    private struct QueueEntry {
        var event: Event
        var delay: Int
    }

    private var queue = [QueueEntry]()
    private var mutex = PThreadMutex()
    
    mutating func send(event: Event, delay: Int) {
        mutex.sync {
            queue.append(QueueEntry(event: event, delay: delay))
        }
    }
    
    mutating func process(handler: ((Event) -> Bool)) -> Bool {
        var continuePorcessing = true

        mutex.sync {
            var delayed = [QueueEntry]()
            
            for entry in queue {
                if entry.delay > 0 {
                    delayed.append(QueueEntry(event: entry.event, delay: entry.delay - 1))
                }
                else {
                    if (!handler(entry.event)) {
                        continuePorcessing = false
                    }
                }
            }
            queue = delayed
        }
        
        return continuePorcessing
    }
}
