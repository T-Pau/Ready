/*
 CollectionViewFetchedResultControllerDelegate.swift -- forward changes from NSFetchedResultsController to UICollectionView
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

class CollectionViewFetchedResultControllerDelegate: NSObject, NSFetchedResultsControllerDelegate {
    var collectionView: UICollectionView
    var fetchedResultsController: NSFetchedResultsController<NSFetchRequestResult>? {
        get { return resultsControler }
        set { setFetchedResultsController(newValue, animated: false) }
    }
    
    var resultsControler: NSFetchedResultsController<NSFetchRequestResult>?
    
    init(collectionView: UICollectionView, fetchedResultsController: NSFetchedResultsController<NSFetchRequestResult>?) {
        self.collectionView = collectionView
        self.resultsControler = fetchedResultsController
    }
    
    private var addedSections = IndexSet()
    private var deletedSections = IndexSet()
    private var addedItems: [IndexPath] = []
    private var changedItems: [IndexPath] = []
    private var deletedItems: [IndexPath] = []
    
    func setFetchedResultsController(_ newFetchedResultsController: NSFetchedResultsController<NSFetchRequestResult>?, animated: Bool) {
        let oldController = resultsControler
        
        do {
            try newFetchedResultsController?.performFetch()
        }
        catch {
            return
        }
        newFetchedResultsController?.delegate = self
        
        resultsControler = newFetchedResultsController

        if animated {
            forwardChanges(oldController, newFetchedResultsController)
        }
        else {
            collectionView.reloadData()
        }
    }
    
    func forwardChanges(_ oldController: NSFetchedResultsController<NSFetchRequestResult>?, _ newController: NSFetchedResultsController<NSFetchRequestResult>?) {
        // This currently assumes that:
        //  - objects inherit from NSObject
        //  - objects remain in the same order
        //  - sections have unique names
        
        clearChanges()
        
        if let newController = newController {
            if let oldController = oldController {
                computeChanges(oldController, newController)
            }
            else {
                for sectionIndex in (0 ..< (newController.sections?.count ?? 0)) {
                    addedSections.insert(sectionIndex)
                }
            }
        }
        else {
            if let oldController = oldController {
                for sectionIndex in (0 ..< (oldController.sections?.count ?? 0)) {
                    deletedSections.insert(sectionIndex)
                }
            }
            else {
                return
            }
        }
        
        commitChanges()
    }
    
    func computeChanges(_ oldController: NSFetchedResultsController<NSFetchRequestResult>,  _ newController: NSFetchedResultsController<NSFetchRequestResult>) {
        guard let oldSections = oldController.sections,
            let newSections = newController.sections else { return }

        var oldIndex = 0
        var newIndex = 0
        
        while oldIndex < oldSections.count && newIndex < newSections.count {
            let oldSection = oldSections[oldIndex]
            let newSection = newSections[newIndex]
            
            if oldSection.name == newSection.name {
                compareSections(oldSection: oldSection, at: oldIndex, newSection: newSection, at: newIndex)
                oldIndex += 1
                newIndex += 1
            }
            else if oldSection.name < newSection.name {
                deletedSections.insert(oldIndex)
                oldIndex += 1
            }
            else {
                addedSections.insert(newIndex)
                newIndex += 1
            }
        }
        for index in (oldIndex ..< oldSections.count) {
            deletedSections.insert(index)
        }
        for index in (newIndex ..< newSections.count) {
            addedSections.insert(index)
        }
    }
    
    private func compareSections(oldSection: NSFetchedResultsSectionInfo, at oldSectionIndex: Int, newSection: NSFetchedResultsSectionInfo, at newSectionIndex: Int) {
        guard let oldObjects = oldSection.objects as? [NSObject], let newObjects = newSection.objects as? [NSObject] else { return }
        
        var oldObjectMap = [NSObject : Int]()
        
        for (index, object) in oldObjects.enumerated() {
            oldObjectMap[object] = index
        }

        for (newIndex, object) in newObjects.enumerated() {
            if let oldIndex = oldObjectMap[object] {
                if oldIndex != newIndex {
                    deletedItems.append(IndexPath(row: oldIndex, section: oldSectionIndex))
                    addedItems.append(IndexPath(row: newIndex, section: newSectionIndex))
                }
                oldObjectMap.removeValue(forKey: object)
            }
            else {
                addedItems.append(IndexPath(row: newIndex, section: newSectionIndex))
            }
        }
        
        for index in oldObjectMap.values {
            deletedItems.append(IndexPath(row: index, section: oldSectionIndex))
        }
    }
    
    // MARK: - NSFetchedResultsController Delegate
    func clearChanges() {
        addedSections.removeAll()
        deletedSections.removeAll()
        addedItems.removeAll()
        changedItems.removeAll()
        deletedItems.removeAll()
    }
    
    func commitChanges() {
        if collectionView.window == nil {
            collectionView.reloadData()
            clearChanges()
        }
        else {
            collectionView.performBatchUpdates({
                collectionView.deleteSections(self.deletedSections)
                collectionView.insertSections(self.addedSections)
                collectionView.deleteItems(at: self.deletedItems)
                collectionView.insertItems(at: self.addedItems)
                collectionView.reloadItems(at: self.changedItems)
            }, completion: { (finished: Bool) in
                self.clearChanges()
            })
        }
    }
        
    func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        clearChanges()
    }
    
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange sectionInfo: NSFetchedResultsSectionInfo, atSectionIndex sectionIndex: Int, for type: NSFetchedResultsChangeType) {
        switch type {
        case .delete:
            deletedSections.insert(sectionIndex)
        case .insert:
            addedSections.insert(sectionIndex)
        case .move:
            // nothing to do?
            break
        case .update:
            // handled per item
            break
        @unknown default:
            break
        }
    }
    
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        switch type {
        case .delete:
            if let ip = indexPath {
                if !deletedSections.contains(ip.section) {
                    deletedItems.append(ip)
                }
            }
        case .insert:
            if let ip = newIndexPath {
                if !addedSections.contains(ip.section) {
                    addedItems.append(ip)
                }
            }
        case .move:
            if let ip = indexPath {
                if !deletedSections.contains(ip.section) {
                    deletedItems.append(ip)
                }
            }
            if let ip = newIndexPath {
                if !addedSections.contains(ip.section) {
                    addedItems.append(ip)
                }
            }
        case .update:
            if let ip = indexPath {
                if !deletedSections.contains(ip.section) && !deletedItems.contains(ip) {
                    changedItems.append(ip)
                }
            }
        @unknown default:
            break
        }
    }
    
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        commitChanges()
    }

}
