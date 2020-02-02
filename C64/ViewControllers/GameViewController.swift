/*
 GameViewController.swift -- Game Detail View
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
import C64UIComponents


class GameViewController: UIViewController, UITextFieldDelegate, ShareableImageViewDelegate, UICollectionViewDelegate, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout, SeguePreparer {
    enum SegueType: String {
        case chooseCategory
        case start
        case startSelected
        case startTools
        case startToolsSelected
        case showTitleImage
        case machineParts
        
        var useTools: Bool {
            switch self {
            case .startTools, .startToolsSelected:
                return true
            default:
                return false
            }
        }
        
        var useSelected: Bool {
            switch self {
            case .startSelected, .startToolsSelected:
                return true
            default:
                return false
            }
        }
    }
    
    enum Mode {
        case inbox
        case tools
        case game(Game)
    }
    
    enum StateKey: String {
        case mode
        case objectID
    }

    @IBOutlet weak var titleImageView: ShareableImageView?
    @IBOutlet weak var nameTextField: UITextField?
    @IBOutlet weak var publisherTextField: SearchTextField?
    @IBOutlet weak var yearTextField: UITextField?
    @IBOutlet weak var screenshotsCollectionView: UICollectionView?
    @IBOutlet weak var toolsButton: UIButton?
    @IBOutlet weak var mediaTableView: UITableView?
    @IBOutlet weak var mediaView: MediaView?
    @IBOutlet weak var categoryButton: PlaceholderButton?
    @IBOutlet var deleteBarButton: UIBarButtonItem?
    @IBOutlet var recoverBarButton: UIBarButtonItem?
    @IBOutlet var machineBarButton: UIBarButtonItem?
    @IBOutlet weak var favoriteButton: UIButton?
    @IBOutlet weak var diskDirectoryTableView: UITableView!
    @IBOutlet weak var generalView: UIView?
    @IBOutlet weak var inboxButtons: UIView?
    @IBOutlet weak var inboxStartButton: UIButton?
    @IBOutlet weak var inboxToolsButton: UIButton?
    @IBOutlet weak var inboxCreateLibraryEntryButton: UIButton?
    @IBOutlet weak var addMediaButton: UIButton?
    @IBOutlet weak var selectMediaButton: UIButton?
    @IBOutlet weak var cancelSelectMediaButton: UIButton?
    
    var diskDirectoryTableViewDelegate = DiskDirectoryTableViewDelegate()
    
    var gameViewItem: GameViewItem? {
        didSet {
            if let game = gameViewItem as? Game {
                observer = ManagedObjectObserver(object: game) { [unowned self] changeType in
                    switch changeType {
                    case .delete:
                        // TODO: pop view controller
                        self.gameViewItem = nil
                        self.updateItem(animated: true)
                        break
                    case .update:
                        self.updateItem(animated: true)
                    }
                }
            }
            else if let inbox = gameViewItem as? Inbox {
                inbox.changeHandler = { [weak self] (inbox, updates) in
                    self?.updateMedia(tableViewUpdates: updates)
                }
            }
        }
    }
    
    private var machine = Machine()
    var screenshots = [UIImage]()
    var media = GameViewItemMedia(sections: [])
    
    private var observer: ManagedObjectObserver?
    
    var dropInteractions = [UIDropInteraction : UIView]()
    var longPressRecognizers = [UILongPressGestureRecognizer : UIView]()

    override func viewDidLoad() {
        super.viewDidLoad()

        nameTextField?.delegate = self
        yearTextField?.delegate = self
        publisherTextField?.delegate = self
        publisherTextField?.inlineMode = true
        titleImageView?.delegate = self
        screenshotsCollectionView?.dataSource = self
        screenshotsCollectionView?.delegate = self
        mediaTableView?.dataSource = self
        mediaTableView?.dragDelegate = self
        mediaTableView?.dropDelegate = self
        mediaTableView?.delegate = self
        mediaTableView?.register(MediaTableHeaderView.self, forHeaderFooterViewReuseIdentifier: "Header")
        let backgroundView = PlaceholderView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
        backgroundView.color = UIColor(named: "EmptyCollection") ?? UIColor.darkGray
        backgroundView.text = "Use + or drag files here\nto add cartridges,\n disks, or tapes."
        mediaTableView?.backgroundView = backgroundView
        
        diskDirectoryTableViewDelegate.tableView = diskDirectoryTableView
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        let sortedPublishers: [String] = Array(Game.allPublishers).sorted() {
            let (k1, v1) = $0
            let (k2, v2) = $1
            if (v1 < v2) {
                return true
            }
            else if (v1 == v2) {
                return k1 < k2
            }
            else {
                return false
            }
        }.map {
            let (k, _) = $0
            return k
        }
        
        publisherTextField?.filterStrings(sortedPublishers)
        
        updateItem(animated: false)
    }
    
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        do {
            try gameViewItem?.save()
        }
        catch {
        }
    }
    
    override var canBecomeFirstResponder: Bool {
        return true
    }
    
    // MARK: - Actions
    
    @IBAction func deleteGame(_ sender: Any) {
        guard let game = gameViewItem as? Game else { return }
        game.delete()
        navigationController?.popViewController(animated: true)
    }
    
    @IBAction func recoverGame(_ sender: Any) {
        guard let game = gameViewItem as? Game else { return }
        game.undelete()
        navigationController?.popViewController(animated: true)
    }
    
    @IBAction func toggleFavorite(_ sender: Any) {
        guard let game = gameViewItem as? Game else { return }
        game.favorite = !game.favorite
    }
    
    @IBAction func addMedia(_ sender: Any) {
        let documentPicker = UIDocumentPickerViewController(documentTypes: Array(C64FileType.typeIdentifiers), in: .import)
        documentPicker.allowsMultipleSelection = true
        documentPicker.delegate = self
        present(documentPicker, animated: true)
    }
    
    @IBAction func selectMedia(_ sender: Any) {
        mediaTableView?.allowsMultipleSelection = true
        selectMediaButton?.isHidden = true
        cancelSelectMediaButton?.isHidden = false
    }
    
    @IBAction func cancelSelectMedia(_ sender: Any) {
        mediaTableView?.allowsMultipleSelection = false
        selectMediaButton?.isHidden = false
        cancelSelectMediaButton?.isHidden = true
    }
    
    @IBAction func createLibraryEntry(_ sender: Any) {
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        guard let mediaItems = mediaTableView?.indexPathsForSelectedRows?.map({ media.item(for: $0) }) else { return }
        let alertController = UIAlertController(title: "Create New Title", message: nil, preferredStyle: .alert)
        alertController.addTextField(configurationHandler: { textField in
            textField.placeholder = "Name"
            textField.autocapitalizationType = .words
            textField.autocorrectionType = .default
            textField.font = UIFont.systemFont(ofSize: 17)
        })
        guard let textField = alertController.textFields?[0] else { return }
        alertController.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
        alertController.addAction(UIAlertAction(title: "OK", style: .default, handler: { _ in
            guard let name = textField.text?.trimmingCharacters(in: .whitespaces), !name.isEmpty else { return }
            do {
                _ = Game(name: name, insertInto: context, mediaItems: mediaItems, move: true)
                try context.save()
                // TODO: jump to game
            }
            catch { }
        }))
        present(alertController, animated: true)
    }
    
    @IBAction func exportGame(_ sender: Any) {
        guard let game = gameViewItem as? Game else { return }

        do {
            try ensureDirectory(AppDelegate.exportedURL)
            try FileManager.default.copyItem(at: game.directoryURL, to: AppDelegate.exportedURL.appendingPathComponent(game.directory))
        }
        catch { }
    }
    
    // MARK: - Delegates

    func textFieldDidEndEditing(_ textField: UITextField) {
        guard let game = gameViewItem as? Game else { return }
        
        if (textField == nameTextField) {
            game.name = nameTextField?.text?.trimmingCharacters(in: NSCharacterSet.whitespaces) ?? ""
        }
        else if (textField == publisherTextField) {
            game.publisher = publisherTextField?.text?.trimmingCharacters(in: NSCharacterSet.whitespaces)
        }
        else if (textField == yearTextField) {
            if let text = yearTextField?.text, text.isEmpty == false {
                game.year = Int(text)
            }
            else {
                game.year = nil
            }
        }
    }
    
    func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString string: String) -> Bool {
        if textField == yearTextField {
            return isValidInt(oldString: textField.text, range: range, replacementString: string, maximumValue: Int(Int16.max))
        }
        return true
    }
    
    func shareableImageViewChanged(view: ShareableImageView) {
        if (view == titleImageView) {
            (gameViewItem as? Game)?.titleImage = titleImageView?.image
        }
    }
    
    // MARK: - Navigation
    
    override func shouldPerformSegue(withIdentifier identifier: String, sender: Any?) -> Bool {
        guard let segueType = SegueType(rawValue: identifier) else { return true }
        
        switch segueType {
        case .showTitleImage:
            return (gameViewItem as? Game)?.titleImage != nil
            
        default:
            return true
        }

    }

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }
        
        switch segueType {
        case .start, .startTools, .startSelected, .startToolsSelected:
            guard let destination = segue.destination as? EmulatorViewController else { return }
            destination.gameViewItem = gameViewItem
            destination.toolsMode = segueType.useTools
            if segueType.useSelected, let indexPath = mediaTableView?.indexPathForSelectedRow {
                destination.selectedMedia = media.item(for: indexPath)
            }
            
        case .chooseCategory:
            guard let destination = segue.destination as? SelectCategoryTableViewController,
                let game = gameViewItem as? Game else { return }
            destination.selectedCategory = game.category
            destination.selectionCallback = { category in
                game.category = category
            }
            
        case .machineParts:
            guard let navigationController = segue.destination as? UINavigationController, let destination = navigationController.topViewController as? MachinePartsViewController, let game = gameViewItem as? Game else { return }
            destination.machineSpecification = game.machineSpecification
            destination.machine = Machine(game: game)
            destination.changeHandler = { machineSpecification in
                do {
                    try machineSpecification.save()
                }
                catch { }
            }

        case .showTitleImage:
            guard let destination = segue.destination as? FullScreenImageViewController,
                let game = gameViewItem as? Game else { return }
            destination.image = game.titleImage
            destination.navigationItem.title = game.name
        }
        
    }


    func updateItem(animated: Bool) {
        guard let item = gameViewItem else { return }
        
        machine = item.machine
        
        diskDirectoryTableViewDelegate.chargenUppercase = machine.specification.computer.chargenUppercase
        diskDirectoryTableViewDelegate.chargenLowercase = machine.specification.computer.chargenLowercase

        cancelSelectMediaButton?.isHidden = true
        
        if let game = item as? Game {
            generalView?.isHidden = false
            inboxButtons?.isHidden = true
            selectMediaButton?.isHidden = true

            if let wantedButton = game.deletedDate == nil ? deleteBarButton : recoverBarButton {
                navigationItem.rightBarButtonItems?.removeAll(where: { $0 == deleteBarButton || $0 == recoverBarButton })
                navigationItem.rightBarButtonItems?.append(wantedButton)
            }
            navigationItem.leftBarButtonItems?.removeAll()
        
            if game.favorite {
                favoriteButton?.imageView?.image = UIImage(named: "HeartFull")
            }
            else {
                favoriteButton?.imageView?.image = UIImage(named: "HeartEmpty")
            }

            game.loadTitleImage() { [unowned self] image in
                self.titleImageView?.image = image
            }
            if nameTextField?.isEditing ?? true == false {
                nameTextField?.text = game.name
            }
            if publisherTextField?.isEditing ?? true == false {
                publisherTextField?.text = game.publisher
            }
            if yearTextField?.isEditing ?? true == false {
                if let year = game.year {
                    yearTextField?.text = String(year)
                }
                else {
                    yearTextField?.text = nil
                }
            }
            categoryButton?.title = game.category?.name
            
            if game.disks?.count ?? 0 > 0 || game.tapeFile != nil || game.programFile != nil {
                toolsButton?.isHidden = false
            }
            else {
                toolsButton?.isHidden = true
            }
        }
        else {
            generalView?.isHidden = true
            inboxButtons?.isHidden = false
            inboxCreateLibraryEntryButton?.isHidden = item.type == .tools
            inboxStartButton?.isHidden = item.type == .tools
            selectMediaButton?.isHidden = item.type == .tools
            inboxCreateLibraryEntryButton?.isEnabled = false
            navigationItem.rightBarButtonItems?.removeAll(where: { $0 == deleteBarButton || $0 == recoverBarButton || $0 == machineBarButton })
        }
        
        item.loadScreenshots() { images in
            self.screenshots = images
            self.screenshotsCollectionView?.reloadData()
        }

        if media != item.media {
            media = item.media
            mediaTableView?.reloadData()
        }
    }
    
    func updateMedia(tableViewUpdates: TableViewUpdates) {
        guard let item = gameViewItem else { return }

        media = item.media
        tableViewUpdates.apply(to: mediaTableView, animation: .automatic)
    }
    
    // MARK: - Screenshots Collection View Data Source
    
    func numberOfSections(in collectionView: UICollectionView) -> Int {
        return 1
    }

    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return screenshots.count
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        let image = screenshots[indexPath.row]
        
        let height = screenshotsCollectionView?.frame.height ?? 200
        let width = image.size.width / image.size.height * height
        
        return CGSize(width: width, height: height)
    }

    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = screenshotsCollectionView!.dequeueReusableCell(withReuseIdentifier: "Screenshot", for: indexPath) as! ScreenshotCollectionViewCell
        
        cell.imageView.image = screenshots[indexPath.row]
        cell.imageView.editable = false // TODO: move to ScreenshotCollectionViewCell
        
        return cell
    }

}

// MARK: - MediaTableView Data Source

extension GameViewController: UITableViewDataSource, UITableViewDelegate {
    func numberOfSections(in tableView: UITableView) -> Int {
        let count = media.activeSections.count
        mediaTableView?.backgroundView?.isHidden = count != 0
        return count
    }

    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return media.activeSections[section].items.count
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return media.activeSections[section].title
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Media", for: indexPath) as! MediaTableViewCell
        cell.mediaView?.item = media.activeSections[indexPath.section].items[indexPath.row]
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        mediaTableViewSelectionChanged()
    }
    
    func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
        mediaTableViewSelectionChanged()
    }
    
    func mediaTableViewSelectionChanged() {
        let selectedCount = mediaTableView?.indexPathsForSelectedRows?.count ?? 0
        inboxCreateLibraryEntryButton?.isEnabled = selectedCount > 0
        inboxStartButton?.isEnabled = selectedCount <= 1
        inboxToolsButton?.isEnabled = selectedCount <= 1

        var disk: DiskImage?

        if selectedCount == 1 {
            if let indexPath = mediaTableView?.indexPathForSelectedRow {
                disk = media.item(for: indexPath) as? DiskImage
            }
        }
        diskDirectoryTableViewDelegate.disk = disk
    }
    
/*    func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        let view = tableView.dequeueReusableHeaderFooterView(withIdentifier: "Header")
        
        return view
    } */
    
    func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
        return media.activeSections[indexPath.section].supportsReorder
    }

    func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        guard sourceIndexPath.section == destinationIndexPath.section else { return }
        guard media.activeSections[sourceIndexPath.section].supportsReorder else { return }
        
        gameViewItem?.moveMedia(from: sourceIndexPath.row, to: destinationIndexPath.row, sectionType: media.activeSections[sourceIndexPath.section].type)
    }
    
    func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        do {
            try gameViewItem?.save()
        }
        catch { }
    }
    
    func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
        let deleteAction = UITableViewRowAction(style: .destructive, title: "Delete") { action, indexPath in
            guard let gameViewItem = self.gameViewItem else { return }
            
            let sectionIndex = self.media.sectionIndexFor(activeIndex: indexPath.section)
            let sectionType = self.media.sections[sectionIndex].type

            gameViewItem.removeMedia(at: indexPath.row, sectionType: sectionType)
            guard self.gameViewItem?.type != .inbox else { return } // inbox notifies of changes
            self.media = gameViewItem.media
            if self.media.sections[sectionIndex].isActive {
                tableView.deleteRows(at: [indexPath], with: .automatic)
            }
            else {
                tableView.deleteSections([indexPath.section], with: .automatic)
            }
        }
        return [ deleteAction ]
    }
    
    func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        return true
    }
}


