/*
 InputMapping.swift -- Mapping Physical Input Devices to Emulated Controllers
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

struct InputPort: Hashable {
    var name: String
    var fullName: String
    var controller: Controller
    var priority: Int
    var playerIndex: Int
    var vicePort: Int
    var orderNumber: Double
    
    var orderFraction: Double {
        return orderNumber - Double(vicePort)
    }
    
    var icon: UIImage? {
        return controller.portIcon
    }
    var inputType: Controller.InputType {
        return controller.inputType
    }

    init(port: Int, isUserPort: Bool, orderFraction: Double = 0, controller: Controller) {
        if isUserPort {
            name = "Userport \(port)"
            fullName = "Userport Joystick \(port)"
            vicePort = port + 2
        }
        else {
            name = "Controller \(port)"
            fullName = "Controller Port \(port)"
            vicePort = port
        }
        playerIndex = vicePort
        self.controller = controller
        if isUserPort {
            priority = MachinePartLowPriority - port
        }
        else {
            if controller.inputType == .mouse || controller.inputType == .lightGun || controller.inputType == .lightPen {
                priority = MachinePartHighPriority + 10
            }
            else if port == 2 {
                priority = MachinePartHighPriority
            }
            else {
                priority = MachinePartNormalPriority - port
            }
        }
        orderNumber = Double(vicePort) + orderFraction
    }
    
    func hash(into hasher: inout Hasher) {
        fullName.hash(into: &hasher)
    }
}

class InputPairing {
    var port: InputPort
    var device: InputDevice
    var lastActivity: Date?
    var isManual: Bool
    
    init(port: InputPort, device: InputDevice, isManual: Bool) {
        self.port = port
        self.device = device
        self.isManual = isManual
    }
}

class InputMapping {
    var unmappedPorts = Set<InputPort>()
    var unmappedDevices = Set<InputDevice>()
    var mappedPorts = [InputPort : InputPairing]()
    var mappedDevices = [InputDevice: InputPairing]()
    
    private var _devices: [InputDevice]?
    private func invalidateDevices() {
        _devices = nil
    }
    
    var devices: [InputDevice] {
        if let devices = _devices {
            return devices
        }
        var devices = Array(unmappedDevices)
        devices.append(contentsOf: mappedDevices.keys)
        devices.sort(by: {
            if $0.priority == $1.priority {
                return $0.fullName < $1.fullName
            }
            else {
                return $0.priority > $1.priority
            }
        })
        
        _devices = devices
        return devices
    }
    
    private var _ports: [InputPort]?
    private func invalidatePorts() {
        _ports = nil
    }
    
    var ports: [InputPort] {
        if let ports = _ports {
            return ports
        }
        var ports = Array(unmappedPorts)
        ports.append(contentsOf: mappedPorts.keys)
        ports.sort(by: { $0.orderNumber < $1.orderNumber })
        
        _ports = ports
        return ports
    }
    
    func add(device: InputDevice) {
        unmappedDevices.insert(device)
        invalidateDevices()
        automap()
    }
    
    func remove(device: InputDevice) {
        cancel(pairing: mappedDevices[device])
        unmappedDevices.remove(device)
        invalidateDevices()
        automap()
    }
    
    func add(ports: [InputPort]) {
        guard !ports.isEmpty else { return }

        unmappedPorts.formUnion(ports)
        invalidatePorts()
        automap()
    }
    
    func remove(ports: [InputPort]) {
        guard !ports.isEmpty else { return }

        for port in ports {
            cancel(pairing: mappedPorts[port])
        }
        unmappedPorts.subtract(ports)
        invalidatePorts()
        automap()
    }
    
    func replace(_ oldPort: InputPort, with newPort: InputPort) {
        if let pairing = mappedPorts[oldPort] {
            if pairing.isManual && pairing.device.supports(inputType: newPort.inputType) {
                NSLog("replacing \(oldPort.name) (\(oldPort.controller.fullName), type \(oldPort.controller.inputType)) with \(oldPort.name) (\(newPort.controller.fullName), type \(newPort.controller.inputType))")
                pairing.port = newPort
                mappedPorts[oldPort] = nil
                mappedPorts[newPort] = pairing
                pairing.device.currentMode = newPort.inputType
                pairing.device.deviceConfig = newPort.controller.deviceConfig
            }
            else {
                cancel(pairing: pairing)
                unmappedPorts.remove(oldPort)
                unmappedPorts.insert(newPort)
                automap()
            }
        }
        else {
            unmappedPorts.remove(oldPort)
            unmappedPorts.insert(newPort)
            automap()
        }
        invalidatePorts()
    }
    
    func pair(port: InputPort, device: InputDevice, isManual: Bool = true) {
        guard device.supports(inputType: port.inputType) else { return }
        if isManual {
            guard mappedPorts[port]?.device != device else { return }
            NSLog("manually pairing \(device.fullName) to \(port.fullName) type \(port.inputType)")
            cancel(pairing: mappedPorts[port])
            cancel(pairing: mappedDevices[device])
        }

        let pairing = InputPairing(port: port, device: device, isManual: isManual)
        
        NSLog("connecting \(device.fullName) to \(port.fullName) (\(port.controller.fullName), type \(port.inputType))")

        mappedPorts[port] = pairing
        mappedDevices[device] = pairing
        unmappedPorts.remove(port)
        unmappedDevices.remove(device)
        
        device.currentMode = port.inputType
        device.playerIndex = port.playerIndex
        device.deviceConfig = port.controller.deviceConfig
        
        if isManual {
            automap(cancelAutomatic: false)
        }
    }
    
    private func automap(cancelAutomatic: Bool = true) {
        if cancelAutomatic {
            // TODO: keep recently used automatic pairings
            for pairing in mappedPorts.values.filter({ $0.isManual == false }) {
                cancel(pairing: pairing)
            }
        }

        for port in unmappedPorts.sorted(by: { $0.priority > $1.priority }) {
            let devices = unmappedDevices.filter({ $0.supports(inputType: port.inputType) }).sorted(by: { $0.priority > $1.priority })
            if !devices.isEmpty {
                pair(port: port, device: devices[0], isManual: false)
            }
        }
    }
    
    private func cancel(pairing: InputPairing?) {
        guard let pairing = pairing else { return }
        
        NSLog("disconnecting \(pairing.device.fullName) from \(pairing.port.fullName)")
        mappedPorts[pairing.port] = nil
        mappedDevices[pairing.device] = nil
        unmappedPorts.insert(pairing.port)
        unmappedDevices.insert(pairing.device)
        
        pairing.device.currentMode = .none
        pairing.device.playerIndex = nil
    }
    
    
    subscript(port: InputPort) -> InputPairing? {
        return mappedPorts[port]
    }
    
    subscript(device: InputDevice) -> InputPairing? {
        return mappedDevices[device]
    }
}
