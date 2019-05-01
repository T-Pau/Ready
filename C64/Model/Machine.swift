/*
 Machine.swift -- C64 Hardware Configuration
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

@objc class Machine: NSObject {
    enum ResourceName: String {
        case AutostartPrgMode
        case CartridgeFile
        case DosName1541
        case DosName1541II
        case DosName1581
        case DosName2000
        case DosName4000
        case Drive8IdleMethod
        case Drive8Type
        case Drive9IdleMethod
        case Drive9Type
        case Drive10IdleMethod
        case Drive10Type
        case Drive11IdleMethod
        case Drive11Type
        case DriveSoundEmulation
        case JoyPort1Device
        case JoyPort2Device
        case JoyPort3Device
        case JoyPort4Device
        case KernalName
        case LogFileName
        case Mouse
        case REU
        case REUfilename
        case REUImageWrite
        case REUsize
        case SidModel
        case UserportJoy
        case UserportJoyType
        case VICIIBorderMode
        
        init?(joyDevice port: Int) {
            guard let resourceName = ResourceName(rawValue: "JoyPort\(port)Device") else { return nil }
            self = resourceName
        }

        init?(driveType unit: Int) {
            guard let resourceName = ResourceName(rawValue: "Drive\(unit)Type") else { return nil }
            self = resourceName
        }
    }

    enum ResourceValue {
        case Bool(Bool)
        case Int(Int32)
        case String(String)
    }
    
    weak var vice: Vice?
    
    var specification: MachineSpecification
    
    var inputMapping = InputMapping()
    
    var resources = [ResourceName: ResourceValue]()
    
    @objc var autostart: Bool

    var cartridgeImage: CartridgeImage?
    var ramExpansionUnit: RamExpansionUnit?
    var programFile: ProgramFile?
    var diskImages = [DiskImage]()
    var tapeImages = [TapeImage]()

    var diskDrives = [DiskDrive]()
    var controllers = [Controller]()
    var cassetteDrive: CasstteDrive
    var cartridges = [Cartridge]()
    var userPortModule: UserPortModule?
    var userPortControllers = [Controller]()
    
    var hasMouse: Bool {
        return controllers.contains(where: { $0.inputType == .mouse })
    }
    
    private static let controllerKeys: [MachineConfig.Key] = [ .controlPort1, .controlPort2 ]

    private static let driveKeys: [MachineConfig.Key] = [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11 ]

    convenience override init() {
        self.init(specification: Defaults.standard.machineSpecification)
    }
    
    init(specification: MachineSpecification) {
        self.specification = specification
        autostart = true
        
        diskDrives = specification.diskDrives
        cassetteDrive = specification.cassetteDrive
        userPortModule = specification.userPortModule
        cartridges = specification.cartridges
        
        if let userJoystickType = userPortModule?.viceJoystickType {
            resources[.UserportJoy] = .Bool(true)
            resources[.UserportJoyType] = .Int(userJoystickType.rawValue)
        }

        super.init()

        update(controllers: specification.controllers, isUserPort: false)
        update(controllers: specification.userPortControllers, isUserPort: true)
    }
    
    func change(borderMode: MachineConfig.BorderMode) {
        guard borderMode != specification.borderMode else { return }
        viceThread?.newBorderMode = borderMode.cValue
        specification.borderMode = borderMode
        do {
            try specification.save()
        }
        catch { }
    }
    
    func connect(inputDevice: InputDevice) {
        inputDevice.delegate = self
        inputMapping.add(device: inputDevice)
    }
    
    @objc func viceSetResources() {
        for (index, drive) in diskDrives.enumerated() {
            if let name = ResourceName(rawValue: "Drive\(index + 8)Type") {
                viceSetResource(name: name, value: .Int(drive.viceType.rawValue))
            }
        }
        
        for cartridge in cartridges {
            for (name, value) in cartridge.resources {
                NSLog("cartridge resource \(name) -> \(value)")
                viceSetResource(name: name, value: value)
            }
        }
        
        for (name, value) in resources {
            viceSetResource(name: name, value: value)
        }
    }
        
    func viceSetResource(name: ResourceName, value: ResourceValue) {
        switch value {
        case .Bool(let value):
            resources_set_int(name.rawValue, value ? 1 : 0)
        case .Int(let value):
            resources_set_int(name.rawValue, value)
        case .String(let value):
            resources_set_string(name.rawValue, value)
        }
    }
    
    func setResource(name: ResourceName, value: ResourceValue) {
        resources[name] = value
        vice?.setResource(name: name, value: value)
    }
    
    func mountDisks() {
        let ports = specification.computer.ports
        
        // TODO: use MachineSpecification.automount
        for disk in diskImages {
            var mountable = false
            for (index, drive) in diskDrives.enumerated() {
                if drive.supports(image: disk) {
                    if !mountable && drive.image == nil {
                        diskDrives[index].image = disk
                    }
                    mountable = true
                    break
                }
            }
            if !mountable {
                guard var newDrive = DiskDrive.getDriveSupporting(image: disk) else { continue }
                for (index, drive) in diskDrives.enumerated() {
                    if ports.contains(Machine.driveKeys[index]) && specification.string(for: Machine.driveKeys[index]) == "auto" {
                        if !drive.hasStatus {
                            newDrive.image = disk
                            diskDrives[index] = newDrive
                            break
                        }
                        if let image = drive.image, newDrive.supports(image: image) {
                            newDrive.image = image
                            diskDrives[index] = newDrive
                            break
                        }
                    }
                }
            }
        }
        
        if cartridges.isEmpty { // TODO: contains auto
            if let cartridge = cartridgeImage {
                cartridges.append(cartridge)
            }
            else if let cartridge = ramExpansionUnit {
                cartridges.append(cartridge)
            }
        }
        
        if !tapeImages.isEmpty && specification.string(for: .cassetteDrive) == "auto" {
            cassetteDrive = CasstteDrive.drives.sorted(by: { $0.priority > $1.priority })[0]
        }
    }
}

extension Machine {
    func update(specification newSpecification: MachineSpecification) {
        let oldSpecification = specification
        specification = newSpecification
        do {
            try specification.save()
        }
        catch { }

        for key in oldSpecification.differences(to: newSpecification) {
            switch key {
            case .controlPort1, .controlPort2:
                update(controllers: newSpecification.controllers, isUserPort: false)
                
            case .userPortJoystick1, .userPortJoystick2:
                update(controllers: newSpecification.userPortControllers, isUserPort: true)

            default:
                // TODO
                break
            }
        }
    }
    
    private func update(controllers newControllers: [Controller], isUserPort: Bool) {
        let oldControllers = isUserPort ? userPortControllers : controllers
        for index in (0 ..< min(oldControllers.count, newControllers.count)) {
            let oldController = oldControllers[index]
            let newController = newControllers[index]
            let port = index + 1
            
            if oldController != newController {
                replace(oldController: oldController, newController: newController, port: port, isUserPort: isUserPort)
                updateVice(controller: newController, port: port, isUserPort: isUserPort)
            }
        }
        if oldControllers.count > newControllers.count {
            for index in (newControllers.count ..< oldControllers.count) {
                let port = index + 1
                remove(controller: oldControllers[index], port: port, isUserPort: isUserPort)
                updateVice(controller: nil, port: port, isUserPort: isUserPort)
            }
        }
        if newControllers.count > oldControllers.count {
            for index in (oldControllers.count ..< newControllers.count) {
                let port = index + 1
                add(controller: newControllers[index], port: port, isUserPort: isUserPort)
                updateVice(controller: newControllers[index], port: port, isUserPort: isUserPort)
            }
        }
        if isUserPort {
            userPortControllers = newControllers
        }
        else {
            controllers = newControllers
        }
    }
    
    private func remove(controller: Controller, port: Int, isUserPort: Bool) {
        inputMapping.remove(ports: inputPorts(for: controller, port: port, isUserPort: isUserPort))
    }
    
    private func add(controller: Controller, port: Int, isUserPort: Bool) {
        inputMapping.add(ports: inputPorts(for: controller, port: port, isUserPort: isUserPort))
    }
    
    private func replace(oldController: Controller, newController: Controller, port: Int, isUserPort: Bool) {
        let oldPorts = inputPorts(for: oldController, port: port, isUserPort: isUserPort)
        let newPorts = inputPorts(for: newController, port: port, isUserPort: isUserPort)
        
        for index in (0 ..< min(oldPorts.count, newPorts.count)) {
            inputMapping.replace(oldPorts[index], with: newPorts[index])
        }
        if oldPorts.count > newPorts.count {
            inputMapping.remove(ports: Array(oldPorts[newPorts.count ..< oldPorts.count]))
        }
        if newPorts.count > oldPorts.count {
            inputMapping.add(ports: Array(newPorts[oldPorts.count ..< newPorts.count]))
        }
    }
    
    private func updateVice(controller: Controller?, port: Int, isUserPort: Bool) {
        let vicePort = port + (isUserPort ? 2 : 0)
        guard let key = Machine.ResourceName(joyDevice: vicePort) else { return }
        setResource(name: key, value: .Int((controller?.viceType ?? .none).rawValue))
    }
    
    private func inputPorts(for controller: Controller, port: Int, isUserPort: Bool) -> [InputPort] {
        var inputPorts = [InputPort]()
        
        switch controller.inputType {
        case .none:
            break
            
        case .joystick, .lightGun, .lightPen, .mouse:
            inputPorts.append(InputPort(port: port, isUserPort: isUserPort, controller: controller))
        case .paddle:
            break // TODO
        }

        return inputPorts
    }
}

extension Machine: InputDeviceDelegate {
    func inputDevice(_ device: InputDevice, joystickChanged buttons: JoystickButtons) {
        guard let port = port(for: device) else { return }
        vice?.joystick(port.vicePort, buttons: buttons)
    }
    
    func inputDevice(_ device: InputDevice, mouseMoved distance: CGPoint) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(moved: distance)
    }
    
    func inputDevice(_ device: InputDevice, mouseButtonPressed index: Int) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(pressed: index)
    }
    
    func inputDevice(_ device: InputDevice, mouseButtonReleased index: Int) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(release: index)
    }
    
    func inputDevice(_ device: InputDevice, lightPenMoved position: CGPoint?, size: CGSize, button1 rawButton1: Bool, button2 rawButton2: Bool) {
        guard let controller = port(for: device)?.controller else { return }

        var button1 = false
        var button2 = false
        
        if controller.viceType.lightPenNeedsButtonOnTouch {
            button1 = position != nil
            if controller.numberOfButtons >= 1 {
                button2 = rawButton1
            }
        }
        else {
            if controller.numberOfButtons >= 1 {
                button1 = rawButton1
            }
            if controller.numberOfButtons >= 2 {
                button2 = rawButton2
            }
        }

        vice?.lightPen(moved: position, size: size, button1: button1, button2: button2)
    }
    
    func inputDeviceDidDisconnect(_ inputDevice: InputDevice) {
        inputMapping.remove(device: inputDevice)
    }
    
    private func port(for device: InputDevice) -> InputPort? {
        guard let pairing = inputMapping[device] else { return nil }
        pairing.lastActivity = Date()
        return pairing.port
    }
}
