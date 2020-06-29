/*
 GridView.swift -- arrange subviews in fixed grid
 Copyright (C) 2020 Dieter Baron
 
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
