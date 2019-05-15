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
        Release(name: "Release 1.2 (Build 11)", year: 2019, month: 5, day: 15, changes: [
            "Support for multiple cartridges."
        ]),
        Release(name: "Release 1.2 (Build 10)", year: 2019, month: 5, day: 14, changes: [
            "Implement ShiftLock in virtual keyboard (C=-Lock for Japanese C64). No visual indication of pressed key yet.",
            "Fix Open in from Safari.",
            "Always display two drive heads for double-sided drives.",
            "Don't duplicate disks when starting emulation in Tools tab."
        ]),
        Release(name: "Release 1.2 (Build 9)", year: 2019, month: 5, day: 11, changes: [
            "Fix port naming and numbering for paddles.",
            "Add two button Amiga mouse."
        ]),
        Release(name: "Release 1.2 (Build 8)", year: 2019, month: 5, day: 10, changes: [
            "Use absolute positioning for KoalaPad.",
            "Fix Neos and Atari ST mice."
        ]),
        Release(name: "Release 1.2 (Build 7)", year: 2019, month: 5, day: 10, changes: [
            "Add Singular Crew 4-Player Adapter.",
            "Allow deleting categories."
        ]),
        Release(name: "Release 1.2 (Build 6)", year: 2019, month: 5, day: 8, changes: [
            "Fix IDE disk support, add CD images.",
            "Fix sensibility of Elite Multifunction 2002 paddles."
        ]),
        Release(name: "Release 1.2 (Build 5)", year: 2019, month: 5, day: 7, changes: [
            "Add IDE64 Cartridge and Compact Flash and Hard Disk support.",
        ]),
        Release(name: "Release 1.2 (Build 4)", year: 2019, month: 5, day: 6, changes: [
            "Add Elite Multi-Function 2002, in Joystick and Paddle mode.",
            "Light pen and light guns don't work in Control Port 2."
        ]),
        Release(name: "Release 1.2 (Build 3)", year: 2019, month: 5, day: 5, changes: [
            "Add support for joysticks with up to three fire buttons.",
            "Add Cheetah Annihilator."
        ]),
        Release(name: "Release 1.2 (Build 2)", year: 2019, month: 5, day: 4, changes: [
            "Add paddles.",
            "Group hardware parts into subcategories."
        ]),
        Release(name: "Release 1.2", year: 2019, month: 5, day: 3, changes: [
            "Add hardware keyboard support for Control and Commodore (via CapsLock, can be disabled in Settings).",
            "Improve thumbstick handling for MfI controllers.",
            "Add RAM Expansion Units.",
            "Add C64 variants Silver, VIC-20 Style Keyboard, Drean, Educator 64.",
            "Add Light Pen, Light Gun, KoalaPad, Nihon Neos Mouse.",
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
        textView.scrollRangeToVisible(NSRange(location: 0, length: 1))
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}
