/*
 GridView.swift -- arrange subviews in fixed grid
 Copyright (C) 2020 Dieter Baron
 
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

@IBDesignable class GridView: UIView {
    private struct SubviewInfo {
        var view: UIView
        var position: CGRect
    }
    @IBInspectable var margin: CGFloat = 8 { didSet { setNeedsLayout() } }
    @IBInspectable var horizontalGap: CGFloat = 8 { didSet { setNeedsLayout() } }
    @IBInspectable var verticalGap: CGFloat = 8 { didSet { setNeedsLayout() } }
    @IBInspectable var cellWidth: CGFloat = 72 { didSet { setNeedsLayout() } }
    @IBInspectable var cellHeight: CGFloat = 72 { didSet { setNeedsLayout() } }

    private var subviewInfos = [SubviewInfo]()
    private var maxColumn = 0
    private var maxRow = 0
    
    func add(subview: UIView, at position: CGRect) {
        subviewInfos.append(SubviewInfo(view: subview, position: position))
        addSubview(subview)
    }
    
    func remove(subview: UIView) {
        subviewInfos.removeAll(where: { $0.view == subview })
        subview.removeFromSuperview()
    }
    
    private func updateSize() {
        maxColumn = 0
        maxRow = 0
        
        for info in subviewInfos {
            maxColumn = max(maxColumn, Int(info.position.maxX))
            maxRow = max(maxRow, Int(info.position.maxY))
        }
    }
    
    override var intrinsicContentSize: CGSize {
        return CGSize(width: width(of: maxColumn) + 2 * margin, height: height(of: maxRow) + 2 * margin)
    }

    internal override func layoutSubviews() {
        for info in subviewInfos {
            let x = Int(info.position.origin.x)
            let y = Int(info.position.origin.y)
            let w = Int(info.position.size.width)
            let h = Int(info.position.size.height)
            info.view.frame = CGRect(x: leftEdge(of: x), y: topEdge(of: y), width: width(of: w), height: height(of: h))
        }
    }
    
    func leftEdge(of column: Int) -> CGFloat {
        return margin + (cellWidth + horizontalGap) * CGFloat(column)
    }
    
    func width(of columns: Int) -> CGFloat {
        return cellWidth * CGFloat(columns) + horizontalGap * CGFloat(columns - 1)
    }
    
    func topEdge(of row: Int) -> CGFloat {
        return margin + (cellHeight + verticalGap) * CGFloat(row)
    }
    
    func height(of rows: Int) -> CGFloat {
        return cellHeight * CGFloat(rows) + verticalGap * CGFloat(rows - 1)
    }
}
