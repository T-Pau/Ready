/*
 DriveStatusView.swift -- Disk Drive Status
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

@IBDesignable public class DriveStatusView: StatusView {
    public var unitLabel = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    public var trackView = DriveTrackView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    public var ledViews = [LedView]()

    @IBInspectable public var numberOfLeds: Int = 2
    @IBInspectable public var unit: Int = 8 {
        didSet {
            unitLabel.text = "\(unit)"
        }
    }
    
    override public var textColor: UIColor? {
        didSet {
            unitLabel.textColor = textColor
            trackView.color = textColor ?? UIColor.black
        }
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setupViews()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupViews()
    }
    
    private func setupViews() {
        stackView.spacing = 8
        
        unitLabel.text = "\(unit)"
        unitLabel.translatesAutoresizingMaskIntoConstraints = false
        unitLabel.font = UIFont.systemFont(ofSize: 17)
        stackView.addArrangedSubview(unitLabel)
        
        trackView.translatesAutoresizingMaskIntoConstraints = false
        stackView.addArrangedSubview(trackView)
        
        var constraints = [NSLayoutConstraint]()
        for _ in (0 ..< numberOfLeds) {
            let ledView = LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
            ledView.radiusFraction = 0.6
            ledView.translatesAutoresizingMaskIntoConstraints = false
            constraints.append(NSLayoutConstraint(item: ledView, attribute: .width, relatedBy: .equal, toItem: ledView, attribute: .height, multiplier: 1, constant: 1))
            stackView.addArrangedSubview(ledView)
            ledViews.append(ledView)
        }
        NSLayoutConstraint.activate(constraints)
    }
}
