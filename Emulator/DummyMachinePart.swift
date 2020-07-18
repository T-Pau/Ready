/*
 DummyMachinePart.swift -- Machine Part without Real Counterpart
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

public struct DummyMachinePart: MachinePart {
    public var identifier: String
    public var name: String
    public var fullName: String
    public var variantName: String?
    public var icon: UIImage?
    public var smallIcon: UIImage?
    public var priority: Int { return MachinePartLowPriority }
    public var connector: ConnectorType

    public init(identifier: String, name: String, fullName: String? = nil, variantName: String? = nil, iconName: String? = nil, connector: ConnectorType = .none) {
        self.identifier = identifier
        self.name = name
        self.fullName = fullName ?? name
        self.variantName = variantName
        if let iconName = iconName {
            self.icon = UIImage(named: iconName)
        }
        else {
            self.icon = nil
        }
        self.smallIcon = icon
        self.connector = connector
    }
    
    public init(identifier: String, fullName: String, annotateName: Bool, basePart: MachinePart) {
        self.identifier = identifier
        name = basePart.name
        if annotateName {
            name = "\(name) (Automatic)"
        }
        self.fullName = fullName
        icon = basePart.icon
        smallIcon = basePart.smallIcon
        var description = basePart.fullName
        if let variantName = basePart.variantName {
            description += " (\(variantName))"
        }
        variantName = description
        connector = basePart.connector
    }
        
    public static let none = DummyMachinePart(identifier: "none", name: "None")
}
