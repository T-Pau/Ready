/*
 Game.swift -- Core Data Entity Game
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

import CoreData
import Emulator

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
    @NSManaged var ideDisks: NSOrderedSet?

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
                                try fileManager.removeItem(at: oldURL)
                                titleImageFileName = nil
                            }
                        }
                        let destinationURL = directoryURL.appendingPathComponent("title.png")
                        if fileManager.fileExists(atPath: destinationURL.path) {
                            try fileManager.removeItem(at: destinationURL)
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
                        
                    case "t64", "tap", "tzx":
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
            let directoryURL = try uniqueName(directory: Defaults.libraryURL, name: name, pathExtension: "", options: [.create, .directory])
            self.init(name: name, directory: directoryURL.lastPathComponent, insertInto: context)
        }
        catch {
            return nil
        }
    }
    
    // MARK: - other methods
    
    var directoryURL: URL {
        return Defaults.libraryURL.appendingPathComponent(directory)
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
            fileURL = try uniqueName(directory: directoryURL, name: fileName, pathExtension: pathExtension)
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
    private var imagesChanged = false
    private var _mediaItems = [MediaItem]()
    
    var mediaItems: [MediaItem] {
        loadImages()
        return _mediaItems
    }
    
    private func loadImages() {
        guard !imagesLoaded else { return }
        _mediaItems = MediaList(game: self).mediaItems
        imagesLoaded = true
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

    
    @objc(insertObject:inIdeDisksAtIndex:)
    @NSManaged public func insertIntoIdeDisks(_ value: Disk, at idx: Int)
    
    @objc(removeObjectFromIdeDisksAtIndex:)
    @NSManaged public func removeFromIdeDisks(at idx: Int)
    
    @objc(insertIdeDisks:atIndexes:)
    @NSManaged public func insertIntoIdeDisks(_ values: [Disk], at indexes: NSIndexSet)
    
    @objc(removeIdeDisksAtIndexes:)
    @NSManaged public func removeFromIdeDisks(at indexes: NSIndexSet)
    
    @objc(replaceObjectInIdeDisksAtIndex:withObject:)
    @NSManaged public func replaceIdeDisks(at idx: Int, with value: Disk)
    
    @objc(replaceIdeDisksAtIndexes:withIdeDisks:)
    @NSManaged public func replaceIdeDisks(at indexes: NSIndexSet, with values: [Disk])
    
    @objc(addIdeDisksObject:)
    @NSManaged public func addToIdeDisks(_ value: Disk)
    
    @objc(removeIdeDisksObject:)
    @NSManaged public func removeFromIdeDisks(_ value: Disk)
    
    @objc(addIdeDisks:)
    @NSManaged public func addToIdeDisks(_ values: NSOrderedSet)
    
    @objc(removeIdeDisks:)
    @NSManaged public func removeFromIdeDisks(_ values: NSOrderedSet)
}

extension Game {
    var machineConfig: MachineConfigOld {
        return MachineConfigOld(contentsOf: directoryURL.appendingPathComponent("Machine Configuration.json"))
    }
    
    var machineSpecification: MachineSpecification {
        return MachineSpecification(layers: [ machineConfig, Defaults.standard.machineConfig ])
    }
}

// MARK: - Machine

extension MachineOld {
    convenience init(game: Game) {
        self.init(specification: game.machineSpecification)
        
        mediaItems = game.mediaItems
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
    
    var machine: MachineOld {
        return MachineOld(game: self)
    }
    
    var media: GameViewItemMedia {
        return GameViewItemMedia(sections: [GameViewItemMedia.Section(type: .mixed, supportsMultiple: true, supportsReorder: true, addInFront: false, items: mediaItems)])
    }
    
    func addMedia(mediaItem: MediaItem, at row: Int, sectionType: GameViewItemMedia.SectionType) {
        _mediaItems.insert(mediaItem, at: row)
        imagesChanged = true
    }
    
    func moveMedia(from sourceRow: Int, to destinationRow: Int, sectionType: GameViewItemMedia.SectionType) {
        let item = _mediaItems[sourceRow]
        _mediaItems.remove(at: sourceRow)
        _mediaItems.insert(item, at: destinationRow)
        imagesChanged = true
    }
        
    func removeMedia(at row: Int, sectionType: GameViewItemMedia.SectionType) {
        _mediaItems.remove(at: row)
        imagesChanged = true
    }
    
    func renameMedia(name: String, at: Int, sectionType: GameViewItemMedia.SectionType) {
        // TODO
        imagesChanged = true
    }
    
    func replaceMedia(mediaItem: MediaItem, sectionType: GameViewItemMedia.SectionType) {
    }
    
    func save() throws {
        try managedObjectContext?.save()
        if imagesChanged {
            let list = MediaList(mediaItems: mediaItems)
            try list.save(directory: directoryURL)
            imagesChanged = false
        }
    }
}
