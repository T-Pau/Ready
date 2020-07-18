/*
 KeyboardView.swift -- C64 Keyboard
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
import Emulator

protocol KeyboardViewDelegate {
    func pressed(key: Key)
    func released(key: Key)
}

class KeyboardView: UIView {
    var keyboardImageView = UIImageView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    var toggleViews = [Key: UIImageView]()

    var delegate: KeyboardViewDelegate?
    
    var keyboard: Keyboard? {
        didSet {
            for view in toggleViews.values {
                view.removeFromSuperview()
            }
            toggleViews.removeAll()
            if let keyboard = keyboard {
                for (key, imageName) in keyboard.toggleKeys {
                    let view = UIImageView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
                    view.image = UIImage(named: imageName)
                    view.isHidden = true
                    setup(view: view)
                    toggleViews[key] = view
                }
                keyboardImageView.image = UIImage(named: keyboard.imageName)
            }
            else {
                keyboardImageView.image = nil
            }
            if let constraint = aspectConstraint {
                removeConstraint(constraint)
            }
            if let image = keyboardImageView.image, image.size.height > 0 {
                let constraint = NSLayoutConstraint(item: self, attribute: .width, relatedBy: .equal, toItem: self, attribute: .height, multiplier: image.size.width / image.size.height, constant: 0)
                addConstraint(constraint)
                aspectConstraint = constraint
            }
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    
    private func setup() {
        setup(view: keyboardImageView)
    }
    
    private func setup(view: UIView) {
        addSubview(view)
        view.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            NSLayoutConstraint(item: self, attribute: .left, relatedBy: .equal, toItem: view, attribute: .left, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .right, relatedBy: .equal, toItem: view, attribute: .right, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .top, relatedBy: .equal, toItem: view, attribute: .top, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: self, attribute: .bottom, relatedBy: .equal, toItem: view, attribute: .bottom, multiplier: 1, constant: 0),
            ])
    }
    
    private var aspectConstraint: NSLayoutConstraint?
    
    private var keyTouches = [UITouch : Key]()

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        var unhandledTouches = Set<UITouch>()
        
        for touch in touches {
            var location = touch.location(in: self)
            location.x *= (keyboardImageView.image?.size.width ?? 0) / frame.width
            location.y *= (keyboardImageView.image?.size.height ?? 0) / frame.height
            
            if let key = keyboard?.hit(location) {
                keyTouches[touch] = key
                if let view = toggleViews[key] {
                    if view.isHidden {
                        view.isHidden = false
                        delegate?.pressed(key: key)
                    }
                    else {
                        view.isHidden = true
                        delegate?.released(key: key)
                    }
                }
                else {
                    delegate?.pressed(key: key)
                }
            }
            else {
                unhandledTouches.insert(touch)
            }
        }
        
        if !unhandledTouches.isEmpty {
            super.touchesBegan(unhandledTouches, with: event)
        }
    }
    
    private func touchesReleased(_ touches: Set<UITouch>) -> Set<UITouch> {
        var unhandledTouches = Set<UITouch>()

        for touch in touches {
            if let key = keyTouches[touch] {
                keyTouches.removeValue(forKey: touch)
                if toggleViews[key] == nil {
                    delegate?.released(key: key)
                }
            }
            else {
                unhandledTouches.insert(touch)
            }
        }
        
        return unhandledTouches
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        let unhandledTouches = touchesReleased(touches)
        
        if !unhandledTouches.isEmpty {
            super.touchesEnded(unhandledTouches, with: event)
        }
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        let unhandledTouches = touchesReleased(touches)
        
        if !unhandledTouches.isEmpty {
            super.touchesCancelled(unhandledTouches, with: event)
        }
    }
    
    override func touchesEstimatedPropertiesUpdated(_ touches: Set<UITouch>) {
        var unhandledTouches = Set<UITouch>()

        for touch in touches {
            if let _ = keyTouches[touch] {
            }
            else {
                unhandledTouches.insert(touch)
            }
        }

        if !unhandledTouches.isEmpty {
            super.touchesEstimatedPropertiesUpdated(unhandledTouches)
        }
    }
    
    @IBAction func tapped(_ sender: Any) {
        print("keyboard tapped")
    }
}
