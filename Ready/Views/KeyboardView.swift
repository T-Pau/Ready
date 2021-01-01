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
    var keyboardImage: UIImage?
    var pressedImage: UIImage?
    var toggleKeys = Set<Key>()
    var toggleImages = [Key: UIImage]()
    var keyRegions = [Key: Keyboard.Region]()

    var delegate: KeyboardViewDelegate?

    var keyboard: Keyboard? {
        didSet {
            keyboardImage = nil
            pressedImage = nil
            toggleKeys.removeAll()
            keyRegions.removeAll()
            toggleImages.removeAll()
            
            if let keyboard = keyboard {
                keyboardImage = UIImage(named: keyboard.imageName)
                toggleKeys = keyboard.toggleKeys
                
                if keyboard.hasPressedImage {
                    keyRegions = keyboard.getKeyRegions()
                    pressedImage = UIImage(named: keyboard.imageName + " Pressed")
                }
                else {
                    for (key, imageName) in keyboard.toggleImages {
                        toggleImages[key] = UIImage(named: imageName)
                    }
                }
            }
            if let constraint = aspectConstraint {
                removeConstraint(constraint)
                aspectConstraint = nil
            }
            if let image = keyboardImage, image.size.height > 0 {
                let constraint = NSLayoutConstraint(item: self, attribute: .width, relatedBy: .equal, toItem: self, attribute: .height, multiplier: image.size.width / image.size.height, constant: 0)
                addConstraint(constraint)
                aspectConstraint = constraint
            }
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }
    
    private var aspectConstraint: NSLayoutConstraint?
    
    private var keyTouches = [UITouch : Key]()
    var pressedKeys = Set<Key>()
    
    override func draw(_ rect: CGRect) {
        //super.draw(rect)
        
        if let image = keyboardImage, let context = UIGraphicsGetCurrentContext() {
            let scale = min(bounds.width / image.size.width, bounds.height / image.size.height)
            let size = CGSize(width: image.size.width * scale, height: image.size.height * scale)
            let rect = CGRect(origin: CGPoint(x: bounds.origin.x + (bounds.width - size.width) / 2, y: bounds.origin.y + (bounds.height - size.height) / 2), size: size)
            
            image.draw(in: rect)
            
            for key in pressedKeys {
                if let toggleImage = toggleImages[key] {
                    toggleImage.draw(in: rect)
                }
            }
            
            if let pressedImage = pressedImage {
                var needsDraw = false
                for key in pressedKeys {
                    if let region = keyRegions[key] {
                        switch region {
                        case .rect(let rect):
                            let scaledRect = CGRect(x: bounds.origin.x + rect.minX * scale, y: bounds.origin.y + rect.minY * scale, width: rect.width * scale, height: rect.height * scale)
                            context.addRect(scaledRect)
                            needsDraw = true
                            
                        default:
                            // TODO
                            break
                        }
                    }
                }

                if needsDraw {
                    context.saveGState()
                    context.clip()
                    pressedImage.draw(in: rect)
                    //context.setFillColor(CGColor(red: 1, green: 0, blue: 0, alpha: 0.5))
                    //context.fill(rect)
                    context.restoreGState()
                }
            }
        }
    }

    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        var unhandledTouches = Set<UITouch>()
        
        for touch in touches {
            var location = touch.location(in: self)
            location.x *= (keyboardImage?.size.width ?? 0) / frame.width
            location.y *= (keyboardImage?.size.height ?? 0) / frame.height
            
            if let key = keyboard?.hit(location) {
                keyTouches[touch] = key
                if toggleKeys.contains(key) {
                    if pressedKeys.contains(key) {
                        pressedKeys.remove(key)
                        delegate?.released(key: key)
                    }
                    else {
                        pressedKeys.insert(key)
                        delegate?.pressed(key: key)
                    }
                }
                else {
                    pressedKeys.insert(key)
                    delegate?.pressed(key: key)
                }
                setNeedsDisplay()
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
                // TODO: keep keys pressed for toggle keys when havePressed
                keyTouches.removeValue(forKey: touch)
                if !toggleKeys.contains(key) {
                    pressedKeys.remove(key)
                    delegate?.released(key: key)
                }
                setNeedsDisplay()
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
}