// MARK: - Document Picker Delegate

extension GameViewController: UIDocumentPickerDelegate {
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let changes = GameViewItemMedia.Changes(gameViewController: self, destinationIndexPath: nil) else { return }
        
        for url in urls {
            changes.add(url: url)
        }
        
        changes.execute()
    }
}

// MARK: MediaTableView Drag Delegate

extension GameViewController: UITableViewDragDelegate {
    func tableView(_ tableView: UITableView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
        return dragItems(tableView: tableView, for: indexPath)
    }
    
    func tableView(_ tableView: UITableView, itemsForAddingTo session: UIDragSession, at indexPath: IndexPath, point: CGPoint) -> [UIDragItem] {
        return dragItems(tableView: tableView, for: indexPath)
    }
    
    private func dragItems(tableView: UITableView, for indexPath: IndexPath) -> [UIDragItem] {
        let mediaItem = media.activeSections[indexPath.section].items[indexPath.row]
        guard let url = mediaItem.url else { return [] }
        
        guard let mediaItemProvider = itemProvider(for: url) else { return [] }
        let item = UIDragItem(itemProvider: mediaItemProvider)
        item.localObject = tableView
        
        var items = [item]

        if let cartridge = mediaItem as? CartridgeImage, let eepromURL = cartridge.eepromUrl {
            do {
                let regex = try NSRegularExpression(pattern: "-flash(-[0-9]*)?", options: [])
                let originalName = url.deletingPathExtension().lastPathComponent
                let name = regex.stringByReplacingMatches(in: originalName, options: [], range: NSRange(0..<originalName.utf16.count), withTemplate: "")

                item.itemProvider.suggestedName = "\(name)-flash.bin"
                if let eepromItemProvider = itemProvider(for: eepromURL) {
                    eepromItemProvider.suggestedName = "\(name)-eeprom.bin"
                    items.append(UIDragItem(itemProvider: eepromItemProvider))
                    // TODO: mark to ignore when dragging into same tableview
                }
            }
            catch { }
        }
        
        return items
    }
}

