/*
 MachinePartView.sift
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
