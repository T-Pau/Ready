/*
 ReleaseNotesViewController.swift
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

class ReleaseNotesViewController: UIViewController {
    struct Release {
        var name: String
        var year: Int
        var month: Int
        var day: Int
        var changes: [String]
    }
    
    var releases = [
        Release(name: "Release 1.6 build 9", year: 2020, month: 8, day: 28, changes: [
            "Update x16-emulator to r38."
        ]),
        Release(name: "Release 1.6 build 8", year: 2020, month: 8, day: 27, changes: [
            "Update Vice to 3.4.",
            "Add VIC-20 32K RAM expansion."
        ]),
        Release(name: "Release 1.6 build 7", year: 2020, month: 7, day: 25, changes: [
            "Add option to disable accellerated tape loading on ZX Spectrum.",
            "Add directory as SD Card for Commander X16."
        ]),
        Release(name: "Release 1.6 build 6", year: 2020, month: 7, day: 22, changes: [
            "Allow choosing which screen to show (automatic not implemented yet).",
            "Add C128 PAL/NTSC with US keyboard.",
            "Implement 40/08 Display key."
        ]),
        Release(name: "Release 1.6 build 5", year: 2020, month: 7, day: 21, changes: [
            "Preliminary Commodore 128 support."
        ]),
        Release(name: "Release 1.6 build 4", year: 2020, month: 7, day: 15, changes: [
            "Preliminary Commander X16 support. (No peripherals supported yet.)"
        ]),
        Release(name: "Release 1.6 build 3", year: 2020, month: 7, day: 11, changes: [
            "Preliminary Atari 800XL support. (No peripherals supported yet. It starts in diagnostic mode, press reset to exit to BASIC.)"
        ]),
        Release(name: "Release 1.6", year: 2020, month: 7, day: 8, changes: [
            "Rename to Ready.",
            "Add Commodore 16 and Plus/4 emulation.",
            "Add ZX Spectrum emulation. Supported models are 16k, 48k, Specturm+, 128k, +2.",
            "Add new app icons, one for each supported computer."
        ]),
        Release(name: "Release 1.5", year: 2020, month: 6, day: 28, changes: [
            "Add Commodore VIC-20 emulation.",
            "Use new keyboard API to support long key presses."
        ]),
        Release(name: "Release 1.4", year: 2020, month: 4, day: 25, changes: [
            "Fix compatibility issues with Dark Mode.",
            "Fix bug that prevented adding games to the library on fresh installs.",
            "Fixes and BIOS update for IDE64.",
            "Add Commodore 1571",
            "Add CMD Smart Mouse and Smart Track.",
            "Add Atari Track-Ball CX22.",
            "Add Sony DualShock 4 icon.",
            "Map more buttons from MiF controllers.",
            "Fix directory and sub-partition in directory listing.",
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
