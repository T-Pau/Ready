/*
 MachinePartView.sift
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

import UIKit

@IBDesignable public class MachinePartView: UIControl {
    @IBInspectable public var image: UIImage? {
        get {
            return imageView.image
        }
        set {
            imageView.image = newValue
        }
    }
    @IBInspectable public var label: String? {
        get {
            return labelView.text
        }
        set {
            labelView.text = newValue
        }
    }
    @IBInspectable public var sublabel: String? {
        get {
            return sublabelView.text
        }
        set {
            sublabelView.text = newValue
        }
    }
    
    @IBInspectable public var boldLabel: Bool = false {
        didSet {
            labelView.font = UIFont.systemFont(ofSize: 14, weight: boldLabel ? .semibold : .regular)
        }
    }
    @IBInspectable public var cornerRadius: CGFloat = 8
    @IBInspectable public var borderThickness: CGFloat = 1
    @IBInspectable public var borderColor: UIColor = UIColor.black
    @IBInspectable public var insideColor: UIColor = UIColor.white
    @IBInspectable public var margin: CGFloat = 8
    @IBInspectable public var lowerMargin: CGFloat = 7
    
    private var imageView = UIImageView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    private var labelView = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
    private var sublabelView = UILabel(frame: CGRect(x: 0, y: 0, width: 0, height: 0))

    public required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
        setup()
    }
    
    public override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    private func setup() {
        isOpaque = false
        isUserInteractionEnabled = true
        imageView.translatesAutoresizingMaskIntoConstraints = false
        imageView.contentMode = .center
        labelView.translatesAutoresizingMaskIntoConstraints = false
        labelView.textAlignment = .center
        labelView.adjustsFontSizeToFitWidth = true
        labelView.minimumScaleFactor = 0.71
        sublabelView.translatesAutoresizingMaskIntoConstraints = false
        sublabelView.textAlignment = .center
        
        labelView.font = UIFont.systemFont(ofSize: 14)
        sublabelView.font = UIFont.systemFont(ofSize: 12)

        addSubview(imageView)
        addSubview(labelView)
        addSubview(sublabelView)

        let views = [
            "image": imageView,
            "label": labelView,
            "sublabel": sublabelView
        ]
        let metrics = [
            "margin": margin + borderThickness,
            "bottomMargin": lowerMargin + borderThickness,
            "separation": lowerMargin,
            "labelSeparation": lowerMargin / 2
        ]
        
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "V:|-margin-[image]-separation-[label]-labelSeparation-[sublabel]-bottomMargin-|", options: [], metrics: metrics, views: views))
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "H:|-margin-[image(>=78)]-margin-|", options: [.alignAllCenterX], metrics: metrics, views: views))
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "H:|-margin-[label]-margin-|", options: [.alignAllCenterX], metrics: metrics, views: views))
        NSLayoutConstraint.activate(NSLayoutConstraint.constraints(withVisualFormat: "H:|-margin-[sublabel]-margin-|", options: [.alignAllCenterX], metrics: metrics, views: views))

        setNeedsUpdateConstraints()
    }
    
    public override func draw(_ rect: CGRect) {
        //print("\(sublabel ?? ""): \(imageView.bounds)")
        if let context = UIGraphicsGetCurrentContext() {
            let roundedRect = UIBezierPath.init(roundedRect: bounds.insetBy(dx: borderThickness / 2, dy: borderThickness / 2), cornerRadius: cornerRadius)
            context.setStrokeColor(borderColor.cgColor)
            context.setFillColor(insideColor.cgColor)
            context.setLineWidth(borderThickness)
            roundedRect.fill()
            roundedRect.stroke()
        }
    }
}
