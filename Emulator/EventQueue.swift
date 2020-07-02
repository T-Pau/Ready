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

public protocol Event {
    var delay: Int { get set }
}

public struct EventQueue<T: Event> {
    private var queue = [T]()
    private var mutex = PThreadMutex()
    
    public init() {
    }

    public mutating func send(event: T) {
        mutex.sync {
            queue.append(event)
        }
    }
    public mutating func process(handler: ((T) -> Void)) {
        mutex.sync {
            var delayed = [T]()
            for event in queue {
                if event.delay > 0 {
                    var newEvent = event
                    newEvent.delay -= 1
                    delayed.append(newEvent)
                }
                else {
                    handler(event)
                }
            }
            queue = delayed
        }
    }
}
