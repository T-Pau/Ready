/*
 ShareableImageView.swift -- Image View with Drag/Drop Support
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

public protocol ShareableImageViewDelegate {
    func shareableImageViewChanged(view: ShareableImageView)
}

public class ShareableImageView: UIImageView, UIDropInteractionDelegate, UIDragInteractionDelegate {
    public var delegate: ShareableImageViewDelegate?
    @IBInspectable public var editable = true
        
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        
//        let recognizer = UILongPressGestureRecognizer(target: self, action: #selector(longPress(_:)))
//        addGestureRecognizer(recognizer)
    
        isUserInteractionEnabled = true
        addInteraction(UIDropInteraction(delegate: self))
        addInteraction(UIDragInteraction(delegate: self))
    }
    
    @IBAction func longPress(_ sender: AnyObject) {
        if let recognizer = sender as? UILongPressGestureRecognizer {
            switch recognizer.state {
            case .began:
                becomeFirstResponder()
                let menuController = UIMenuController.shared
                menuController.showMenu(from: self, rect: bounds)
                
            case .cancelled:
                UIMenuController.shared.hideMenu(from: self)
                
            default:
                break
            }
        }
    }

    /*
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        // Drawing code
    }
    */
    
    override public var canBecomeFirstResponder: Bool {
        return true
    }
    
    override public func canPaste(_ itemProviders: [NSItemProvider]) -> Bool {
        for provider in itemProviders {
            if provider.canLoadObject(ofClass: UIImage.self) {
                return true
            }
        }
        return false
    }
    
#if false
    override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
        if action == #selector(copy(_:)) {
            return image != nil
        }
        // TODO: share?
#if false
        if action == #selector(paste(_:)) {
            return true
        }
#endif
        return false
    }
#endif
    
    override public func copy(_ sender: Any?) {
        print("TODO: copy image")
    }
    
    override public func paste(itemProviders: [NSItemProvider]) {
        for provider in itemProviders {
            if provider.canLoadObject(ofClass: UIImage.self) {
                provider.loadObject(ofClass: UIImage.self) { [weak self] object, _ in
                    guard let image = object as? UIImage else { return }
                    DispatchQueue.main.async {
                        guard let self = self else { return }

                        self.image = image
                        self.delegate?.shareableImageViewChanged(view: self)
                    }
                }
                break
            }
        }
    }
    
    public func dragInteraction(_ interaction: UIDragInteraction, itemsForBeginning session: UIDragSession) -> [UIDragItem] {
        return getDragItems()
    }
    
    public func dragInteraction(_ interaction: UIDragInteraction, itemsForAddingTo session: UIDragSession, withTouchAt point: CGPoint) -> [UIDragItem] {
        return getDragItems()
    }
    
    public func dropInteraction(_ interaction: UIDropInteraction, canHandle session: UIDropSession) -> Bool {
        return editable && session.canLoadObjects(ofClass: UIImage.self)
    }
    
    public func dropInteraction(_ interaction: UIDropInteraction, sessionDidUpdate session: UIDropSession) -> UIDropProposal {
        return UIDropProposal(operation: .copy)
    }
    
    public func dropInteraction(_ interaction: UIDropInteraction, performDrop session: UIDropSession) {
        guard editable else { return }
        session.loadObjects(ofClass: UIImage.self) { [weak self] objects in
            guard let images = objects as? [UIImage] else { return }
            guard !images.isEmpty else { return }
            guard let self = self else { return }
            
            self.image = images[0]
            self.delegate?.shareableImageViewChanged(view: self)
        }
    }
    
    private func getDragItems() -> [UIDragItem] {
        guard let image = image else { return [] }
        
        let provider = NSItemProvider(object: image)
        let item = UIDragItem(itemProvider: provider)
        item.localObject = image
        
        return [item]
    }
}
