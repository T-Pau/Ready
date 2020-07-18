/*
 UserPortModule.swift -- User Port Module Harware Part
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

import UIKit

extension MachineConfigOld.SingularAdapterMode {
    public var viceJoystickType: UserPortModule.ViceJoystickType? {
        switch self {
        case .cga:
            return .cga
        case .hs:
            return .hit
        case .bba:
            return .kingsoft
        case .off:
            return nil
        }
    }
}

public struct UserPortModule: MachinePart {
    public enum ViceJoystickType: Int32 {
        case cga = 0
        case pet = 1
        case hummer = 2
        case oem = 3
        case hit = 4
        case kingsoft = 5
        case starbyte = 6
        
        var joystickPorts: Int {
            switch self {
            case .cga, .hit, .kingsoft, .pet, .starbyte:
                return 2
                
            case .hummer, .oem:
                return 1
            }
        }
    }
    
    public enum ModuleType: Equatable {
        case none
        case protovisionAdapter
        case singularAdapter
    }
    
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var priority: Int

    public var connector: ConnectorType { return .c64UserPort }
    
    public var moduleType: ModuleType
    
    public func getViceJoystickType(for specification: MachineSpecification) -> ViceJoystickType? {
        switch moduleType {
        case .protovisionAdapter:
            return .cga
        case .singularAdapter:
            return specification.singularAdapterMode.viceJoystickType
        default:
            return nil
        }
    }
    
    public func getJoystickPorts(for specification: MachineSpecification) -> Int {
        return getViceJoystickType(for: specification)?.joystickPorts ?? 0
    }
    
    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String?, priority: Int = MachinePartNormalPriority, moduleType: ModuleType = .none) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        self.priority = priority
        self.moduleType = moduleType
    }
    
    public static let none = UserPortModule(identifier: "none", name: "None", iconName: nil)
    
    public static let modules = MachinePartList(sections: [
        MachinePartSection(title: nil, parts: [
            none
        ]),
        
        MachinePartSection(title: "Joystick Adapters", parts: [
            UserPortModule(identifier: "Singular 4 Player Adapter",
                           name: "Singular",
                           fullName: "Singular Crew 4 Player Adapter",
                           iconName: "Singular Crew 4 Player Adapter",
                           moduleType: .singularAdapter),
            UserPortModule(identifier: "Protovision 4 Player Adapter",
                           name: "Protovision",
                           fullName: "Protovision 4 Player Adapter",
                           iconName: "Protovision 4 Player Adapter",
                           moduleType: .protovisionAdapter),
        ])
    ])
    
    static private var byIdentifier = [String: UserPortModule]()
    
    public static func module(identifier: String) -> UserPortModule? {
        if byIdentifier.isEmpty {
            for module in modules.parts {
                byIdentifier[module.identifier] = module as? UserPortModule
            }
        }
        
        return byIdentifier[identifier]
    }

}
