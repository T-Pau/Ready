/*
 SingularAdapterStatusView.swift -- Singular 4-Player Adapter Status
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
import C64UIComponents
import Emulator

@IBDesignable class SingularAdapterStatusView: StatusView {
    public var mode = MachineConfigOld.SingularAdapterMode.cga {
        didSet {
            let onIndex: Int?
            switch mode {
            case .cga:
                onIndex = 0
            case .hs:
                onIndex = 1
            case .bba:
                onIndex = 2
            case .off:
                onIndex = nil
            }
            
            for index in (0 ..< ledViews.count) {
                ledViews[index].intensity = index == onIndex ? 1 : 0
            }
        }
    }
    
    public var radiusFraction = CGFloat(0.5)
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setupViews()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupViews()
    }
    
    private var ledViews = [
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0)),
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0)),
        LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    ]
    
    private func setupViews() {
        backgroundColor = UIColor(named: "Singular Adapter Case")
        stackView.spacing = 12
        
        ledViews[0].setColor(name: "Singular Adapter CGA")
        ledViews[1].setColor(name: "Singular Adapter H&S")
        ledViews[2].setColor(name: "Singular Adapter BBA")
        
        var constraints = [NSLayoutConstraint]()
        for ledView in ledViews {
            ledView.isRound = true
            ledView.radiusFraction = 1
            ledView.translatesAutoresizingMaskIntoConstraints = false
            constraints.append(NSLayoutConstraint(item: ledView, attribute: .width, relatedBy: .equal, toItem: ledView, attribute: .height, multiplier: radiusFraction, constant: 1))
            stackView.addArrangedSubview(ledView)
        }
        
        let contentHuggingConstraint = NSLayoutConstraint(item: self, attribute: .width, relatedBy: .lessThanOrEqual, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: 8)
        contentHuggingConstraint.priority = .defaultLow
        constraints.append(contentHuggingConstraint)
        
        NSLayoutConstraint.activate(constraints)
    }
}
