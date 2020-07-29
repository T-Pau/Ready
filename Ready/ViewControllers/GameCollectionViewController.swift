/*
 GameCollectionViewController.swift -- List of Games
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
import UIKit
import C64UIComponents
import Emulator

private let reuseIdentifier = "Game"

class GameCollectionViewController: UIViewController, UICollectionViewDataSource, SeguePreparer {
    enum SegueType: String {
        case chooseSort
        case openGame
        case showSettings
    }
    
    enum Mode: String {
        case all
        case favorites
        case deleted
    }
    
    @IBOutlet weak var stackView: UIStackView!
    @IBOutlet weak var paddingView: UIView!
    @IBOutlet weak var paddingViewHeight: NSLayoutConstraint!
    @IBOutlet weak var searchBar: UISearchBar!
    @IBOutlet weak var collectionView: UICollectionView!
    @IBOutlet weak var sortButton: UIBarButtonItem!
    @IBOutlet weak var addButton: UIBarButtonItem!
    
    var mode = Mode.all
    
    private var searchTerm: String? {
        didSet {
            if oldValue != searchTerm {
                updateSorting(animated: true)
            }
        }
    }
    
    private var showPublisher = true
    private var showYear = true
    
    var resultController: NSFetchedResultsController<Game>?
    var changesDelegate: CollectionViewFetchedResultControllerDelegate?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        if let navigationBar = navigationController?.navigationBar {
            paddingViewHeight.constant = navigationBar.frame.maxY + 24 // TODO: fix correctly
        }
        
        let backgroundView = PlaceholderView(frame: CGRect(x: 0, y: 0, width: 0, height: 0))
        backgroundView.color = UIColor(named: "EmptyCollection") ?? UIColor.darkGray
        switch mode {
        case .all, .favorites:
            backgroundView.text = "Use + or the Inbox\nto create entries\nfor your games and demos."
        case .deleted:
            backgroundView.frameWidth = 0
            backgroundView.text = "No recently deleted games."
        }
        collectionView.backgroundView = backgroundView
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        guard let collectionView = collectionView else { return }
        changesDelegate = CollectionViewFetchedResultControllerDelegate(collectionView: collectionView, fetchedResultsController: nil)
        
        if mode == .deleted {
            navigationItem.rightBarButtonItems?.removeAll(where: { $0 == sortButton || $0 == addButton })
        }
        
        updateSorting(animated: false)
    }
    
    override var keyCommands: [UIKeyCommand]? {
        var commands = [
            UIKeyCommand(title: "Search", action: #selector(showSearch(_:)), input: "f", modifierFlags: .command, discoverabilityTitle: "Search")
        ]
        
        if mode != .deleted {
            commands.append(UIKeyCommand(title: "Add New Title", action: #selector(addGame(_:)), input: "n", modifierFlags: .command, discoverabilityTitle: "Add New Title"))
        }
        
        return commands
    }
    
    // MARK: - Actions
    
    @IBAction func showSearch(_ sender: Any) {
        if searchBar.isHidden {
            toggleSearch(sender)
        }
        else {
            searchBar.becomeFirstResponder()
        }
    }

    @IBAction func toggleSearch(_ sender: Any) {
        let isHidden = !searchBar.isHidden
        UIView.animate(withDuration: 0.3, animations: {
            if isHidden {
                self.searchBar.resignFirstResponder()
                self.searchTerm = nil
            }
            else {
                self.searchBar.becomeFirstResponder()
            }
            self.searchBar.isHidden = isHidden
            self.paddingView.isHidden = isHidden
            self.stackView.layoutIfNeeded()
        })
    }
    
    @IBAction func addGame(_ sender: Any) {
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        let alertController = UIAlertController(title: "Add New Title", message: nil, preferredStyle: .alert)
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
                let game = Game(name: name, insertInto: context)
                try context.save()
                self.performSegue(withIdentifier: SegueType.openGame.rawValue, sender: game)
            }
            catch { }
        }))
        present(alertController, animated: true)
    }
    
    // MARK: - Navigation

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        guard let segueType = segueType(from: segue) else { return }
        switch segueType {
        case .openGame:
            guard let destination = segue.destination as? GameViewController else { return }
            
            if let game = sender as? Game {
                destination.gameViewItem = game
            }
            else {
                guard let indexPath = collectionView.indexPathsForSelectedItems?.first else { return }
                destination.gameViewItem = resultController?.object(at: indexPath)
            }

        case .chooseSort:
            guard let destination = segue.destination as? SortTableViewController else { return }
            destination.mode = mode
            destination.changedCallback = { [weak self] animated in
                self?.updateSorting(animated: animated)
            }
            
        case .showSettings:
            break
        }
    }

    // MARK: UICollectionViewDataSource

    func numberOfSections(in collectionView: UICollectionView) -> Int {

        collectionView.backgroundView?.isHidden = resultController?.sections?.contains(where: { $0.numberOfObjects > 0 }) ?? false == true
        return resultController?.sections?.count ?? 0
    }


    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return resultController?.sections?[section].numberOfObjects ?? 0
    }

    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        guard let cell = collectionView.dequeueReusableCell(withReuseIdentifier: reuseIdentifier, for: indexPath) as? GameCollectionViewCell else { fatalError("wrong type of cell dequeued") }
    
        if let game = resultController?.object(at: indexPath) {
            cell.titleImage.layer.magnificationFilter = .nearest
            cell.name.text = game.name
            var detail = [String]()
            if mode == .deleted {
                if let deletedDate = game.deletedDate {
                    let age = min(Int(deletedDate.timeIntervalSinceNow / (24 * 60 * 60)) * -1, 30)
                    detail.append("\(30 - age) days left") // TODO: don't hardcode
                }
            }
            else {
                if showPublisher, let publisher = game.publisher {
                    detail.append(publisher)
                }
                if showYear, let year = game.year {
                    detail.append("\(year)")
                }
            }
            cell.detail.text = detail.joined(separator: ", ")
            game.loadTitleImage() {
                cell.titleImage.image = $0
            }
        }
        
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView, viewForSupplementaryElementOfKind kind: String, at indexPath: IndexPath) -> UICollectionReusableView {
        let view = collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "Header", for: indexPath)
        
        if kind == UICollectionView.elementKindSectionHeader {
            if let headerView = view as? GameHeaderCollectionViewCell {
                headerView.title.text = resultController?.sections?[indexPath.section].name
            }
            if !grouped {
                view.isHidden = true
            }
        }
        
        return view
    }
    
    var grouped = false

    func updateSorting(animated: Bool) {
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        guard let entityName = Game.entity().name else { return }
        
        var sortDescriptors = [NSSortDescriptor]()
        var sectionNameKeyPath: String?
        var subPredicates = [NSPredicate]()
        
        if mode == .deleted {
            sortDescriptors = [NSSortDescriptor(key: "deletedDate", ascending: false)]
        }
        else {
            showPublisher = true
            showYear = true
        
            switch mode == .all ? Defaults.standard.sortCriterium : Defaults.standard.sortCriteriumFavorites {
            case .added:
                sortDescriptors.append(NSSortDescriptor(key: "added", ascending: false))
                sectionNameKeyPath = "addedSection"
            case .lastOpened:
                sortDescriptors.append(NSSortDescriptor(key: "lastOpened", ascending: false))
                sectionNameKeyPath = "lastOpenedSection"
            case .name:
                // name is always added as last arbiter
                sectionNameKeyPath = "nameSection"
            case .publisher:
                sortDescriptors.append(NSSortDescriptor(key: "publisher", ascending: true))
                sectionNameKeyPath = "publisherSection"
                showPublisher = false
            case .publisherYear:
                sortDescriptors.append(NSSortDescriptor(key: "publisher", ascending: true))
                sortDescriptors.append(NSSortDescriptor(key: "yearRaw", ascending: true))
                sectionNameKeyPath = "publisherSection"
                showPublisher = false
            case .year:
                sortDescriptors.append(NSSortDescriptor(key: "yearRaw", ascending: true))
                sectionNameKeyPath = "yearSection"
                showYear = false
            }
            
            sortDescriptors.append(NSSortDescriptor(key: "name", ascending: true))
            
            grouped = mode == .all ? Defaults.standard.groupGames : Defaults.standard.groupFavorites
            
            if !grouped {
                sectionNameKeyPath = nil
                showPublisher = true
                showYear = true
            }
            
            var categoryPredicate = NSPredicate(format: "category.visible == true")
            if Defaults.standard.showUncategorized {
                categoryPredicate = NSCompoundPredicate(orPredicateWithSubpredicates: [NSPredicate(format: "category == nil"), categoryPredicate])
            }
            subPredicates.append(categoryPredicate)
        }
        
        let tabPredicate: NSPredicate
        switch mode {
        case .all:
            tabPredicate = NSPredicate(format: "deletedDate == nil")
        case .favorites:
            tabPredicate = NSPredicate(format: "deletedDate == nil and favorite == true")
        case .deleted:
            tabPredicate = NSPredicate(format: "deletedDate != nil")
        }
        subPredicates.append(tabPredicate)

        if let searchTerm = searchTerm?.trimmingCharacters(in: .whitespaces), !searchTerm.isEmpty {
            let words = searchTerm.components(separatedBy: .whitespaces)
            if !words.isEmpty {
                let wordPredicates: [NSPredicate] = words.map({ word in
                    return NSPredicate(format: "name contains[cd] %@", word)
                })

                subPredicates.append(contentsOf: wordPredicates)
            }
        }
        
        let fetchRequest = NSFetchRequest<Game>(entityName: entityName)
        fetchRequest.sortDescriptors = sortDescriptors

        fetchRequest.predicate = NSCompoundPredicate(andPredicateWithSubpredicates: subPredicates)
        resultController = NSFetchedResultsController<Game>(fetchRequest: fetchRequest, managedObjectContext: context, sectionNameKeyPath: sectionNameKeyPath, cacheName: nil)
        resultController?.delegate = changesDelegate
        do {
            try resultController?.performFetch()
        }
        catch {
            
        }
        
        (collectionView.collectionViewLayout as? UICollectionViewFlowLayout)?.sectionHeadersPinToVisibleBounds = grouped
        
        changesDelegate?.setFetchedResultsController(resultController as? NSFetchedResultsController<NSFetchRequestResult>, animated: animated)
    }
}

extension GameCollectionViewController: UIDataSourceModelAssociation {
    func modelIdentifierForElement(at indexPath: IndexPath, in view: UIView) -> String? {
        guard let game = resultController?.object(at: indexPath) else { return nil }
        return game.objectID.uriRepresentation().absoluteString
    }
    
    func indexPathForElement(withModelIdentifier identifier: String, in view: UIView) -> IndexPath? {
        guard let appDelegate = UIApplication.shared.delegate as? AppDelegate,
            let url = URL(string: identifier),
            let objectID = appDelegate.persistentContainer.persistentStoreCoordinator.managedObjectID(forURIRepresentation: url),
        let index = resultController?.fetchedObjects?.firstIndex(where: { $0.objectID == objectID }) else { return nil }
        return IndexPath(row: index, section: 0)
    }
}

extension GameCollectionViewController: UISearchBarDelegate {
    func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
        searchTerm = searchText
    }

    func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
        toggleSearch(searchBar)
    }
}
