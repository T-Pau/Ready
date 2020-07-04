/*
 MachineSpecification.swift -- Hardware Configuration
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

public struct MachineSpecification {
    public var layers: [MachineConfigOld]
    
    public init(layers: [MachineConfigOld]) {
        self.layers = layers
        self.layers.append(MachineConfigOld.defaultConfig)
    }
    
    public func save() throws {
        try layers[0].save()
    }
    
    public mutating func append(layer: MachineConfigOld) {
        layers.insert(layer, at: 0)
    }
    
    public func appending(layer: MachineConfigOld) -> MachineSpecification {
        var newSecification = self
        newSecification.append(layer: layer)
        return newSecification
    }
    
    public func topLayerIdentifier(for key: MachineConfigOld.Key) -> String {
        return layers[0].string(for: key) ?? "default"
    }
    
    public func identifier(for key: MachineConfigOld.Key) -> String {
        return string(for: key) ?? "none"
    }
    
    func value(for key: MachineConfigOld.Key, skipFirstLayer: Bool = false) -> MachineConfigOld.Value? {
        let relevantLayers = skipFirstLayer ? layers.dropFirst() : ArraySlice(layers)
        
        for layer in relevantLayers {
            if let value = layer.value(for: key) {
                return value
            }
        }
        
        return nil
    }
    
    func string(for key: MachineConfigOld.Key, skipFirstLayer: Bool = false) -> String? {
        if let value = value(for: key, skipFirstLayer: skipFirstLayer) {
            switch value {
            case .string(let string):
                return string
            default:
                break
            }
        }
        return nil
    }
    
    func integer(for key: MachineConfigOld.Key, skipFirstLayer: Bool = false) -> Int? {
        if let value = value(for: key, skipFirstLayer: skipFirstLayer) {
            switch value {
            case .integer(let integer):
                return integer
            default:
                break
            }
        }
        return nil
    }
    
    public mutating func set(string: String?, for key: MachineConfigOld.Key) {
        layers[0].set(string: string, for: key)
    }
    
    public func isDefault(key: MachineConfigOld.Key) -> Bool {
        return !layers[0].hasValue(for: key)
    }
    
    public func differences(to specification: MachineSpecification) -> Set<MachineConfigOld.Key> {
        return layers[0].differences(to: specification.layers[0])
    }
}

extension MachineSpecification {
    public var borderMode: MachineConfigOld.BorderMode {
        get {
            guard let value = string(for: .borderMode) else { return .auto }
            return MachineConfigOld.BorderMode(rawValue: value) ?? .auto
        }
        set {
            set(string: newValue.rawValue, for: .borderMode)
        }
    }
    
    public var singularAdapterMode: MachineConfigOld.SingularAdapterMode {
        get {
            guard let value = string(for: .singularAdapterMode) else { return .cga }
            return MachineConfigOld.SingularAdapterMode(rawValue: value) ?? .cga
        }
        set {
            set(string: newValue.rawValue, for: .singularAdapterMode)
        }

    }
    
    public func cartridges(for machine: Machine?) -> [Cartridge] {
        var cartridges = [Cartridge](repeating: OtherCartridge.none, count: 3)
        
        let identifiers = MachineConfigOld.cartridgeKeys.map({ identifier(for: $0) })
        let specifiedCartridges = identifiers.map({ OtherCartridge.cartridge(identifier: $0) })
        var machineCartridges = [Cartridge]()

        if let machine = machine {
            if let cartridge = machine.cartridgeImage {
                machineCartridges.append(cartridge)
            }
            if let ramExpansionUnit = machine.ramExpansionUnit {
                machineCartridges.append(ramExpansionUnit)
            }
            if !machine.ideDiskImages.isEmpty {
                machineCartridges.append(Ide64Cartridge.standard)
            }
        }
        
        if let port = computer.port(for: .expansionPort) {
            machineCartridges = machineCartridges.filter({ ($0 as! MachinePart).isCompatible(with: port) })
        }
        
        var isMainSlotFull = false

        if identifiers[0] == "auto" {
            if specifiedCartridges[1] != nil || specifiedCartridges[2] != nil || machineCartridges.count > 1 {
                cartridges[0] = OtherCartridge.expander
            }
            else if !machineCartridges.isEmpty {
                cartridges[0] = machineCartridges[0]
                machineCartridges.remove(at: 0)
            }
        }
        else {
            cartridges[0] = specifiedCartridges[0] ?? OtherCartridge.none
        }
        isMainSlotFull = cartridges[0].cartridgeType == .main

        if cartridges[0].numberOfSlots > 0 {
            for index in (1 ... cartridges[0].numberOfSlots) {
                if isMainSlotFull {
                    machineCartridges.removeAll(where: { $0.cartridgeType == .main })
                }
                if identifiers[index] == "auto" && !machineCartridges.isEmpty {
                    cartridges[index] = machineCartridges[0]
                    machineCartridges.remove(at: 0)
                }
                else {
                    cartridges[index] = specifiedCartridges[index] ?? OtherCartridge.none
                }
                isMainSlotFull = isMainSlotFull || cartridges[index].cartridgeType == .main
            }
        }
        
        return cartridges
    }
    
    public var cassetteDrive: CasstteDrive {
        guard computer.has(port: .cassetteDrive) else { return CasstteDrive.none }
        return CasstteDrive.drive(identifier: identifier(for: .cassetteDrive)) ?? CasstteDrive.none
    }
    
    public var computer: Computer {
        return Computer.computer(identifier: identifier(for: .computer)) ?? Computer.c64Pal
    }
    
    public var controllers: [Controller] {
        var controllers = [Controller]()
        
        if computer.has(port: .controlPort1) {
            controllers.append(controller1)
        }
        if computer.has(port: .controlPort2) {
            controllers.append(controller2)
        }

        return controllers
    }
    
    public var controller1: Controller {
        return Controller.controller(identifier: identifier(for: .controlPort1)) ?? Controller.none
    }
    
    public var controller2: Controller {
        guard computer.has(port: .controlPort2) else { return Controller.none }
        return Controller.controller(identifier: identifier(for: .controlPort2)) ?? Controller.none
    }

    public var diskDrives: [DiskDrive] {
        var keys = MachineConfigOld.diskDriveKeys
        var drives = [DiskDrive]()
        if let drive8 = computer.drive8 {
            drives.append(drive8)
            keys.removeFirst()
        }
        for key in keys {
            if computer.has(port: key) {
                drives.append(DiskDrive.drive(identifier: identifier(for: key)) ?? DiskDrive.none)
            }
            else {
                drives.append(DiskDrive.none)
            }
        }
        return drives
    }
    
    public var userPortController1: Controller {
        if userPortModule.getJoystickPorts(for: self) >= 1 {
            return Controller.controller(identifier: identifier(for: .userPortJoystick1)) ?? Controller.none
        }
        else {
            return Controller.none
        }
    }

    public var userPortController2: Controller {
        if userPortModule.getJoystickPorts(for: self) >= 2 {
            return Controller.controller(identifier: identifier(for: .userPortJoystick2)) ?? Controller.none
        }
        else {
            return Controller.none
        }
    }

    public var userPortControllers: [Controller] {
        return [ userPortController1, userPortController2 ]
    }
    
    public var userPortModule: UserPortModule {
        guard computer.has(port: .userPort) else { return UserPortModule.none }
        return UserPortModule.module(identifier: identifier(for: .userPort)) ?? UserPortModule.none
    }
    
    public func automount(images: [DiskImage]) -> ([DiskDrive], [DiskImage?]) {
        var drives = diskDrives
        var mountedImages = [DiskImage?](repeating: nil, count: drives.count)

        for image in images {
            var mountable = false
            for index in drives.indices {
                if drives[index].supports(image: image) {
                    if mountedImages[index] == nil {
                        mountedImages[index] = image
                    }
                    mountable = true
                    break
                }
            }
            
            if !mountable {
                guard let newDrive = DiskDrive.getDriveSupporting(image: image) else { continue }
                for index in drives.indices {
                    if computer.has(port: MachineConfigOld.diskDriveKeys[index]) && string(for: MachineConfigOld.diskDriveKeys[index]) == "auto" {
                        if !drives[index].hasStatus {
                            drives[index] = newDrive
                            mountedImages[index] = image
                            break
                        }
                        if let mountedImage = mountedImages[index], newDrive.supports(image: mountedImage) {
                            drives[index] = newDrive
                            break
                        }
                    }
                }
            }
        }

        return (drives, mountedImages)
    }
}

extension MachineSpecification {
    private func autoPart(for key: MachineConfigOld.Key, machine: Machine?) -> MachinePart {
        if let machine = machine {
            var part: MachinePart = DummyMachinePart.none
            var annotateName = true

            switch key {
            case .cassetteDrive:
                part = machine.cassetteDrive
                annotateName = false

            case .expansionPort, .expansionPort1, .expansionPort2:
                annotateName = false
                guard let index = key.expansionPortIndex else { break }
                let specification = self.appending(layer: [ key: .string("auto") ])
                let cartridges = specification.cartridges(for: machine)
                
                if index < cartridges.count {
                    part = cartridges[index] as! MachinePart
                }
                
            case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11:
                guard let driveNumber = key.driveNumber else { break }
                let index = driveNumber - 8
                let specification = self.appending(layer: [ key: .string("auto") ])
                let (drives, _) = specification.automount(images: machine.diskImages)
                
                if index < drives.count {
                    part = drives[index]
                }

            default:
                return DummyMachinePart.none
            }
            
            return DummyMachinePart(identifier: "auto", fullName: "Automatic", annotateName: annotateName, basePart: part)
        }
        else {
            var iconName: String
            switch key {
            case .cassetteDrive:
                iconName = "Automatic Cassette Drive"
                
            case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11:
                iconName = "Automatic Disk Drive II"
                
            case .expansionPort:
                if let port = computer.port(for: .expansionPort), port.connectorTypes.contains(.vic20ExpansionPort) {
                    iconName = "Automatic VIC-20 Cartridge"
                }
                else {
                    iconName = "Automatic Cartridge"
                }
                
            default:
                return DummyMachinePart.none
            }
            
            return DummyMachinePart(identifier: "auto", name: "Automatic", iconName: iconName)
        }
    }
    
    public func part(for key: MachineConfigOld.Key, value: String?, machine: Machine?) -> MachinePart {
        guard let value = value else { return DummyMachinePart.none }
        if value == "none" {
            return DummyMachinePart.none
        }
        
        if value == "auto" {
            return autoPart(for: key, machine: machine)
        }
        
        switch key {
        case .computer:
            return Computer.computer(identifier: value) ?? DummyMachinePart.none
            
        case .controlPort1, .controlPort2, .userPortJoystick1, .userPortJoystick2:
            return Controller.controller(identifier: value) ?? Controller.none
            
        case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11:
            return DiskDrive.drive(identifier: value) ?? DiskDrive.none
            
        case .cassetteDrive:
            return CasstteDrive.drive(identifier: value) ?? CasstteDrive.none
            
        case .expansionPort, .expansionPort1, .expansionPort2:
            // TODO: CartridgeImage
            return OtherCartridge.cartridge(identifier: value) as? MachinePart ?? OtherCartridge.none
            
        case .userPort:
            return UserPortModule.module(identifier: value) ?? UserPortModule.none
            
        case .borderMode, .singularAdapterMode:
            return DummyMachinePart.none
        case .screen:
            return DummyMachinePart.none // TODO
        }
    }

    public func part(for key: MachineConfigOld.Key, machine: Machine?) -> MachinePart {
        return part(for: key, value: string(for: key), machine: machine)
    }
    
    public func partList(for key: MachineConfigOld.Key, machine: Machine?) -> MachinePartList {
        var partList: MachinePartList
        
        switch key {
        case .cassetteDrive:
            partList = CasstteDrive.drives
            
        case .computer:
            partList = Computer.computers
            
        case .controlPort1:
            partList = Controller.controllers
            
        case .controlPort2:
            partList = Controller.port2Controllers
            
        case .diskDrive8, .diskDrive9, .diskDrive10, .diskDrive11:
            partList = DiskDrive.drives

        case .expansionPort:
            // TODO: CartridgeImage, REUImage
            partList = OtherCartridge.cartridges

        case .expansionPort1, .expansionPort2:
            // TODO: CartridgeImage, REUImage
            partList = OtherCartridge.cartridges.filter({ ($0 as! Cartridge).cartridgeType != .expander })

        case .userPort:
            partList = UserPortModule.modules
            
        case .userPortJoystick1, .userPortJoystick2:
            partList = Controller.userPortControllers
            
        default:
            return MachinePartList(sections: [])
        }
        
        if let port = computer.port(for: key) {
            partList = partList.filter({ $0.isCompatible(with: port )})
            if partList.sections[0].title != nil {
                partList.sections.insert(MachinePartSection(title: nil, parts: []), at: 0)
            }
        }
        
        let defaultValue = string(for: key, skipFirstLayer: true)
        let defaultPart = DummyMachinePart(identifier: "default", fullName: "Default", annotateName: false, basePart: part(for: key, value: defaultValue, machine: machine))
        partList.insert(defaultPart, at: IndexPath(row: 0, section: 0))
        
        if key.supportsAuto {
            partList.insert(autoPart(for: key, machine: machine), at: IndexPath(row: 1, section: 0))
        }
        
        return partList
    }
    
}
