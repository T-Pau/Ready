/*
 Game.swift -- Core Data Entity Game
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

import CoreData

// TODO: move elsewhere
public enum ShowBorder: Int {
    case Never = 0
    case Always = 1
    case Auto = 2
}

public class Game: NSManagedObject {
    // MARK: - CoreData properties
    
    @NSManaged var added: Date
    @NSManaged var autoLoadName: String?
    @NSManaged var cartridgeFile: String?
    @NSManaged var cartridgeEEPROM: String?
    @NSManaged var deletedDate: Date?
    @NSManaged var directory: String
    @NSManaged var favorite: Bool
    @NSManaged var lastOpened: Date?
    @NSManaged var name: String
    @NSManaged var programFile: String?
    @NSManaged var publisher: String?
    @NSManaged var ramExpansionFile: String?
    @NSManaged var tapeFile: String?
    @NSManaged var titleImageFileName: String?
    @NSManaged var yearRaw: NSNumber?

    @NSManaged var category: CategoryEntity?
    @NSManaged var disks: NSOrderedSet?
    
    // MARK: - processed properties
    
    var year: Int? {
        get {
            guard let number = yearRaw else { return nil }
            return number.intValue == 0 ? nil : number.intValue
        }
        set {
            if let value = newValue {
                yearRaw = NSNumber(value: value)
            }
            else {
                yearRaw = nil
            }
        }
    }
    
    // MARK: - synthesized properties
    
    private var cachedTitleImage: UIImage??

    var titleImage: UIImage? {
        get {
            if (cachedTitleImage == nil) {
                if let fileName = titleImageFileName {
                    let image = UIImage(contentsOfFile: directoryURL.appendingPathComponent(fileName).path)
                    cachedTitleImage = image
                }
                else {
                    cachedTitleImage = .some(nil)
                }
            }
            return cachedTitleImage ?? nil
        }
        set {
            let fileManager = FileManager.default
            cachedTitleImage = newValue
            if let image = newValue {
                if let png = image.pngData() {
                    let tempURL = self.directoryURL.appendingPathComponent(UUID().uuidString + ".png")
                    do {
                        try png.write(to: tempURL)
                        if let oldName = self.titleImageFileName {
                            let oldURL = directoryURL.appendingPathComponent(oldName)
                            if fileManager.fileExists(atPath: oldURL.path) {
                                try FileManager.default.removeItem(at: oldURL)
                                titleImageFileName = nil
                            }
                        }
                        try FileManager.default.moveItem(at: tempURL, to: directoryURL.appendingPathComponent("title.png"))
                        titleImageFileName = "title.png"
                    }
                    catch {
                        if fileManager.fileExists(atPath: tempURL.path) {
                            do {
                                try fileManager.removeItem(at: tempURL)
                            }
                            catch { }
                        }
                        cachedTitleImage = nil
                    }
                }
                else {
                }
            }
            else {
                if let oldName = titleImageFileName {
                    let oldURL = directoryURL.appendingPathComponent(oldName)
                    if fileManager.fileExists(atPath: oldURL.path) {
                        do {
                            try FileManager.default.removeItem(at: oldURL)
                        }
                        catch { }
                    }
                }

                titleImageFileName = nil
            }
        }
    }
    
    // MARK: - Section Names
    
    @objc var lastOpenedSection: String {
        get {
            return dateSection(lastOpened, nameForNil: NSLocalizedString("Never", comment: "Never")
)
        }
    }
    
    @objc var addedSection: String {
        get {
            return dateSection(added, nameForNil: NSLocalizedString("Unknown", comment: "Unknown")
)
        }
    }
    
    
    @objc var nameSection: String {
        get {
            guard !name.isEmpty else { return NSLocalizedString("Unknown", comment: "Unknown") }
            
            if let initial = name.unicodeScalars.first {
                if CharacterSet.letters.contains(initial) {
                    return String(initial).uppercased()
                }
                else {
                    return "#"
                }
            }
            else {
                return "#"
            }
        }
    }

    @objc var publisherSection: String {
        get {
            guard let publisher = publisher, !publisher.isEmpty  else { return NSLocalizedString("Unknown", comment: "Unknown") }

            return publisher
        }
    }

    @objc var yearSection: String {
        get {
            if let year = year {
                return "\(year)"
            }
            else {
                return NSLocalizedString("Unknown", comment: "Unknown")
            }
        }
    }
    
    static var today: Date?
    static var todayYear = Int(0)
    
    private func dateSection(_ optionalDate: Date?, nameForNil: String) -> String {
        if let date = optionalDate {
            let today = Calendar.current.startOfDay(for: Date())
            if today != Game.today {
                Game.today = today
                Game.todayYear = Calendar.current.dateComponents([.year], from: today).year!
            }
            
            let diff = today.timeIntervalSince(date)
            
            if diff <= 0 {
                return NSLocalizedString("Today", comment: "Today")
            }
            else if diff <= 24*60*60 {
                return NSLocalizedString("Yesterday", comment: "Yesterday")
            }
            else if diff <= 24*60*60*7 {
                return NSLocalizedString("Previous 7 Days", comment: "Previous 7 Days")
            }
            else if diff <= 24*60*60*30 {
                return NSLocalizedString("Previous 30 Days", comment: "Previous 30 Days")
            }
            else {
                let playedComponents = Calendar.current.dateComponents([.year, .month], from: date)
                if let year = playedComponents.year, let month = playedComponents.month {
                    if (year == Game.todayYear) {
                        return Calendar.current.monthSymbols[month - 1]
                    }
                    else {
                        return "\(year)"
                    }
                }
            }
        }
        
        return nameForNil
    }

    
    // MARK: - initializers
    
    convenience init(name: String, directory: String, insertInto context: NSManagedObjectContext, importFiles: Bool = false) {
        self.init(entity: Game.entity(), insertInto: context)

        self.name = name
        self.directory = directory
        added = Date()
        
        if importFiles {
            do {
                let files = try FileManager.default.contentsOfDirectory(at: directoryURL, includingPropertiesForKeys: [], options: [.skipsPackageDescendants, .skipsHiddenFiles])

                var gmod2EEPROM: String?
            
                for file in files.sorted(by: { $0.path < $1.path }) {
                    let name = file.lastPathComponent
                    
                    switch file.pathExtension.lowercased() {
                    case "d1m", "d2m", "d4m", "d64", "d81", "g64", "p64", "x64":
                        addToDisks(Disk(fileName: name, insertInto: context))
                        
                    case "crt":
                        if cartridgeFile == nil {
                            cartridgeFile = name
                        }
                        
                    case "prg":
                        if programFile == nil {
                            programFile = name
                        }
                        
                    case "reu":
                        if ramExpansionFile == nil {
                            ramExpansionFile = name
                        }
                        
                    case "t64", "tap":
                        if tapeFile == nil {
                            tapeFile = name
                        }
                        
                    default:
                        if name == "title.png" {
                            titleImageFileName = name
                        }
                        else if name.hasSuffix("-gmod2-eeprom.bin") {
                            gmod2EEPROM = name
                        }
                        else if name.hasSuffix("-gmod2-flash.bin") {
                            // TODO: check that rest of name is equal
                            if gmod2EEPROM != nil {
                                cartridgeFile = name
                                cartridgeEEPROM = gmod2EEPROM
                            }
                        }
                    }
                }
            }
            catch { }
        }
    }
    
    convenience init?(name: String, insertInto context: NSManagedObjectContext) {
        do {
            // TODO: replace unsafe characters (:, /, &c)
            let directoryURL = try uniqeName(directory: AppDelegate.libraryURL, name: name, pathExtension: "")
            try FileManager.default.createDirectory(at: directoryURL, withIntermediateDirectories: false, attributes: [:])
            self.init(name: name, directory: directoryURL.lastPathComponent, insertInto: context)
        }
        catch {
            return nil
        }
    }
    
    // MARK: - other methods
    
    var directoryURL: URL {
        return AppDelegate.libraryURL.appendingPathComponent(directory)
    }
    
    func loadTitleImage(completion: @escaping (_: UIImage?) -> Void) {
        if let image = cachedTitleImage {
            completion(image)
        }
        else if let fileName = titleImageFileName {
            DispatchQueue.global(qos: .userInteractive).async {
                let image = UIImage(contentsOfFile: self.directoryURL.appendingPathComponent(fileName).path)
                
                self.cachedTitleImage = image
                DispatchQueue.main.async {
                    completion(image)
                }
            }
        }
        else {
            cachedTitleImage = nil
            completion(nil)
        }
    }
    
    func loadScreenshots(completion: @escaping (_: [UIImage]) -> Void) {
        let directoryURL = self.directoryURL
        DispatchQueue.global(qos: .userInteractive).async {
            let fileManager = FileManager.default
            
            do {
                let screenshots = try fileManager.contentsOfDirectory(at: directoryURL, includingPropertiesForKeys: [.creationDateKey], options: []).filter({ $0.lastPathComponent.hasPrefix("screenshot") && $0.pathExtension == "png" }).sorted(by: {
                    let a = try $0.resourceValues(forKeys: [.creationDateKey]).creationDate
                    let b = try $1.resourceValues(forKeys: [.creationDateKey]).creationDate
                    switch (a, b) {
                    case (nil, nil):
                        return false
                    case (nil, _):
                        return false
                    case (_, nil):
                        return true
                    case (_, _):
                        return a! < b!
                    }
                }).compactMap({ UIImage(contentsOfFile: $0.path )})
                DispatchQueue.main.async {
                    completion(screenshots)
                }
            }
            catch {
                
            }
        }
    }
    
    func addFile(name: String?, pathExtension: String, data: Data) -> String? {
        var fileName: String
        if let name = name, let index = name.lastIndex(of: ".") {
            fileName = String(name[name.startIndex..<index])
        }
        else {
            fileName = name ?? "unknown"
        }
        
        let fileURL: URL
            
        do {
            fileURL = try uniqeName(directory: directoryURL, name: fileName, pathExtension: pathExtension)
            try data.write(to: fileURL)
        }
        catch {
            return nil
        }
        return fileURL.lastPathComponent
    }
    
    static var allPublishers: [String : Int] {
        let context = (UIApplication.shared.delegate as! AppDelegate).persistentContainer.viewContext
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: "Game")
        fetchRequest.resultType = .dictionaryResultType
        let countDescription = NSExpressionDescription()
        let keypathExp = NSExpression(forKeyPath: "publisher") // can be any column
        let expression = NSExpression(forFunction: "count:", arguments: [keypathExp])
        countDescription.expression = expression
        countDescription.name = "count"
        countDescription.expressionResultType = .integer64AttributeType
        fetchRequest.returnsObjectsAsFaults = false
        fetchRequest.propertiesToGroupBy = [ "publisher" ]
        fetchRequest.propertiesToFetch = [ "publisher", countDescription ]
        
        var value = [String : Int]()

        do {
            if let results = try context.fetch(fetchRequest) as? [[String:AnyObject]] {
                for row in results {
                    if let publisher = row["publisher"] as? String,
                        let count = row["count"] as? NSNumber {
                        value[publisher] = count.intValue
                    }
                }
            }
        }
        catch {
            
        }

        return value
    }
    
    func delete() {
        deletedDate = Date()
    }
    
    func undelete() {
        deletedDate = nil
    }
    
    private var imagesLoaded = false
    private var _cartridge: CartridgeImage? = nil
    private var _ramExpansionUnit: RamExpansionUnit? = nil
    private var _program: ProgramFile? = nil
    private var _diskImages = [DiskImage]()
    private var _tape: TapeImage? = nil
    
    var cartridge: CartridgeImage? {
        if !imagesLoaded {
            loadImages()
        }
        return _cartridge
    }
    
    var ramExpansionUnit: RamExpansionUnit? {
        if !imagesLoaded {
            loadImages()
        }
        
        return _ramExpansionUnit
    }
    
    var program: ProgramFile? {
        if !imagesLoaded {
            loadImages()
        }
        return _program
    }
    
    var diskImages: [DiskImage] {
        if !imagesLoaded {
            loadImages()
        }
        return _diskImages
    }
    
    var tape: TapeImage? {
        if !imagesLoaded {
            loadImages()
        }
        return _tape
    }
    
    var tapes: [TapeImage] {
        if !imagesLoaded {
            loadImages()
        }
        
        if let tape = tape {
            return [ tape ]
        }
        else {
            return []
        }
    }

    private func loadImages() {
        _cartridge = CartridgeImage(directory: directoryURL, file: cartridgeFile, eeprom: cartridgeEEPROM)
        _program = ProgramFile(directory: directoryURL, file: programFile)
        let diskObjects = disks?.array as? [Disk] ?? []
        _diskImages = diskObjects.compactMap({ return DxxImage.image(from: directoryURL.appendingPathComponent($0.fileName)) })
        _tape = T64Image.image(directory: directoryURL, file: tapeFile)
        imagesLoaded = true

        if let ramExpansionFile = ramExpansionFile {
            _ramExpansionUnit = RamExpansionUnit(url: directoryURL.appendingPathComponent(ramExpansionFile))
        }
        else {
            _ramExpansionUnit = nil
        }
    }
}

// MARK: - To-Many Property accessors

extension Game {
    @objc(insertObject:inDisksAtIndex:)
    @NSManaged public func insertIntoDisks(_ value: Disk, at idx: Int)
    
    @objc(removeObjectFromDisksAtIndex:)
    @NSManaged public func removeFromDisks(at idx: Int)
    
    @objc(insertDisks:atIndexes:)
    @NSManaged public func insertIntoDisks(_ values: [Disk], at indexes: NSIndexSet)
    
    @objc(removeDisksAtIndexes:)
    @NSManaged public func removeFromDisks(at indexes: NSIndexSet)
    
    @objc(replaceObjectInDisksAtIndex:withObject:)
    @NSManaged public func replaceDisks(at idx: Int, with value: Disk)
    
    @objc(replaceDisksAtIndexes:withDisks:)
    @NSManaged public func replaceDisks(at indexes: NSIndexSet, with values: [Disk])
    
    @objc(addDisksObject:)
    @NSManaged public func addToDisks(_ value: Disk)
    
    @objc(removeDisksObject:)
    @NSManaged public func removeFromDisks(_ value: Disk)
    
    @objc(addDisks:)
    @NSManaged public func addToDisks(_ values: NSOrderedSet)
    
    @objc(removeDisks:)
    @NSManaged public func removeFromDisks(_ values: NSOrderedSet)
    
}

extension Game {
    var machineConfig: MachineConfig {
        return MachineConfig(contentsOf: directoryURL.appendingPathComponent("Machine Configuration.json"))
    }
    
    var machineSpecification: MachineSpecification {
        return MachineSpecification(layers: [ machineConfig, Defaults.standard.machineConfig ])
    }
}

// MARK: - Machine

extension Machine {
    convenience init(game: Game) {
        self.init(specification: game.machineSpecification)
        
        cartridgeImage = game.cartridge
        ramExpansionUnit = game.ramExpansionUnit
        programFile = game.program
        tapeImages = game.tapes
        diskImages = game.diskImages
    }
}

// TODO: move to own file

import C64UIComponents

// MARK: - GameViewItem

extension Game: GameViewItem {
    var type: GameViewItemType {
        return .game
    }
    
    var title: String {
        return name
    }
    
    var machine: Machine {
        return Machine(game: self)
    }
    
    var media: GameViewItemMedia {

        var cartridgeSection = GameViewItemMedia.Section(type: .cartridge, supportsMultiple: false, supportsReorder: false, addInFront: false, items: [])
        
        if let cartridge = cartridge {
            cartridgeSection.items.append(cartridge)
        }
        
        var ramExpansionSection = GameViewItemMedia.Section(type: .ramExpansionUnit, supportsMultiple: false, supportsReorder: false, addInFront: false, items: [])
        
        if let ramExpansionUint = ramExpansionUnit {
            ramExpansionSection.items.append(ramExpansionUint)
        }
     
        var programSection = GameViewItemMedia.Section(type: .programFile, supportsMultiple: false, supportsReorder: false, addInFront: false, items: [])

        if let program = program {
            programSection.items.append(program)
        }
        
        let diskSection = GameViewItemMedia.Section(type: .disks, supportsMultiple: true, supportsReorder: true, addInFront: false, items: diskImages as! [MediaItem])
        
        let tapeSection = GameViewItemMedia.Section(type: .tapes, supportsMultiple: false, supportsReorder: false, addInFront: false, items: tapes as! [MediaItem])

        return GameViewItemMedia(sections: [ cartridgeSection, ramExpansionSection, programSection, diskSection, tapeSection ])
    }
    
    func addMedia(mediaItem: MediaItem, at row: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge, .mixed, .programFile, .ramExpansionUnit:
            return

        case .disks:
            guard let managedObjectContext = managedObjectContext,
                let disk = mediaItem as? DiskImage,
                let fileName = disk.url?.lastPathComponent  else { return }
            insertIntoDisks(Disk(fileName: fileName, insertInto: managedObjectContext), at: row)
            _diskImages.insert(disk, at: row)
            
        case .tapes:
            // TODO
            return
        }
    }
    
    func moveMedia(from sourceRow: Int, to destinationRow: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge, .mixed, .programFile, .ramExpansionUnit:
            return

        case .disks:
            guard let disk = disks?[sourceRow] as? Disk else { return }
            let diskImage = _diskImages[sourceRow]
            removeFromDisks(at: sourceRow)
            _diskImages.remove(at: sourceRow)
            insertIntoDisks(disk, at: destinationRow)
            _diskImages.insert(diskImage, at: destinationRow)
            
        case .tapes:
            // TODO
            return
        }
    }
    
    func removeMedia(at row: Int, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge:
            removeFile(cartridgeFile)
            cartridgeFile = nil
            removeFile(cartridgeEEPROM)
            cartridgeEEPROM = nil
            _cartridge = nil
            
        case .disks:
            guard let disk = disks?[row] as? Disk else { return }
            removeFile(disk.fileName)
            removeFromDisks(at: row)
            _diskImages.remove(at: row)
            
        case .mixed:
            return
            
        case .programFile:
            removeFile(programFile)
            programFile = nil
            _program = nil
            
        case .ramExpansionUnit:
            removeFile(ramExpansionFile)
            ramExpansionFile = nil
            _ramExpansionUnit = nil
            
        case .tapes:
            removeFile(tapeFile)
            tapeFile = nil
            _tape = nil
        }
    }
    
    func renameMedia(name: String, at: Int, sectionType: GameViewItemMedia.SectionType) {
        // TODO
    }
    
    func replaceMedia(mediaItem: MediaItem, sectionType: GameViewItemMedia.SectionType) {
        switch sectionType {
        case .cartridge:
            guard let cartridge = mediaItem as? CartridgeImage,
                let fileName = cartridge.url?.lastPathComponent else { return }
            removeFile(cartridgeFile)
            removeFile(cartridgeEEPROM)
            cartridgeFile = fileName
            cartridgeEEPROM = cartridge.eepromUrl?.lastPathComponent
            _cartridge = cartridge
            
        case .disks:
            break
            
        case .programFile:
            guard let program = mediaItem as? ProgramFile,
                let fileName = program.url?.lastPathComponent else { return }
            removeFile(programFile)
            programFile = fileName
            _program = program

        case .ramExpansionUnit:
            guard let ramExpansionUnit = mediaItem as? RamExpansionUnit,
                let fileName = ramExpansionUnit.url?.lastPathComponent else { return }
            removeFile(ramExpansionFile)
            ramExpansionFile = fileName
            _ramExpansionUnit = ramExpansionUnit
            break

        case .tapes:
            guard let tape = mediaItem as? TapeImage,
                let fileName = tape.url?.lastPathComponent else { return }
            removeFile(tapeFile)
            tapeFile = fileName
            _tape = tape

        case .mixed:
            break
        }
    }
    
    func save() throws {
        try managedObjectContext?.save()
    }
}
