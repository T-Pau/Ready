/*
 TapeStatusView -- Tape Drive Status
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

public class TapeStatusView: StatusView {
    public var ledView: LedView?
    public var controlView: UIImageView?
    public var counterView: TapeCounterView?
    public var wheelView: UIImageView?
    
    @IBInspectable public var isMotorOn: Bool = false {
        didSet {
            guard  showCounter == false && isMotorOn != oldValue else { return }
            
            if isMotorOn {
                wheelView?.startAnimating()
            }
            else {
                wheelView?.stopAnimating()
                wheelView?.image = wheelView?.animationImages?.isEmpty ?? true ? nil : wheelView?.animationImages?[0]
            }
        }
    }
    
    @IBInspectable public var showCounter: Bool = true {
        didSet {
            guard showCounter != oldValue else { return }
            counterView?.isHidden = !showCounter
            wheelView?.isHidden = showCounter
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
        isOpaque = false
        stackView.spacing = 8
        
        var constraints = [NSLayoutConstraint]()
        
        let paddingView = UIView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
        paddingView.translatesAutoresizingMaskIntoConstraints = false
        stackView.addArrangedSubview(paddingView)
        
        let ledView = LedView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
        self.ledView = ledView
        ledView.translatesAutoresizingMaskIntoConstraints = false
        constraints.append(NSLayoutConstraint(item: ledView, attribute: .width, relatedBy: .equal, toItem: ledView, attribute: .height, multiplier: 0.5, constant: 0))
        //constraints.append(NSLayoutConstraint(item: ledView, attribute: .height, relatedBy: .equal, toItem: self, attribute: .height, multiplier: 0.5, constant: 0))
        stackView.addArrangedSubview(ledView)
        
        let controlView = UIImageView(image: nil)
        self.controlView = controlView
        controlView.translatesAutoresizingMaskIntoConstraints = false
        constraints.append(NSLayoutConstraint(item: controlView, attribute: .width, relatedBy: .equal, toItem: controlView, attribute: .height, multiplier: 1, constant: 0))
        //constraints.append(NSLayoutConstraint(item: ledView, attribute: .height, relatedBy: .equal, toItem: self, attribute: .height, multiplier: 1, constant: 0))
        stackView.addArrangedSubview(controlView)

        let wheelView = UIImageView(image: nil)
        self.wheelView = wheelView
        wheelView.translatesAutoresizingMaskIntoConstraints = false
        wheelView.animationImages = (1...12).reversed().compactMap({ UIImage(named: "Tape Wheel \($0)" ) })
        wheelView.animationDuration = 0.33
        wheelView.image = wheelView.animationImages?.isEmpty ?? true ? nil : wheelView.animationImages?[0]
        if showCounter {
            wheelView.isHidden = true
        }
        stackView.addArrangedSubview(wheelView)
        
        let counterView = TapeCounterView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
        self.counterView = counterView
        counterView.translatesAutoresizingMaskIntoConstraints = false
        counterView.digits = 3
        counterView.font = UIFont(name: "HelveticaNeue-Medium", size: 17) ?? UIFont.systemFont(ofSize: 17)
        counterView.textColor = UIColor.white
        counterView.counterColor = UIColor.black
        counterView.lineSpacing = 0.75
        counterView.charSpacing = 1.5
        if !showCounter {
            counterView.isHidden = true
        }
        stackView.addArrangedSubview(counterView)
        
        let contentHuggingConstraint = NSLayoutConstraint(item: self, attribute: .width, relatedBy: .lessThanOrEqual, toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: 8)
        contentHuggingConstraint.priority = .defaultLow
        
        constraints.append(contentHuggingConstraint)
        NSLayoutConstraint.activate(constraints)
    }
}
