/*
 EmulatorViewController.swift
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
import AVFoundation
import GameController
import MobileCoreServices

import C64UIComponents
import Emulator
import Vice_C64
import ViceVIC20

class EmulatorViewController: FullScreenViewController, KeyboardViewDelegate, SeguePreparer, UIGestureRecognizerDelegate {
    enum SegueType: String {
        case borderMode
        case diskDrive8Control
        case diskDrive9Control
        case diskDrive10Control
        case diskDrive11Control
        case inputMapping
        case machineParts
        case singularAdapterMode
        
        var unit: Int? {
            switch self {
            case .diskDrive8Control:
                return 8
            case .diskDrive9Control:
                return 9
            case .diskDrive10Control:
                return 10
            case .diskDrive11Control:
                return 11
            default:
                return nil
            }
        }
    }
    
    @IBOutlet weak var stackView: UIStackView!
    @IBOutlet weak var imageView: UIImageView!
    @IBOutlet weak var keyboardView: KeyboardView!
    @IBOutlet weak var keyboardButton: UIBarButtonItem!
    @IBOutlet weak var freezeButton: UIBarButtonItem!
    @IBOutlet weak var tapeStatus: TapeStatusView!
    @IBOutlet var drive8Status: DriveStatusView!
    @IBOutlet var drive9Status: DriveStatusView!
    @IBOutlet var drive10Status: DriveStatusView!
    @IBOutlet var drive11Status: DriveStatusView!
    @IBOutlet var singularStatus: SingularAdapterStatusView!
    @IBOutlet weak var statusBarView: UIView!
    @IBOutlet weak var safeInsetView: UIView!
    @IBOutlet weak var screenshotFlashView: UIView!
    @IBOutlet weak var safeInsetHeight: NSLayoutConstraint!
    @IBOutlet weak var mouseJoystickButton: UIBarButtonItem!
    @IBOutlet weak var controllerView: VirtualControlsView!
    @IBOutlet weak var saveTopConstraint: NSLayoutConstraint!
    @IBOutlet weak var fullscreenTopConstraint: NSLayoutConstraint!
    @IBOutlet weak var controllerViewTopConstraint: NSLayoutConstraint!
    
    private var driveStatus = [DriveStatusView]()
    
    var gameViewItem: GameViewItem?
    var toolsMode = false
    var selectedMedia: MediaItem?
    var emulator: Emulator?
    var running = false
    var machine = Machine()
    
    private var controllers = [GCController : MfiInputDevice]()
    
    private var keyPressTranslator: KeyPressTranslator?
        
    override func viewSafeAreaInsetsDidChange() {
        safeInsetHeight.constant = view.safeAreaInsets.bottom
    }
    
    override func viewDidLoad() {
        driveStatus = [ drive8Status, drive9Status, drive10Status, drive11Status ]
        controllerView.topOffset = controllerViewTopConstraint.constant
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        navigationItem.title = gameViewItem?.title
        
        keyboardView.delegate = self

        guard !running else { return }
        
        if let gameViewItem = gameViewItem {
            machine = gameViewItem.machine
            machine.directoryURL = gameViewItem.directoryURL
            
            if let mediaItem = selectedMedia {
                switch mediaItem.mediaType {
                case .cartridge:
                    machine.cartridgeImage = mediaItem as? CartridgeImage
                case .disk:
                    if let index = machine.diskImages.firstIndex(where: { $0.url == mediaItem.url }) {
                        machine.diskImages.remove(at: index)
                    }
                    machine.diskImages.insert(mediaItem as! DiskImage, at: 0)

                case .ideDisk:
                    if let index = machine.ideDiskImages.firstIndex(where: { $0.url == mediaItem.url }) {
                        machine.ideDiskImages.remove(at: index)
                    }
                    machine.ideDiskImages.insert(mediaItem as! IdeDiskImage, at: 0)

                case .programFile:
                    machine.programFile = mediaItem as? ProgramFile
                case .ramExpansionUnit:
                    machine.ramExpansionUnit = mediaItem as? RamExpansionUnit
                case .tape:
                    if let index = machine.tapeImages.firstIndex(where: { $0.url == mediaItem.url }) {
                        machine.tapeImages.remove(at: index)
                    }
                    machine.tapeImages.insert(mediaItem as! TapeImage, at: 0)
                }
            }
        }
        else {
            machine = Machine()
        }
        
        if toolsMode {
            machine.cartridgeImage = Tools.standard.selectedCartridge
            machine.autostart = false
        }
        
        switch machine.specification.computer.viceMachineModel.viceMachine {
        case .c64:
            emulator = Vice_C64.Vice()
        case .vic:
            emulator = ViceVIC20.Vice()
        }
        machine.vice = emulator
        
        _keyboardCommands.removeAll()
        preapareKeyPressTranslator()
        
        let computer = machine.specification.computer
        statusBarView.backgroundColor = computer.caseColor
        keyboardView.backgroundColor = computer.caseColor
        keyboardView.keyboard = computer.keyboard
        safeInsetView.backgroundColor = computer.caseColor
        
        if view.frame.width > view.frame.height || computer.keyboard == nil {
            keyboardView.isHidden = true
        }

        if gameViewItem?.type != .tools && toolsMode {
            machine.diskImages.append(contentsOf: Tools.standard.disks)
        }
        
        machine.mountDisks()
        
        for index in (0 ..< driveStatus.count) {
            if machine.diskDrives.count > index && machine.diskDrives[index].hasStatus {
                driveStatus[index].configureFrom(drive: machine.diskDrives[index])
                driveStatus[index].isHidden = false
            }
            else {
                driveStatus[index].isHidden = true
            }
        }
        
        if machine.userPortModule?.moduleType == .singularAdapter {
            singularStatus.isHidden = false
            singularStatus.mode = machine.specification.singularAdapterMode
        }
        else {
            singularStatus.isHidden = true
        }
        
        if let tapeFile = (machine.tapeImages.isEmpty ? nil : machine.tapeImages[0])?.url, machine.cassetteDrive.hasStatus {
            tapeStatus.ledView?.isRound = true
            tapeStatus.ledView?.darkColor = UIColor(named: "LED 1530 Off")!
            tapeStatus.ledView?.lightColor = UIColor(named: "LED 1530 On")!
            tapeStatus.controlView?.image = UIImage(named: "Tape Stop")
            tapeStatus.showCounter = tapeFile.pathExtension.lowercased() != "t64"
            tapeStatus.backgroundColor = machine.cassetteDrive.caseColor
        }
        else {
            tapeStatus.isHidden = true
        }
    
        if !haveStatusBar {
            statusBarView.isHidden = true
            if keyboardView.isHidden {
                safeInsetView.isHidden = true
            }
        }
        
        let biosURL = Defaults.biosURL

        if !toolsMode && machine.tapeImages.isEmpty && Defaults.standard.enableJiffyDOS {
            if let romC64 = Defaults.standard.biosJiffyDosC64 {
                if machine.specification.computer.viceMachineModel.viceMachine == .c64 {
                    machine.resources[.KernalName] = .String(biosURL.appendingPathComponent(romC64).path)
                }
                if let rom1541 = Defaults.standard.biosJiffyDos1541 {
                    machine.resources[.DosName1541] = .String(biosURL.appendingPathComponent(rom1541).path)
                }
                if let rom1541II = Defaults.standard.biosJiffyDos1541II {
                    machine.resources[.DosName1541II] = .String(biosURL.appendingPathComponent(rom1541II).path)
                }
                if let rom1581 = Defaults.standard.biosJiffyDos1581 {
                    machine.resources[.DosName1581] = .String(biosURL.appendingPathComponent(rom1581).path)
                }
            }
        }
        
        if let rom2000 = Defaults.standard.biosFD2000 {
            machine.resources[.DosName2000] = .String(biosURL.appendingPathComponent(rom2000).path)
        }
        if let rom4000 = Defaults.standard.biosFD4000 {
            machine.resources[.DosName4000] = .String(biosURL.appendingPathComponent(rom4000).path)
        }
        
        machine.resources[.DriveSoundEmulation] = .Bool(Defaults.standard.emulateDriveSounds)

        emulator?.machine = machine
        emulator?.imageView = imageView
        emulator?.delegate = self
       
        if Defaults.standard.videoFilter == "None" {
            imageView.layer.magnificationFilter = .nearest
        }
    
        if machine.cartridgeImage?.hasFreeze ?? false != true {
            navigationItem.rightBarButtonItems?.removeAll(where: { $0 == freezeButton })
        }
        if computer.keyboard == nil {
            navigationItem.rightBarButtonItems?.removeAll(where: { $0 == keyboardButton })
        }
        
        if let game = gameViewItem as? Game {
            game.lastOpened = Date()
        }
        
        do {
            try (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext.save()
        }
        catch {
            
        }
        
        NotificationCenter.default.addObserver(self, selector: #selector(controllerWasConnected(_:)), name: .GCControllerDidConnect, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(controllerWasDisconnected(_:)), name: .GCControllerDidDisconnect, object: nil)

        machine.connect(inputDevice: TouchInputDevice(view: controllerView))
        
        for controller in GCController.controllers() {
            connect(controller: controller)
        }
        
        running = true
        emulator?.start()
    }
    
    override func viewDidDisappear(_ animated: Bool) {
        emulator?.quit()
        running = false
    }
    
    var _keyboardCommands = [UIKeyCommand]()
    
    override var keyCommands: [UIKeyCommand]? {
        if (_keyboardCommands.count == 0) {
            _keyboardCommands = [
                UIKeyCommand(title: "Reset", action: #selector(resetMachine(_:)), input: "r", modifierFlags: .command, discoverabilityTitle: "Reset")
            ]
            
            if machine.cartridgeImage?.hasFreeze ?? false {
                _keyboardCommands.append(UIKeyCommand(title: "Freeze", action: #selector(freezeMachine(_:)), input: "z", modifierFlags: .command, discoverabilityTitle: "Freeze"))
            }
        }
        return _keyboardCommands
    }
        

    @IBAction func toggleKeyboard(_ sender: Any) {
        UIView.animate(withDuration: 0.3) {
            self.keyboardView.isHidden = !self.keyboardView.isHidden
            if self.keyboardView.isHidden {
                if self.haveStatusBar && self.autoHideStatusBar && self.statusBarIsIdle {
                    self.statusBarView.isHidden = true
                }
                if !self.haveStatusBar || self.statusBarView.isHidden {
                    self.safeInsetView.isHidden = true
                }
            }
            else {
                self.safeInsetView.isHidden = false
            }
            self.stackView.layoutIfNeeded()
        }
    }
    
    @IBAction func takeScreenshot(_ sender: Any) {
        guard let gameViewItem = gameViewItem else { return }
        guard let ciImage = imageView.image?.ciImage else { return }
        let tempURL = gameViewItem.directoryURL.appendingPathComponent(UUID().uuidString + ".png")
        let context = CIContext(options: nil)
        guard let cgImage = context.createCGImage(ciImage, from: ciImage.extent) else { return }
        guard let destination = CGImageDestinationCreateWithURL(tempURL as CFURL, kUTTypePNG, 1, nil) else { return }
        CGImageDestinationAddImage(destination, cgImage, nil)
        CGImageDestinationFinalize(destination)
        gameViewItem.addScreenshot(url: tempURL)
        UIView.animate(withDuration: 0.1, animations: {
            self.screenshotFlashView.alpha = 1.0
        }, completion: { finished in
            if finished {
                UIView.animate(withDuration: 0.3) {
                    self.screenshotFlashView.alpha = 0.0
                }
            }
        })
    }

    @IBAction func freezeMachine(_ sender: Any) {
        emulator?.freeze()
    }

    @IBAction func resetMachine(_ sender: Any) {
        emulator?.reset()
    }
        
    // MARK: - Navigation

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }
        
        switch segueType {
        case .borderMode:
            guard let destination = segue.destination as? SingleSelectionTableViewController else { return }
            // TODO: add row to revert to default
            destination.items = MachineConfig.BorderMode.allCases
            destination.dismissOnSelection = true
            destination.selectedRow = MachineConfig.BorderMode.allCases.firstIndex(where: { $0 == machine.specification.borderMode })
            destination.selected = { value in
                guard let mode = value as? MachineConfig.BorderMode else { return }
                self.machine.change(borderMode: mode)
            }

        case .diskDrive8Control, .diskDrive9Control, .diskDrive10Control, .diskDrive11Control:
            guard let destination = segue.destination as? SelectDiskViewController, let emulator = emulator,
                let unit = segueType.unit else { return }

            let drive = emulator.machine.diskDrives[unit - 8]
            destination.drive = drive
            destination.unit = unit
            destination.diskImages = emulator.machine.diskImages.filter({ drive.supports(image: $0) })
            destination.currentDiskImage = drive.image
            //destination.status = String(cString: drive_get_status(Int32(unit)))

            destination.changeCallback = { diskImage in
                self.driveStatus[unit - 8].configureFrom(image: diskImage)
                self.emulator?.attach(drive: unit, image: diskImage)
            }
            
        case .inputMapping:
            guard let navigationController = segue.destination as? UINavigationController,
                let destination = navigationController.topViewController as? InputMappingTableViewController else { return }
            destination.inputMapping = machine.inputMapping

        case .machineParts:
            guard let navigationController = segue.destination as? UINavigationController, let destination = navigationController.topViewController as? MachinePartsViewController else { return }
            destination.machineSpecification = machine.specification
            destination.machine = machine
            destination.isMachineRunning = true
            destination.changeHandler = { machineSpecification in
                self.machine.update(specification: machineSpecification)
            }
            
        case .singularAdapterMode:
            guard let destination = segue.destination as? SingularAdapterStatusViewController else { return }
            destination.selectedValue = machine.specification.singularAdapterMode
            destination.singularAdapter = machine.userPortModule
            destination.changeCallback = { mode in
                var newSpecification = self.machine.specification
                newSpecification.singularAdapterMode = mode
                self.machine.update(specification: newSpecification)
                self.singularStatus.mode = mode
            }
        }
    }
    
    // MARK: - KeyboardViewDelegate
    
    func pressed(key: Key) {
        emulator?.press(key: key)
    }
    
    func released(key: Key) {
        emulator?.release(key: key)
    }
    
    // MARK: - Status Bar Handling
    
    var haveStatusBar: Bool {
        return !tapeStatus.isHidden || driveStatus.contains(where: { !$0.isHidden })
    }
    
    var statusBarIsIdle: Bool {
        return activeStatusBarItems.isEmpty
    }
    
    private var activeStatusBarItems = Set<UIView>()
    
    override func statusBarShown() {
        fullscreenTopConstraint.isActive = false
        saveTopConstraint.isActive = true
        DebugLog.shared.log("show navigation bar")
        updateStatusBarAutoHide()
    }
    
    override func statusBarHidden() {
        saveTopConstraint.isActive = false
        fullscreenTopConstraint.isActive = true
        DebugLog.shared.log("hide navigation bar")
        updateStatusBarAutoHide()
    }
    
    var autoHideStatusBar: Bool {
        return keyboardView.isHidden && isBarHidden
    }
    
    func updateStatusBarAutoHide() {
        guard haveStatusBar else { return }
        
        if autoHideStatusBar {
            DebugLog.shared.log("status bar autohides")
            if statusBarIsIdle {
                DebugLog.shared.log("status bar is idle, hide")
                hideStatusBar()
            }
        }
        else {
            DebugLog.shared.log("status bar doesn't autohide, show")
            showStatusBar()
        }
    }
    
    var autoHideStatusBarTimer: Timer?
    
    func startStatusBarTimer() {
        guard haveStatusBar && autoHideStatusBar else { return }
        let timer = Timer(fire: Date().addingTimeInterval(3.0), interval: 0.0, repeats: false) { _ in
            self.autoHideStatusBarTimer = nil
            self.hideStatusBar()
        }
        autoHideStatusBarTimer = timer
        RunLoop.main.add(timer, forMode: .default)
    }
    
    func cancelStatusBarTimer() {
        guard let timer = autoHideStatusBarTimer else { return }
        timer.invalidate()
        autoHideStatusBarTimer = nil
    }
    
    func hideStatusBar() {
        UIView.animate(withDuration: TimeInterval(UINavigationController.hideShowBarDuration)) {
            self.statusBarView.isHidden = true
            if self.keyboardView.isHidden {
                self.safeInsetView.isHidden = true
            }
            self.stackView.layoutIfNeeded()
        }
    }
    
    func showStatusBar() {
        autoHideStatusBarTimer?.invalidate()
        UIView.animate(withDuration: TimeInterval(UINavigationController.hideShowBarDuration)) {
            self.statusBarView.isHidden = false
            self.safeInsetView.isHidden = false
            self.stackView.layoutIfNeeded()
        }
    }
    
    func statusBar(item: UIView, isActive: Bool) {
        DebugLog.shared.log("status bar item \(item) is " + (isActive ? "active" : "idle"))
        if isActive {
            if statusBarIsIdle && autoHideStatusBar {
                DebugLog.shared.log("status bar was hidden, show")
                cancelStatusBarTimer()
                showStatusBar()
            }
            activeStatusBarItems.insert(item)
        }
        else {
            activeStatusBarItems.remove(item)
            if statusBarIsIdle && autoHideStatusBar {
                DebugLog.shared.log("status bar autohides and is idle, hide")
                startStatusBarTimer()
            }
        }
    }
    
    func connect(controller: GCController) {
        let inputDevice = MfiInputDevice(controller: controller)
        controllers[controller] = inputDevice
        machine.connect(inputDevice: inputDevice)
    }
    
    func disconnect(controller: GCController) {
        guard let inputDevice = controllers[controller] else { return }
        inputDevice.disconnect()
        controllers[controller] = nil
    }
}

extension EmulatorViewController: EmulatorDelegate {
    func updateDriveStatus(unit: Int, track: Double, led1Intensity: Double, led2Intensity: Double) {
        guard haveStatusBar else { return }
        
        let statusView = driveStatus[unit - 8]
        
        statusView.trackView.currentTrack = track
        statusView.ledViews[0].intensity = led1Intensity
        statusView.ledViews[1].intensity = led2Intensity

        statusBar(item: statusView, isActive: led1Intensity != 0 || led2Intensity != 0)
    }
    
func updateTapeStatus(controlStatus: DatasetteControlStatus, isMotorOn: Bool, counter: Double) {
        guard haveStatusBar else { return }

        tapeStatus.counterView?.counter = counter
        tapeStatus.ledView?.intensity = controlStatus == .record ? 1 : 0

        let imageName: String
        switch controlStatus {
        case .forward:
            imageName = "Tape Forward"

        case .rewind:
            imageName = "Tape Rewind"
            
        case .record, .start:
            imageName = "Tape Play"
            
        default:
            imageName = "Tape Stop"
        }
        tapeStatus.controlView?.image = UIImage(named: imageName)
        tapeStatus.isMotorOn = isMotorOn

        statusBar(item: tapeStatus, isActive: isMotorOn)
    }
}


extension EmulatorViewController {
    @objc
    func controllerWasConnected(_ notification: Notification) {
        guard let controller = notification.object as? GCController else { return }
        
        connect(controller: controller)
    }
    
    @objc
    func controllerWasDisconnected(_ notification: Notification) {
        guard let controller = notification.object as? GCController else { return }
        
        disconnect(controller: controller)
    }
}

extension EmulatorViewController: KeyPressTranslatorDelegate {
    func preapareKeyPressTranslator() {
        if var keyboardSymbols = machine.specification.computer.keyboard?.keyboardSymbols {
            // TODO: only for Commodore machines, better moved into Keyboard?
            if Defaults.standard.capsLockAsCommodore {
                keyboardSymbols.modifierMap[.keyboardCapsLock] = .Commodore
            }
            keyPressTranslator = KeyPressTranslator(keyboardSymbols: keyboardSymbols)
            keyPressTranslator?.delegate = self
        }
        else {
            keyPressTranslator = nil
        }
    }
    
    func press(key: Key, delayed: Int) {
        emulator?.press(key: key, delayed: delayed)
    }
    
    func release(key: Key, delayed: Int) {
        emulator?.release(key: key, delayed: delayed)
    }
    
    override func pressesBegan(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
        super.pressesBegan(keyPressTranslator?.pressesBegan(presses, with: event) ?? presses, with: event)
    }
    
    override func pressesEnded(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
        super.pressesEnded(keyPressTranslator?.pressesEnded(presses, with: event) ?? presses, with: event)
    }
    
    override func pressesCancelled(_ presses: Set<UIPress>, with event: UIPressesEvent?) {
        super.pressesCancelled(keyPressTranslator?.pressesEnded(presses, with: event) ?? presses, with: event)
    }
}
