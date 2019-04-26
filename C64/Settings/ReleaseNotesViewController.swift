/*
 ReleaseNotesViewController.swift
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

class ReleaseNotesViewController: UIViewController {
    struct Release {
        var name: String
        var year: Int
        var month: Int
        var day: Int
        var changes: [String]
    }
    
    var releases = [
        Release(name: "Release 1.1 (Build 4)", year: 2019, month: 4, day: 26, changes: [
            "Add hardware keyboard support for Control and Commodore (via CapsLock, can be disabled in Settings)."
        ]),
        Release(name: "Release 1.1 (Build 3)", year: 2019, month: 4, day: 23, changes: [
            "Improve thumbstick handling for MfI controllers."
        ]),
        Release(name: "Release 1.1 (Build 2)", year: 2019, month: 4, day: 22, changes: [
            "Add Commodore 64 variants with VIC-20 Style Keyboard.",
            "Add Competition Pro variants and QuickShot IX."
        ]),
        Release(name: "Release 1.1", year: 2019, month: 4, day: 21, changes: [
            "First public release."
        ])
    ]
    
    @IBOutlet weak var textView: UITextView!
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewDidAppear(animated)

        let headingBoldAttributes = [NSAttributedString.Key.font: UIFont.systemFont(ofSize: 17, weight: .bold)]
        let headingAttributes = [NSAttributedString.Key.font: UIFont.systemFont(ofSize: 17)]
        let bodyAttributes = [NSAttributedString.Key.font: UIFont.systemFont(ofSize: 15)]
        
        let text = NSMutableAttributedString()
        
        for release in releases {
            text.append(NSAttributedString(string: "\(release.name) ", attributes: headingBoldAttributes))
            text.append(NSAttributedString(string: String(format: "(%04d-%02d-%02d)\n", release.year, release.month, release.day), attributes: headingAttributes))
            for change in release.changes {
                text.append(NSAttributedString(string: "\n    – \(change)\n", attributes: bodyAttributes))
            }
            text.append(NSAttributedString(string: "\n\n", attributes: bodyAttributes))
        }
        
        textView.attributedText = text
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}
