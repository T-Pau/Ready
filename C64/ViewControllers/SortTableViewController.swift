/*
 SortTableViewController.swift -- Sort Criteria
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
import CoreData
import Emulator

class SortTableViewController: UITableViewController {
    var changedCallback: ((_ animated: Bool) -> ())?
    
    var mode = GameCollectionViewController.Mode.all
    
    private var categories = [CategoryEntity]()

    private enum Section: Int, CaseIterable {
        case criterium = 0
        case grouping = 1
        case categories = 2
        
        func reuseIdentifier(for row: Int) -> String {
            switch self {
            case .criterium:
                return "Basic"
            case .grouping:
                return "Switch"
                
            case .categories:
                if let extraRow = CategoriesExtraRow(rawValue: row) {
                    return extraRow.reuseIdentifier
                }
                else {
                    return "Basic"
                }
            }
        }
        
        var headerTitle: String? {
            switch self {
            case .categories:
                return "Show Categories"
            case .criterium:
                return "Sort By"
            case .grouping:
                return nil
            }
        }
    }
    
    private enum CategoriesExtraRow: Int, CaseIterable {
        case showHideAll = 0
        case none = 1

        var reuseIdentifier: String {
            switch self {
            case .showHideAll:
                return "Button"
                
            case .none:
                return "Basic"
            }
        }
    }
    
    private struct SortCriterium {
        var name: String
        var value: Defaults.SortCriterium
    }
    
    private var sortCriteria = [
        SortCriterium(name: "Name", value: .name),
        SortCriterium(name: "Year", value: .year),
        SortCriterium(name: "Publisher", value: .publisher),
        SortCriterium(name: "Publisher, Year", value: .publisherYear),
        SortCriterium(name: "Recently Played", value: .lastOpened),
        SortCriterium(name: "Recently Added", value: .added)
    ]
    
    override func viewDidLoad() {
        super.viewDidLoad()

        tableView.addObserver(self, forKeyPath: "contentSize", options: .new, context: nil)
        
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        guard let entityName = CategoryEntity.entity().name else { return }
        
        
        let fetchRequest = NSFetchRequest<CategoryEntity>(entityName: entityName)
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "name", ascending: true)]
        
        do {
            categories = try context.fetch(fetchRequest)
        }
        catch { }
    }
    
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        tableView.removeObserver(self, forKeyPath: "contentSize")
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey : Any]?, context: UnsafeMutableRawPointer?) {
        if keyPath == "contentSize" {
            preferredContentSize = tableView.contentSize
        }
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return Section.allCases.count
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection rawSection: Int) -> Int {
        guard let section = Section(rawValue: rawSection) else { return 0 }
        switch section {
        case .categories:
            return categories.count + CategoriesExtraRow.allCases.count
        case .criterium:
            return sortCriteria.count
        case .grouping:
            return 1
        }
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection sectionIndex: Int) -> String? {
        guard let section = Section(rawValue: sectionIndex) else { return nil }
        return section.headerTitle
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let section = Section(rawValue: indexPath.section) ?? .criterium
        
        let cell = tableView.dequeueReusableCell(withIdentifier: section.reuseIdentifier(for: indexPath.row), for: indexPath)
        
        switch section {
        case .categories:
            if let extraRow = CategoriesExtraRow(rawValue: indexPath.row) {
                switch extraRow {
                case .none:
                    cell.textLabel?.text = "None"
                    cell.accessoryType = Defaults.standard.showUncategorized ? .checkmark : .none
                
                case .showHideAll:
                    cell.textLabel?.textColor = cell.tintColor
                    if areAllCategoriesVisible() {
                        cell.textLabel?.text = "Hide All Categories"
                    }
                    else {
                        cell.textLabel?.text = "Show All Categories"
                    }
                }
            }
            else {
                let category = categories[indexPath.row - CategoriesExtraRow.allCases.count]
                cell.textLabel?.text = category.name
                cell.accessoryType = category.visible ? .checkmark : .none
            }
            
        case .criterium:
            let criterium = sortCriteria[indexPath.row]
            let sortCriterium = mode == .all ? Defaults.standard.sortCriterium : Defaults.standard.sortCriteriumFavorites
            cell.textLabel?.text = criterium.name
            cell.accessoryType = criterium.value == sortCriterium ? .checkmark : .none
            
        case .grouping:
            guard let sell = cell as? SwitchTableViewCell else { return cell }
            
            sell.textLabel?.text = "Group Games"
            sell.switcher?.isOn = mode == .all ? Defaults.standard.groupGames : Defaults.standard.groupFavorites
        }
        
        return cell
    }
    
    override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
        guard let section = Section(rawValue: indexPath.section) else { return nil }

        switch section {
        case .categories, .criterium:
            return indexPath
            
        case .grouping:
            return nil
        }
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let section = Section(rawValue: indexPath.section) else { return }

        var animated = false

        switch section {
        case .categories:
            if let extraRow = CategoriesExtraRow(rawValue: indexPath.row) {
                switch extraRow {
                case .none:
                    Defaults.standard.showUncategorized = !Defaults.standard.showUncategorized
                case .showHideAll:
                    let visible = !areAllCategoriesVisible()
                    Defaults.standard.showUncategorized = visible
                    if !categories.isEmpty {
                        for category in categories {
                            category.visible = visible
                        }
                        do {
                            try categories[0].managedObjectContext?.save()
                        }
                        catch { }
                    }

                    tableView.reloadSections([section.rawValue], with: .automatic)
                }
            }
            else {
                let category = categories[indexPath.row - CategoriesExtraRow.allCases.count]
                category.visible = !category.visible
                do {
                    try category.managedObjectContext?.save()
                }
                catch { }
                tableView.reloadRows(at: [IndexPath(row: 0, section: section.rawValue)], with: .automatic)
            }
            tableView.reloadRows(at: [indexPath], with: .automatic)
            animated = true
            break
            
        case .criterium:
            let sortCriterium = sortCriteria[indexPath.row].value
            if mode == .all {
                Defaults.standard.sortCriterium = sortCriterium
            }
            else {
                Defaults.standard.sortCriteriumFavorites = sortCriterium
            }
            tableView.cellForRow(at: indexPath)?.accessoryType = .checkmark
            for i in 0..<sortCriteria.count {
                if i != indexPath.row {
                    tableView.cellForRow(at: IndexPath(row: i, section: indexPath.section))?.accessoryType = .none
                }
            }

        case .grouping:
            return
        }
        
        changedCallback?(animated)
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    @IBAction func groupedChanged(_ sender: Any) {
        guard let switcher = sender as? UISwitch else { return }
        let value = switcher.isOn
        if mode == .all {
            Defaults.standard.groupGames = value
        }
        else {
            Defaults.standard.groupFavorites = value
        }
        changedCallback?(false)
    }
    
    func areAllCategoriesVisible() -> Bool {
        if !Defaults.standard.showUncategorized {
            return false
        }
        
        return !categories.contains(where: { !$0.visible })
    }
}
