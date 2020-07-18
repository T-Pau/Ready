/*
 PlaceholderButton.swift -- Button with Placeholder Text
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

@IBDesignable public class PlaceholderButton: UIButton {
    @IBInspectable public var title: String? { didSet { update() } }
    @IBInspectable public var placeholder: String? { didSet { update() } }
    @IBInspectable public var placeholderColor: UIColor? { didSet { update() } }
    @IBInspectable public var placeholderFont: UIFont? { didSet { update() } }
    @IBInspectable public var suffix: String? { didSet { update() } }
    
    @objc public var titleColor: UIColor? { didSet { update() } }
    public var titleFont: UIFont? { didSet { update() } }
    
    override public init(frame: CGRect) {
        super.init(frame: frame)
    }

    public required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        
        titleColor = titleLabel?.textColor
        titleFont = titleLabel?.font
        
        if placeholderColor == nil {
            placeholderColor = titleColor
        }
        if placeholderFont == nil {
            placeholderFont = titleFont
        }
}
    
    private func update() {
        let text = "\(title ?? placeholder ?? "")\(suffix ?? "")"
        setTitle(text == "" ? nil : text, for: [])
        setTitleColor(title != nil ? titleColor : placeholderColor, for: [])
        titleLabel?.font = (title != nil ? titleFont : placeholderFont)
    }
}
