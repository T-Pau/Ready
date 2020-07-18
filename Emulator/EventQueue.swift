/*
 EventQueue.swift -- Queue input events to emulator thread
 Copyright (C) 2020 Dieter Baron
 
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

public enum Event {
    case attach(unit: Int, image: DiskImage?)
    case freeze
    case joystick(port: Int, buttons: JoystickButtons, oldButtons: JoystickButtons)
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
