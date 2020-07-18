/*
 SingleSelectionTableViewController.swift -- Table to select one of an arary of items
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
