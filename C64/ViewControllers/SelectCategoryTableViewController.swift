/*
 SelectCategoryTableViewController.swift
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
import CoreData

class SelectCategoryTableViewController: UITableViewController {
    typealias SelectionCallback = (_ category: CategoryEntity?) -> Void
    var selectedCategory: CategoryEntity?
    var selectionCallback: SelectionCallback?
    
    private var categories = [CategoryEntity]()

    override func viewDidLoad() {
        super.viewDidLoad()
        
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        guard let entityName = CategoryEntity.entity().name else { return }

        let fetchRequest = NSFetchRequest<CategoryEntity>(entityName: entityName)
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "name", ascending: true)]
        
        do {
            categories = try context.fetch(fetchRequest)
        }
        catch { }
    }

    // MARK: - Table view data source

    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return categories.count + 2
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Row", for: indexPath)

        cell.accessoryType = .none
        
        switch indexPath.row {
        case 0:
            cell.textLabel?.text = "Create New Category"
            break

        case 1:
            cell.textLabel?.text = "None"
            if selectedCategory == nil {
                cell.accessoryType = .checkmark
            }
            break
            
        default:
            let category = categories[indexPath.row - 2]
            
            cell.textLabel?.text = category.name
            
            if category == selectedCategory {
                cell.accessoryType = .checkmark
            }
        }

        return cell
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let shouldDismiss: Bool
        switch indexPath.row {
        case 0:
            tableView.deselectRow(at: indexPath, animated: true)
            presentCreateNewCategory()
            shouldDismiss = false
            break
            
        case 1:
            selectionCallback?(nil)
            shouldDismiss = true
            
        default:
            selectionCallback?(categories[indexPath.row - 2])
            shouldDismiss = true
        }
        
        if shouldDismiss {
            dismiss(animated: true)
        }
    }
    
    override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }

        do {
            try context.save()
        }
        catch { }
    }
    
    override func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
        let deleteAction = UITableViewRowAction(style: .destructive, title: "Delete") { action, indexPath in
            guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
            
            let index = indexPath.row - 2
            context.delete(self.categories[index])
            self.categories.remove(at: index)
            tableView.deleteRows(at: [indexPath], with: .automatic)
        }
        return [ deleteAction ]

    }
    
    private func presentCreateNewCategory() {
        guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else { return }
        let alertController = UIAlertController(title: "Create New Category", message: nil, preferredStyle: .alert)
        alertController.addTextField(configurationHandler: { textField in
            textField.placeholder = "Category"
            textField.autocapitalizationType = .words
            textField.autocorrectionType = .default
            textField.font = UIFont.systemFont(ofSize: 17)
        })
        guard let textField = alertController.textFields?[0] else { return }
        alertController.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
        alertController.addAction(UIAlertAction(title: "OK", style: .default, handler: { _ in
            guard let name = textField.text?.trimmingCharacters(in: .whitespaces), !name.isEmpty else { return }
            let category = CategoryEntity(name: name, insertInto: context)
            self.selectionCallback?(category)
            self.dismiss(animated: true)
        }))
        present(alertController, animated: true)
    }
}
