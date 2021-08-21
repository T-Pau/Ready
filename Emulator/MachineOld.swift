/*
 MachineConfigOld.swift -- C64 Hardware Configuration
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
import C64UIComponents

public class MachineOld {
    public enum ResourceName: String {
        case AutostartPrgMode
        case C128ColumnKey
        case CartridgeFile
        case CartridgeType
        case CPMCart
        case Datasette
        case DatasetteSound
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
        case GenericCartridgeFile2000
        case GenericCartridgeFile4000
        case GenericCartridgeFile6000
        case GenericCartridgeFileA000
        case GenericCartridgeFileB000
        case GMod2EEPROMImage
        case GMod2EEPROMRW
        case IDE64version
        case IDE64Image1
        case IDE64Image2
        case IDE64Image3
        case IDE64Image4
        case IDE64RTCSave
        case JoyPort1Device
        case JoyPort2Device
        case JoyPort3Device
        case JoyPort4Device
        case KernalName
        case LogFileName
        case MachineType
        case MIDIEnable
        case MIDIMode
        case Mouse
        case RAMBlock0
        case RAMBlock1
        case RAMBlock2
        case RAMBlock3
        case RAMBlock5
        case RAMLINK
        case RAMLINKfilename
        case RAMLINKImageWrite
        case RAMLINKmode
        case RAMLINKsize
        case REU
        case REUfilename
        case REUImageWrite
        case REUsize
        case SidModel
        case UserportJoy
        case UserportJoyType
        case VICIIBorderMode
        
        public init?(vic20CartridgeAddress address: Int) {
            switch address {
            case 0x2000:
                self = .GenericCartridgeFile2000
            case 0x4000:
                self = .GenericCartridgeFile4000
            case 0x6000:
                self = .GenericCartridgeFile6000
            case 0xA000:
                self = .GenericCartridgeFileA000
            case 0xB000:
                self = .GenericCartridgeFileB000
            default:
                return nil
            }
        }
        
        public init?(driveType unit: Int) {
            guard let resourceName = ResourceName(rawValue: "Drive\(unit)Type") else { return nil }
            self = resourceName
        }
        
        public init?(ide64Image unit: Int) {
            guard let resourceName = ResourceName(rawValue: "IDE64Image\(unit)") else { return nil }
            self = resourceName
        }
        
        public init?(joyDevice port: Int) {
            guard let resourceName = ResourceName(rawValue: "JoyPort\(port)Device") else { return nil }
            self = resourceName
        }
    }

    public enum ResourceValue {
        case Bool(Bool)
        case Int(Int32)
        case String(String)
    }
    
    public weak var vice: Emulator?
    
    public var specification: MachineSpecification
    
    public var inputMapping = InputMapping()
    
    public var resources = [ResourceName: ResourceValue]()
    
    public var autostart: Bool

    public var directoryURL: URL?

    public var mediaItems = [MediaItem]()

    public var diskDrives = [DiskDrive]()
    public var controllers = [Controller]()
    public var cassetteDrive: CasstteDrive
    public var cartridges = [Cartridge]()
    public var userPortModule: UserPortModule?
    public var userPortControllers = [Controller]()
    
    public var hasMouse: Bool {
        return controllers.contains(where: { $0.inputType == .mouse })
    }
    
    private static let controllerKeys: [MachineConfig.Key] = [ .controlPort1, .controlPort2 ]

    private static let driveKeys: [MachineConfig.Key] = [ .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11 ]

    public convenience init() {
        self.init(specification: Defaults.standard.machineSpecification)
    }
    
    public init(specification: MachineSpecification) {
        self.specification = specification
        autostart = true
        
        diskDrives = specification.diskDrives
        cassetteDrive = specification.cassetteDrive
        userPortModule = specification.userPortModule
        
        if let info = specification.computer.emulatorInfo(for: .c128) as? ViceEmulatorInfo {
            resources.merge(info.resources) {(_, new) in new}
        }
        
        if let userJoystickType = userPortModule?.getViceJoystickType(for: specification) {
            resources[.UserportJoy] = .Bool(true)
            resources[.UserportJoyType] = .Int(userJoystickType.rawValue)
        }

        update(controllers: specification.controllers, isUserPort: false)
        update(controllers: specification.userPortControllers, isUserPort: true)
    }
    
    public func change(borderMode: MachineConfigOld.BorderMode) {
        guard borderMode != specification.borderMode else { return }
        vice?.set(borderMode: borderMode)
        specification.borderMode = borderMode
        do {
            try specification.save()
        }
        catch { }
    }
    
    public func change(displayedScreens: MachineConfigOld.DisplayedScreens) {
        guard displayedScreens != specification.displayedScreens else { return }
        vice?.set(displayedScreens: displayedScreens)
        specification.displayedScreens = displayedScreens
        do {
            try specification.save()
        }
        catch { }
    }
    
    public func connect(inputDevice: InputDevice) {
        inputDevice.delegate = self
        inputMapping.add(device: inputDevice)
    }
    
    public func viceSetResources() {
        for (index, drive) in diskDrives.enumerated() {
            if let name = ResourceName(driveType: index + 8) {
                viceSetResource(name: name, value: .Int(drive.viceType.rawValue))
            }
        }
        
        for cartridge in cartridges {
            for (name, value) in cartridge.resources {
                NSLog("cartridge resource \(name) -> \(value)")
                viceSetResource(name: name, value: value)
            }
        }
        
        for (index, disk) in mediaItems.filter({ $0.connector == .ide }).enumerated() {
            if let name = ResourceName(ide64Image: index + 1),
                let url = disk.url {
                viceSetResource(name: name, value: .String(url.path))
            }
        }
        
        for (name, value) in resources {
            viceSetResource(name: name, value: value)
        }
    }
        
    public func viceSetResource(name: ResourceName, value: ResourceValue) {
        vice?.setResourceNow(name: name, value: value)
    }
    
    public func setResource(name: ResourceName, value: ResourceValue) {
        resources[name] = value
        vice?.setResource(name: name, value: value)
    }
    
    public func mountDisks() {
        // TODO: use MachineSpecification.automount
        for disk in mediaItems.compactMap({ $0 as? DiskImage }) {
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
                    if specification.computer.has(port: MachineOld.driveKeys[index]) && specification.string(for: MachineOld.driveKeys[index]) == "auto" {
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
        
        cartridges = specification.cartridges(for: self)
        // TODO: add cartridge from mediaItems
        // TODO: remove all but first main slot cartridge

        if mediaItems.contains(where: { $0.connector == .tapeCommodore || $0.connector == .tapeSpectrum }) && specification.string(for: .cassetteDrive) == "auto" {
            cassetteDrive = CasstteDrive.drives.parts.sorted(by: { $0.priority > $1.priority })[0] as! CasstteDrive
        }
    }
    
    public var hasFreeze: Bool {
        guard let cartridge = mediaItems.first(where: { $0 as? Cartridge != nil }) as? Cartridge else { return false }
        return cartridge.hasFreeze
    }
}

extension MachineOld {
    public func update(specification newSpecification: MachineSpecification) {
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

            case .singularAdapterMode:
                if userPortModule?.moduleType == .singularAdapter {
                    if let type = specification.singularAdapterMode.viceJoystickType {
                        viceSetResource(name: .UserportJoy, value: .Bool(true))
                        viceSetResource(name: .UserportJoyType, value: .Int(type.rawValue))
                    }
                    else {
                        viceSetResource(name: .UserportJoy, value: .Bool(false))
                    }
                    update(controllers: newSpecification.userPortControllers, isUserPort: true)
                }

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
        guard let key = MachineOld.ResourceName(joyDevice: vicePort) else { return }
        setResource(name: key, value: .Int((controller?.viceType ?? .none).rawValue))
    }
    
    private func inputPorts(for controller: Controller, port: Int, isUserPort: Bool) -> [InputPort] {
        var inputPorts = [InputPort]()
        
        switch controller.inputType {
        case .none:
            break
            
        case .joystick, .lightGun, .lightPen, .mouse:
            let priorityPort = specification.computer.model.type == .spectrum ? 1 : 2
            inputPorts.append(InputPort(port: port, isUserPort: isUserPort, controller: controller, priority: port == priorityPort))
        case .paddle:
            if !isUserPort {
                inputPorts.append(InputPort(port: port, isUserPort: false, subPort: 1, controller: controller))
                inputPorts.append(InputPort(port: port, isUserPort: false, subPort: 2, controller: controller))
            }
        }

        return inputPorts
    }
}

extension MachineOld: InputDeviceDelegate {
    public func inputDevice(_ device: InputDevice, joystickChanged buttons: JoystickButtons) {
        guard let port = port(for: device) else { return }
        vice?.joystick(port.vicePort, buttons: buttons)
    }
    
    public func inputDevice(_ device: InputDevice, mouseMoved distance: CGPoint) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(moved: distance)
    }
    
    public func inputDevice(_ device: InputDevice, mouseButtonPressed index: Int) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(pressed: index)
    }
    
    public func inputDevice(_ device: InputDevice, mouseButtonReleased index: Int) {
        guard let _ = port(for: device) else { return }
        vice?.mouse(release: index)
    }
    
    public func inputDevice(_ device: InputDevice, lightPenMoved position: CGPoint?, size: CGSize, button1 rawButton1: Bool, button2 rawButton2: Bool) {
        guard let controller = port(for: device)?.controller else { return }

        var button1 = false
        var button2 = false
        
        if controller.viceType.lightPenNeedsButtonOnTouch {
            button1 = position != nil
            if controller.deviceConfig.numberOfButtons >= 1 {
                button2 = rawButton1
            }
        }
        else {
            if controller.deviceConfig.numberOfButtons >= 1 {
                button1 = rawButton1
            }
            if controller.deviceConfig.numberOfButtons >= 2 {
                button2 = rawButton2
            }
        }
        
        vice?.lightPen(moved: position, size: size, button1: button1, button2: button2, isKoalaPad: controller.viceType == .koalaPad)
    }
    
    public func inputDevice(_ device: InputDevice, paddleMoved position: Double) {
        guard let port = port(for: device) else { return }
        
        let value = Int32(255 * position)
        
        if port.subPort == 1 {
            vice?.mouse(setX: value)
        }
        else {
            vice?.mouse(setY: value)
        }
    }
    
    public func inputDevice(_ device: InputDevice, paddleChangedButton isPressed: Bool) {
        guard let port = port(for: device) else { return }

        let index = port.subPort == 1 ? 1 : 2
        if isPressed {
            vice?.mouse(pressed: index)
        }
        else {
            vice?.mouse(release: index)
        }
    }
    
    public func inputDeviceDidDisconnect(_ inputDevice: InputDevice) {
        inputMapping.remove(device: inputDevice)
    }
    
    private func port(for device: InputDevice) -> InputPort? {
        guard let pairing = inputMapping[device] else { return nil }
        pairing.lastActivity = Date()
        return pairing.port
    }
}