extension GameViewController: UITableViewDropDelegate {
    func tableView(_ tableView: UITableView, canHandle session: UIDropSession) -> Bool {
        
        return session.hasItemsConforming(toTypeIdentifiers: Array(media.supportedTypeIdentifiers))
    }
    
    func tableView(_ tableView: UITableView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UITableViewDropProposal {
        
        let hasForeignItems = session.items.contains(where: { $0.localObject as? UITableView != tableView })
        let canInsertAtDestinationPath = dropSession(session, canInsertAt: destinationIndexPath)
        
        if hasForeignItems {
            if canInsertAtDestinationPath {
                return UITableViewDropProposal(operation: .copy, intent: .insertAtDestinationIndexPath)
            }
            else {
                return UITableViewDropProposal(operation: .copy, intent: .unspecified)
            }
        }
        else {
            if canInsertAtDestinationPath {
                return UITableViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
            }
            else {
                return UITableViewDropProposal(operation: .forbidden)
            }
        }
    }
    
    private func dropSession(_ session: UIDropSession, canInsertAt indexPath: IndexPath?) -> Bool {
        guard let indexPath = indexPath else { return false }
        
        let section = media.activeSections[indexPath.section]
        
        if !section.supportsMultiple || !section.supportsReorder {
            return false
        }

        if !session.hasItemsConforming(toTypeIdentifiers: Array(section.type.supportedTypeIdentifiers)) {
            return false
        }
        
        for otherSection in media.sections {
            guard otherSection.type != section.type else { continue }
            if session.hasItemsConforming(toTypeIdentifiers: Array(otherSection.type.supportedTypeIdentifiers)) {
                return false
            }
        }
        
        return true
    }
    
    func tableView(_ tableView: UITableView, performDropWith coordinator: UITableViewDropCoordinator) {
        guard let changes = GameViewItemMedia.Changes(gameViewController: self, destinationIndexPath: coordinator.destinationIndexPath) else { return }
        
        for item in coordinator.items {
            if let sourceIndexPath = item.sourceIndexPath {
                changes.move(from: sourceIndexPath)
            }
            else {
                changes.add(dragItem: item.dragItem)
            }
        }
        
        changes.execute()
    }
}

// MARK: - State

extension GameViewController: StateEncodable {
    override func encodeRestorableState(with coder: NSCoder) {
        super.encodeRestorableState(with: coder)
        
        guard let game = gameViewItem as? Game else { return }
        encode(managedObject: game, forKey: .objectID, inCoder: coder)
    }

    override func decodeRestorableState(with coder: NSCoder) {
        super.decodeRestorableState(with: coder)

        guard coder.containsValue(forKey: StateKey.objectID.rawValue) else { return }
        if let restoredGame = decodeManagedObject(forKey: .objectID, fromCoder: coder) as? Game {
            gameViewItem = restoredGame
        }
        else {
            dismiss(animated: false)
        }
    }
}
