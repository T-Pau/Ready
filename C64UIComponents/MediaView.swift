/*
 MediaView.swift -- Media Item
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

public protocol MediaItem {
    var displayTitle: String? { get }
    var displaySubtitle: String? { get }
    var subtitleIsPETASCII: Bool { get }
    var displayIcon: UIImage? { get }
    
    var typeIdentifier: String? { get }
    var url: URL? { get }
}

public protocol MediaViewDropDelegate: class {
    func mediaView(_ sender: MediaView, dropped item: UIDragItem, of type: String)
}

@IBDesignable public class MediaView: UIView {
    private var iconView: UIImageView?
    private var labelView: UIStackView?
    private var titleLabel = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    private var subtitleLabel = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    
    @IBInspectable public var icon: UIImage? { didSet { updateIcon() } }
    @IBInspectable public var title: String? { didSet { updateTitle() } }
    @IBInspectable public var subtitle: String? { didSet { updateSubtitle() } }
    @IBInspectable public var subtitleIsPETASCII: Bool = false { didSet { updateSubtitleFont() } }
    @IBInspectable public var enableDrag: Bool = false { didSet { updateDrag() } }
    @IBInspectable public var showIcon: Bool = true {
        didSet {
            if showIcon {
                iconView?.isHidden = false
            }
            else {
                iconView?.isHidden = true
            }
        }
    }

    public var textAlignment: NSTextAlignment {
        get {
            return titleLabel.textAlignment
        }
        set {
            titleLabel.textAlignment = newValue
            subtitleLabel.textAlignment = newValue
        }
    }
    
    public var acceptableDropTypes = Set<String>() { didSet { updateDrop() } }
    public weak var dropDelegate: MediaViewDropDelegate?
    
    public var item: MediaItem? {
        didSet {
            icon = item?.displayIcon
            title = item?.displayTitle
            subtitle = item?.displaySubtitle
            subtitleIsPETASCII = item?.subtitleIsPETASCII ?? false
        }
    }

    override init(frame: CGRect) {
        super.init(frame: frame)
        setupView()
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        updateDrag()
        updateDrop()
        setupView()
    }
    
    override public static var requiresConstraintBasedLayout: Bool { return true }
    
    private func setupView() {
        let iconView = UIImageView(image: nil)
        self.iconView = iconView
        iconView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(iconView)


        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.font = UIFont.systemFont(ofSize: 17)

        subtitleLabel.translatesAutoresizingMaskIntoConstraints = false

        let labelView = UIStackView(arrangedSubviews: [titleLabel, subtitleLabel])
        self.labelView = labelView
        labelView.translatesAutoresizingMaskIntoConstraints = false
        labelView.spacing = linesep
        labelView.axis = .vertical
        addSubview(labelView)
        
        updateIcon()
        updateSubtitle()
        updateSubtitleFont()
        updateTitle()
    }
    
    let separation = CGFloat(8)
    let linesep = CGFloat(5)
    let iconSize = CGSize(width: 40, height: 39)
    
    override public func updateConstraints() {
        let minimizeHeightConstraing = NSLayoutConstraint(item: self, attribute: .height, relatedBy: .equal, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: 0)
        minimizeHeightConstraing.priority = .defaultLow

        NSLayoutConstraint.activate([
            minimizeHeightConstraing,
            
            NSLayoutConstraint(item: iconView!, attribute: .leading, relatedBy: .equal, toItem: self, attribute: .leading, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: iconView!, attribute: .centerY, relatedBy: .equal, toItem: self, attribute: .centerY, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: iconView!, attribute: .top, relatedBy: .greaterThanOrEqual, toItem: self, attribute: .top, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: iconView!, attribute: .height, relatedBy: .equal, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: iconSize.height),
            NSLayoutConstraint(item: iconView!, attribute: .width, relatedBy: .equal, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: iconSize.width),
            
            NSLayoutConstraint(item: labelView!, attribute: .leading, relatedBy: .equal, toItem: iconView!, attribute: .trailing, multiplier: 1, constant: separation),
            NSLayoutConstraint(item: labelView!, attribute: .trailing, relatedBy: .equal, toItem: self, attribute: .trailing, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: labelView!, attribute: .top, relatedBy: .greaterThanOrEqual, toItem: self, attribute: .top, multiplier: 1, constant: 0),
            NSLayoutConstraint(item: labelView!, attribute: .centerY, relatedBy: .equal, toItem: self, attribute: .centerY, multiplier: 1, constant: 0),
        ])

        super.updateConstraints()
    }

    private func updateIcon() {
        iconView?.image = icon
    }
    
    private func updateSubtitle() {
        if subtitle == nil {
            subtitleLabel.isHidden = true
        }
        else {
            subtitleLabel.text = subtitle
            subtitleLabel.isHidden = false
        }
        //labelView?.layoutIfNeeded()
    }
    
    private func updateSubtitleFont() {
        if (subtitleIsPETASCII) {
            subtitleLabel.font = UIFont(name: "CBM", size: 12)
        }
        else {
            subtitleLabel.font = UIFont.systemFont(ofSize: 11)
        }

    }
    
    private func updateTitle() {
        if let title = title {
            titleLabel.text = title
            if #available(iOS 13.0, *) {
                titleLabel.textColor = UIColor.label
            } else {
                titleLabel.textColor = UIColor.darkText
            }
        }
        else {
            titleLabel.text = "None"
            titleLabel.textColor = UIColor.gray
        }
    }
    
    private var dragInteraction: UIDragInteraction?
    private var dropInteraction: UIDropInteraction?
    
    private func updateDrag() {
        if enableDrag {
            if dragInteraction == nil {
                let interaction = UIDragInteraction(delegate: self)
                addInteraction(interaction)
                dragInteraction = interaction
            }
        }
        else {
            if let interaction = dragInteraction {
                removeInteraction(interaction)
                dragInteraction = nil
            }
        }
    }
    
    private func updateDrop() {
        if acceptableDropTypes.isEmpty {
            if let interaction = dropInteraction {
                removeInteraction(interaction)
                dropInteraction = nil
            }
            else {
                let interaction = UIDropInteraction(delegate: self)
                addInteraction(interaction)
                dropInteraction = interaction
            }
        }
    }
}

extension MediaView: UIDragInteractionDelegate {
    public func dragInteraction(_ interaction: UIDragInteraction, itemsForBeginning session: UIDragSession) -> [UIDragItem] {
        guard enableDrag else { return [] }
        guard let item = item else { return [] }
        guard let typeIdentifier = item.typeIdentifier else { return [] }
        guard let url = item.url else { return [] }
 
        let itemProvider = NSItemProvider()
        
        itemProvider.registerFileRepresentation(forTypeIdentifier: typeIdentifier, fileOptions: [], visibility: .all){ completionHandler in
            
            completionHandler(url, true, nil)
            return nil
        }
        itemProvider.suggestedName = url.lastPathComponent
        
        return [ UIDragItem(itemProvider: itemProvider) ]
    }
}

extension MediaView: UIDropInteractionDelegate {
    public func dropInteraction(_ interaction: UIDropInteraction, canHandle session: UIDropSession) -> Bool {
        guard dropDelegate != nil && !acceptableDropTypes.isEmpty else { return false }
        return session.hasItemsConforming(toTypeIdentifiers: [String](acceptableDropTypes))
    }
    
    public func dropInteraction(_ interaction: UIDropInteraction, sessionDidUpdate session: UIDropSession) -> UIDropProposal {
        return UIDropProposal(operation: .copy)
    }
    
    public func dropInteraction(_ interaction: UIDropInteraction, performDrop session: UIDropSession) {
        for item in session.items {
            for typeIdentifier in item.itemProvider.registeredTypeIdentifiers {
                if acceptableDropTypes.contains(typeIdentifier) {
                    dropDelegate?.mediaView(self, dropped: item, of: typeIdentifier)
                    return
                }
            }
        }
    }
}
