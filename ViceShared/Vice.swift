/*
 Vice.swift -- High Level Interface to vice
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
//import C64UIComponents
import Emulator

import ViceX64sc

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
        if fire2 {
            value |= 0x20
        }
        if fire3 {
            value |= 0x40
        }

        return value
    }
}



@objc public class Vice: Emulator {
    private var tempFile: URL?
    
    public struct DriveInfo : Equatable {
        public var track: Double
        public var led1Intensity: Double
        public var led2Intensity: Double
        
        public init() {
            track = 1
            led1Intensity = 0
            led2Intensity = 0
        }
    }
    
    public struct TapeInfo : Equatable {
        public var counter = 0.0
        public var controlStatus = DatasetteControlStatus.stop
        public var isMotorOn = false
    }
    
    // accessed from vice thread
    public var lastDriveInfo = [DriveInfo](repeating: DriveInfo(), count: 4)
    public var currentDriveInfo = [DriveInfo](repeating: DriveInfo(), count: 4)
    public var lastTapeInfo = TapeInfo()
    public var currentTapeInfo = TapeInfo()

    deinit {
        if let tempFile = tempFile {
            do {
                try FileManager.default.removeItem(at: tempFile)
            }
            catch { }
        }
    }
    
    public init() {
        viceThread = ViceThread()
        super.init(emulatorThread: viceThread)
        if let multipleScreens = viceVariant.multipleScreens {
            screens = multipleScreens
        }
    }
    
    private var keyboard = [[Int]](repeating: [Int](repeating: 0, count: 8), count: 8)
    
    private var autostartPrgName: [UInt8]?
    
    public override func start() {
        super.start()
        
        machine.resources[.AutostartPrgMode] = .Int(AUTOSTART_PRG_MODE_INJECT);
        //machineSpecification.resources[.VICIIBorderMode] = .Int(VICII_TALL_BORDERS)
        machine.resources[.Mouse] = .Bool(true)
        machine.resources[.LogFileName] = .String(Defaults.documentURL.appendingPathComponent("vice-log.txt").path)
        machine.resources[.Drive8IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive9IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive10IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)
        machine.resources[.Drive11IdleMethod] = .Int(DRIVE_IDLE_TRAP_IDLE)

        viceThread?.borderMode = machine.specification.borderMode.cValue
        
        var argv = [ "vice" ]
        
        var autostartDisk = machine.autostart
        
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
        viceThread?.start()
    }
    
    public override func attach(drive: Int, image: DiskImage?) {
        machine.diskDrives[drive - 8].image = image
        send(event: .attach(unit: drive, image: image))
    }

    public override func mouse(moved distance: CGPoint) {
        guard let viceThread = viceThread else { return }
        viceThread.mouseX = (viceThread.mouseX + Int32(distance.x)) & 0xffff
        viceThread.mouseY = (viceThread.mouseY - Int32(distance.y)) & 0xffff
        viceThread.mouseTimestamp = vsyncarch_gettime();
    }
    
    public override func mouse(setX x: Int32) {
        guard let viceThread = viceThread else { return }
        viceThread.mouseX = x
    }

    public override func mouse(setY y: Int32) {
        guard let viceThread = viceThread else { return }
        viceThread.mouseY = y
    }

    public override func mouse(pressed button: Int) {
        send(event: .mouseButton(button: button, pressed: true))
    }
    
    public override func mouse(release button: Int) {
        send(event: .mouseButton(button: button, pressed: false))
    }
    
    public override func lightPen(moved position: CGPoint?, size: CGSize, button1: Bool, button2: Bool, isKoalaPad: Bool) {
        if let position = position {
            update_light_pen(Int32(position.x), Int32(position.y), Int32(size.width), Int32(size.height), button1 ? 1 : 0, button2 ? 1 : 0, isKoalaPad ? 1 : 0)
        }
        else {
            update_light_pen(-1, -1, 1, 1, button1 ? 1 : 0, button2 ? 1 : 0, isKoalaPad ? 1 : 0)
        }
    }

    public override func setResource(name: MachineOld.ResourceName, value: MachineOld.ResourceValue) {
        send(event: .setResource(key: name, value: value))
    }
    
    public override func setResourceNow(name: MachineOld.ResourceName, value: MachineOld.ResourceValue) {
        // print("setting resource: \(name) = \(value)")
        switch value {
        case .Bool(let value):
            resources_set_int(name.rawValue, value ? 1 : 0)
        case .Int(let value):
            resources_set_int(name.rawValue, value)
        case .String(let value):
            resources_set_string(name.rawValue, value)
        }
    }
    
    override public func handle(event: Event) -> Bool {
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
            
        case .joystick(let port, let buttons, _):
            joystick_set_value_absolute(UInt32(port), UInt8(buttons.value))
            
        case .key(let key, pressed: let pressed):
            if key == .Restore {
                if pressed {
                    keyboard_restore_pressed()
                }
                else {
                    keyboard_restore_released()
                }
            }
            else if let row = viceVariant.keyboardMatrix.row[key], let column = viceVariant.keyboardMatrix.column[key] {
                if pressed {
                    viceThread?.pressKey(row: Int32(row), column: Int32(column))
                }
                else {
                    viceThread?.releaseKey(row: Int32(row), column: Int32(column))
                }
            }
            
        case .mouseButton(let button, let pressed):
            mouse_button_press(Int32(button), pressed ? 1 : 0)
            
        case .playPause(_):
            // TODO: implemement
            break
            
        case .quit:
            maincpu_running = 0
            return false
            
        case .reset:
            machine_trigger_reset(UInt32(MACHINE_RESET_MODE_SOFT))
            
        case .setResource(let key, let value):
            setResourceNow(name: key, value: value)
        }
        
        return true
    }

}

// These are called on the Vice thread
extension Vice: ViceThreadDelegate {
    @objc public func autostartInjectDeviceInfo() {
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


    @objc public func getDirectoryPath() -> String {
        return machine.directoryURL?.path ?? ""
    }
    
    
    @objc public func setupVice() {
        guard let model = viceVariant.viceModel[machine.specification.computer.model] else { return }
        model_set(model)
    }

    
    @objc public func updateDriveUnit(_ unit: Int32, led1Intensity intensity1: Double, led2Intensity intensity2: Double) {
        currentDriveInfo[Int(unit)].led1Intensity = intensity1
        currentDriveInfo[Int(unit)].led2Intensity = intensity2
    }

    @objc public func updateDriveUnit(_ unit: Int32, track: Double) {
        currentDriveInfo[Int(unit)].track = track
    }
    
    @objc public func updateStatusBar() {
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

    @objc public func updateTapeControlStatus(_ controlStatus: Int32) {
        currentTapeInfo.controlStatus = DatasetteControlStatus(rawValue: Int(controlStatus)) ?? .stop
    }
    
    @objc public func updateTapeCounter(_ counter: Double) {
        currentTapeInfo.counter = counter
    }
    
    @objc public func updateTapeIsMotor(on motor: Int32) {
        currentTapeInfo.isMotorOn = motor != 0
    }
    
    @objc public func viceSetResources() {
        machine.viceSetResources()
    }
}
