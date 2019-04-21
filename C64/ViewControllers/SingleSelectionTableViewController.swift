/*
 SingleSelectionTableViewController.swift -- Table to select one of an arary of items
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
//
//  SingleSelectionTableViewController.swift
//  C64
//
//  Created by Dieter Baron on 2019/01/07.
//  Copyright © 2019 Spiderlab. All rights reserved.
//

import UIKit

class SingleSelectionTableViewController: UITableViewController {
    // MARK: - Configuration
    
    @IBInspectable var reuseIdentifier = "Cell"
    @IBInspectable var dismissOnSelection = false
    
    @objc var selected: ((Any) -> ())?

    // MARK: - Data
    
    @objc var items =  [Any]()
    var selectedRow: Int?

#if false
    override func viewDidLoad() {
        // TODO: get font/size from cell, row height from table view
        let attributes = [NSAttributedString.Key.font: UIFont.systemFont(ofSize: 17.0)]
        var width = CGFloat(0.0)
        for gameViewItem in items {
            let str = "\(gameViewItem)"
            width = max((str as NSString).size(withAttributes: attributes).width, width)
        }
        preferredContentSize = CGSize(width: width + 40.0, height: min(44 * CGFloat(items.count), 600))
    }
#else
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tableView.addObserver(self, forKeyPath: "contentSize", options: .new, context: nil)
    }
    
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        tableView.removeObserver(self, forKeyPath: "contentSize")
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey : Any]?, context: UnsafeMutableRawPointer?) {
        if keyPath == "contentSize" {
            preferredContentSize = CGSize(width: tableView.contentSize.width, height: min(tableView.contentSize.height, 600))
        }
    }
#endif
    
    // MARK: - Table view data source

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return items.count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: reuseIdentifier, for: indexPath)

        if indexPath.row == selectedRow {
            cell.accessoryType = .checkmark
        }
        cell.textLabel?.text = "\(items[indexPath.row])"
        return cell
    }
    
    // MARK: - Table view selection
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        selected?(items[indexPath.row])
        if dismissOnSelection {
            self.dismiss(animated: true)
        }
        else {
            var indexPaths = [IndexPath]()
            if let oldRow = selectedRow {
                indexPaths.append(IndexPath(row: oldRow, section: 0))
            }
            selectedRow = indexPath.row
            indexPaths.append(IndexPath(row: indexPath.row, section: 0))
            tableView.reloadRows(at: indexPaths, with: .automatic)
        }
    }
}
