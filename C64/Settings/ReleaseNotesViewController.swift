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
        Release(name: "Release 1.3 (Build 11)", year: 2019, month: 12, day: 5, changes: [
            "Add Sony DualShock 4 icon.",
            "More Dark Mode fixes."
        ]),
        Release(name: "Release 1.3 (Build 10)", year: 2019, month: 12, day: 4, changes: [
            "More Dark Mode fixes."
        ]),
        Release(name: "Release 1.3 (Build 9)", year: 2019, month: 12, day: 3, changes: [
            "Mostly fix Dark Mode."
        ]),
        Release(name: "Release 1.3 (Build 8)", year: 2019, month: 9, day: 28, changes: [
            "Update IDE64 BIOS to 0.90 patch 53 (2019-08-19).",
            "Fix version number in About screen."
        ]),
        Release(name: "Release 1.3 (Build 7)", year: 2019, month: 8, day: 10, changes: [
            "Save IDE64 configuration between emulations."
        ]),
        Release(name: "Release 1.3 (Build 6)", year: 2019, month: 8, day: 9, changes: [
            "Fix IDE64 on subsequent emulator launches."
        ]),
        Release(name: "Release 1.3 (Build 5)", year: 2019, month: 7, day: 4, changes: [
            "Map more buttons from MiF controllers.",
            "Fix directory and sub-partition in directory listing."
        ]),
        Release(name: "Release 1.3 (Build 4)", year: 2019, month: 5, day: 28, changes: [
            "Add CMD Smart Mouse and Smart Track."
        ]),
        Release(name: "Release 1.3 (Build 3)", year: 2019, month: 5, day: 24, changes: [
            "Update IDE64 BIOS.",
            "Add Atari Track-Ball CX22."
        ]),
        Release(name: "Release 1.3 (Build 2)", year: 2019, month: 5, day: 22, changes: [
            "Make Start / Tools buttons more prominent."
        ]),
        Release(name: "Release 1.3", year: 2019, month: 5, day: 22, changes: [
            "Change function key keyboard shortcuts to Command-1 through Command-8.",
            "Implement ShiftLock in virtual keyboard (C=-Lock for Japanese C64).",
            "Support multiple cartridges (via Mini X-Pander).",
            "Add IDE64 Cartridge.",
            "Add Singular Crew 4-Player Adapter.",
            "Add paddles.",
            "Add Cheetah Annihilator (two button joystick for C64 Game System).",
            "Add two button Amiga mouse.",
            "Fix Neos and Atari ST mice.",
            "Use absolute positioning for KoalaPad.",
            "Group hardware parts into subcategories.",
            "Display GEOS disks with lowercase character set.",
            "Allow deleting categories.",
            "Fix p00 file handling."
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
        
        if #available(iOS 13.0, *) {
            text.addAttribute(.foregroundColor, value: UIColor.label, range: NSRange(location: 0, length: text.length))
        }
        textView.attributedText = text
        textView.scrollRangeToVisible(NSRange(location: 0, length: 1))
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}
