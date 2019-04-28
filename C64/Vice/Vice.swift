/*
 Vice.swift -- High Level Interface to Vice
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

import Foundation
import C64UIComponents

protocol ViceDelegate {
    func updateDriveStatus(unit: Int, track: Double, led1Intensity: Double, led2Intensity: Double)
    
    func updateTapeStatus(controlStatus: Int32, isMotorOn: Bool, counter: Double)
}

enum ViceEvent {
    case attach(unit: Int, image: DiskImage?)
    case freeze
    case setResource(key: Machine.ResourceName, value: Machine.ResourceValue)
    case joystick(port: Int, buttons: JoystickButtons)
    case mouseButton(button: Int, pressed: Bool)
    case playPause(_ running: Bool)
    case quit
    case reset
    case restore(pressed: Bool)
}


extension JoystickButtons {
    var value: Int {
        var value = 0
        if up {
            value |= 0x01
        }
        if down {
            value |= 0x02
        }
        if left {
            value |= 0x04
        }
        if right {
            value |= 0x08
        }
        if fire {
            value |= 0x10
        }
        
        return value
    }
}


@objc class Vice: NSObject {
    var machine = Machine()
    var imageView: UIImageView? {
        didSet {
            viceThread?.imageView = imageView
        }
    }
    
    var delegate: ViceDelegate?
    
    var joysticks = [JoystickButtons](repeating: JoystickButtons(), count: 10)
    
    private var tempFile: URL?
    
    struct DriveInfo : Equatable {
        var track: Double
        var led1Intensity: Double
        var led2Intensity: Double
        
        init() {
            track = 1
            led1Intensity = 0
            led2Intensity = 0
        }
    }
    
    struct TapeInfo : Equatable {
        var counter = 0.0
        var controlStatus = DATASETTE_CONTROL_STOP
        var isMotorOn = false
    }
    
    // accessed from vice thread
    var lastDriveInfo = [DriveInfo](repeating: DriveInfo(), count: 4)
    var currentDriveInfo = [DriveInfo](repeating: DriveInfo(), count: 4)
    var lastTapeInfo = TapeInfo()
    var currentTapeInfo = TapeInfo()

    deinit {
        if let tempFile = tempFile {
            do {
                try FileManager.default.removeItem(at: tempFile)
            }
            catch { }
        }
    }
    
    override init() {
        viceThread = ViceThread()
        
        super.init()

        viceThread?.vice = self
    }
    
    private var keyboard = [[Int]](repeating: [Int](repeating: 0, count: 8), count: 8)
    
    private var autostartPrgName: [UInt8]?
    
    func start() {
        machine.resources[.AutostartPrgMode] = .Int(AUTOSTART_PRG_MODE_INJECT);
        //machineSpecification.resources[.VICIIBorderMode] = .Int(VICII_TALL_BORDERS)
        machine.resources[.Mouse] = .Bool(true)
        machine.resources[.LogFileName] = .String(AppDelegate.libraryURL.appendingPathComponent("vice-log.txt").path)
        machine.resources[.Drive8IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive9IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive10IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive11IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)

        viceThread?.newBorderMode = machine.specification.borderMode.cValue
        viceThread?.currentBorderMode = machine.specification.borderMode.cValue
        
        var argv = [ "vice" ]
        
        var autostartDisk = machine.autostart

        if let cartridgeURL = machine.cartridgeImage?.url {
            if let cartridgeEeprom = machine.cartridgeImage?.eepromUrl {
                argv.append("-cartgmod2")
                argv.append(cartridgeURL.path)
                argv.append("-gmod2eepromimage")
                argv.append(cartridgeEeprom.path)
            }
            else {
                argv.append("-cartcrt")
                argv.append(cartridgeURL.path)
            }
        }
        
        if let programFileURL = machine.programFile?.url {
            argv.append("-autostart")
            argv.append(programFileURL.path)
            autostartDisk = false
        }
        
        if let tapeImageURL = (machine.tapeImages.isEmpty ? nil : machine.tapeImages[0])?.url {
            if autostartDisk {
                argv.append("-autostart")
                autostartDisk = false
            }
            else {
                argv.append("-1")
            }
            argv.append(tapeImageURL.path)
        }
        
        // TODO: Tape
                
        for (index, drive) in machine.diskDrives.enumerated() {
            guard let diskImage = drive.image, let url = diskImage.url else { continue }
            
            if (autostartDisk) {
                if let directory = diskImage.readDirectory() {
                    var fileData: Data? = nil
                    var fileName: [UInt8]? = nil

                    for entry in directory.entries {
                        if entry.closed && entry.fileType == 2 {
                            fileData = diskImage.readFile(track: entry.track, sector: entry.sector)
                            fileName = entry.namePETASCII
                            break
                        }
                    }
                
                    if let fileData = fileData, fileData.count > 2 &&  fileData[0] == 0x01 && fileData[1] == 0x08 {
                        do {
                            let temporaryDirectoryURL = try FileManager.default.url(for: .itemReplacementDirectory, in: .userDomainMask, appropriateFor: url, create: true)
                            let fileURL = temporaryDirectoryURL.appendingPathComponent(UUID().uuidString + ".prg")
                            try fileData.write(to: fileURL)
                            tempFile = fileURL
                            argv.append("-autostart")
                            argv.append(fileURL.path)
                            autostartDisk = false
                            autostartPrgName = fileName
                        }
                        catch { }
                    }
                }
            }

            if autostartDisk {
                argv.append("-autostart")
                autostartDisk = false
            }
            else {
                argv.append("-\(index + 8)")
            }
            argv.append(url.path)
        }
        
        NSLog("\(argv)")
        viceThread?.argv = argv
        viceThread?.machine = machine
        viceThread?.start()
    }
    
    private var eventMutex = PThreadMutex()
    private var eventQueue = [ViceEvent]()
    
    private func send(event: ViceEvent) {
        eventMutex.sync {
            eventQueue.append(event)
        }
    }
    
    func attach(drive: Int, image: DiskImage?) {
        machine.diskDrives[drive - 8].image = image
        send(event: .attach(unit: drive, image: image))
    }
    
    func freeze() {
        send(event: .freeze)
    }

    func press(key: Key) {
        if key == .Restore {
            send(event: .restore(pressed: true))
        }
        else if let row = key.row, let column = key.column {
            if (keyboard[row][column] == 0) {
                viceThread?.pressKey(row: Int32(row), column: Int32(column))
            }
            keyboard[row][column] += 1
        }
    }
    
    func quit() {
        send(event: .quit)
    }

    func release(key: Key) {
        if key == .Restore {
            send(event: .restore(pressed: false))
        }
        else if let row = key.row, let column = key.column {
            if (keyboard[row][column] > 0) {
                keyboard[row][column] -= 1
                if (keyboard[row][column] == 0) {
                    viceThread?.releaseKey(row: Int32(row), column: Int32(column))
                }
            }
        }
    }

    func reset() {
        send(event: .reset)
    }
    
    func joystick(_ index: Int, buttons: JoystickButtons) {
        guard index > 0 else { return }
        guard buttons != joysticks[index] else { return }
        
        joysticks[index] = buttons
        send(event: .joystick(port: index, buttons: buttons))
    }
    
    func mouse(moved distance: CGPoint) {
        guard let viceThread = viceThread else { return }
        viceThread.mouseX = (viceThread.mouseX + Int32(distance.x)) & 0xffff
        viceThread.mouseY = (viceThread.mouseY - Int32(distance.y)) & 0xffff
    }
    
    func mouse(pressed button: Int) {
        send(event: .mouseButton(button: button, pressed: true))
    }
    
    func mouse(release button: Int) {
        send(event: .mouseButton(button: button, pressed: false))
    }
    
    func lightPen(moved position: CGPoint?, size: CGSize) {
        if let position = position {
            update_light_pen(Int32(position.x), Int32(position.y), Int32(size.width), Int32(size.height))
        }
        else {
            update_light_pen(-1, -1, 1, 1);
        }
    }

    func setResource(name: Machine.ResourceName, value: Machine.ResourceValue) {
        send(event: .setResource(key: name, value: value))
    }
    
    // accessed from vice thread
    @objc(updateDriveUnit:track:) func updateDrive(unit: Int, track: Double) {
        currentDriveInfo[unit].track = track
    }
    
    @objc(updateDriveUnit:led1Intensity:led2Intensity:) func updateDrive(unit: Int, led1Intensity: Double, led2Intensity: Double) {
        currentDriveInfo[unit].led1Intensity = led1Intensity
        currentDriveInfo[unit].led2Intensity = led2Intensity
    }
    
    @objc(updateTapeControlStatus:) func updateTape(controlStatus: Int32) {
        currentTapeInfo.controlStatus = controlStatus
    }

    @objc(updateTapeIsMotorOn:) func updateTape(isMotorOn: Bool) {
        currentTapeInfo.isMotorOn = isMotorOn
    }

    @objc(updateTapeCounter:) func updateTape(counter: Double) {
        currentTapeInfo.counter = counter
    }
    
    @objc func updateStatusBar() {
        guard let delegate = delegate else { return }
        for i in (0..<currentDriveInfo.count) {
            if currentDriveInfo[i] != lastDriveInfo[i] {
                lastDriveInfo[i] = currentDriveInfo[i]
                DispatchQueue.main.async { [weak self] in
                    guard let self = self else { return }
                    delegate.updateDriveStatus(unit: i + 8, track: self.lastDriveInfo[i].track, led1Intensity: self.lastDriveInfo[i].led1Intensity, led2Intensity: self.lastDriveInfo[1].led2Intensity)
                }
            }
        }
        
        if currentTapeInfo != lastTapeInfo {
            lastTapeInfo = currentTapeInfo
            DispatchQueue.main.async { [weak self] in
                guard let self = self else { return }
                delegate.updateTapeStatus(controlStatus: self.lastTapeInfo.controlStatus, isMotorOn: self.lastTapeInfo.isMotorOn, counter: self.lastTapeInfo.counter)
            }
        }
    }
    
    @objc func handleEvents() -> Bool {
        return eventMutex.sync {
            var continueProcessing = true
            
            for event in eventQueue {
                switch event {
                case .attach(let unit, let image):
                    if let url = image?.url {
                        file_system_attach_disk(UInt32(unit), url.path)
                    }
                    else {
                        // TODO: detach image
                    }
                    
                case .freeze:
                    cartridge_trigger_freeze()
                    
                case .setResource(let key, let value):
                    machine.viceSetResource(name: key, value: value)
                    
                case .joystick(let port, let buttons):
                    joystick_set_value_absolute(UInt32(port), UInt8(buttons.value))
                    
                case .mouseButton(let button, let pressed):
                    mouse_button_press(Int32(button), pressed ? 1 : 0)
                    
                case .playPause(_):
                    // TODO: implemement
                    break
                    
                case .quit:
                    maincpu_running = 0
                    continueProcessing = false
                    
                case .reset:
                    machine_trigger_reset(UInt32(MACHINE_RESET_MODE_SOFT))
                    
                case .restore(let pressed):
                    if pressed {
                        keyboard_restore_pressed()
                    }
                    else {
                        keyboard_restore_released()
                    }
                }
            }
            eventQueue.removeAll()
        
            return continueProcessing
        }
    }
    
    @objc func setupVice() {
        c64model_set(machine.specification.computer.viceMachineModel.int32Value)
    }
    
    @objc func autostartInjectDeviceInfo() {
        if let name = autostartPrgName {
            let nameAddress = 0x0230
            // length of file name
            mem_inject(0x00b7, UInt8(name.count))
            // logical file number
            mem_inject(0x00b8, 96)
            // secondary address
            mem_inject(0x00b9, 1)
            // set device number
            mem_inject(0x00ba, 8)
            // pointer to file name
            mem_inject(0x00bb, UInt8(nameAddress & 0xff))
            mem_inject(0x00bc, UInt8(nameAddress / 0x100))
            // file name
            for (index, byte) in name.enumerated() {
                mem_inject(UInt32(nameAddress + index), byte)
            }
            
            guard let diskId = machine.diskDrives[0].image?.diskId else { return }
            drive_set_id(8, diskId[0], diskId[1]);
        }
    }
}
