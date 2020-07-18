/*
 SelectCategoryTableViewController.swift
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
    
    
    override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
        return UISwipeActionsConfiguration(actions: [
            UIContextualAction(style: .destructive, title: "Delete", handler: { action, view, completionHandler in
                guard let context = (UIApplication.shared.delegate as? AppDelegate)?.persistentContainer.viewContext else {
                    completionHandler(false)
                    return
                }
                
                let index = indexPath.row - 2
                context.delete(self.categories[index])
                self.categories.remove(at: index)
                tableView.deleteRows(at: [indexPath], with: .automatic)
                completionHandler(true)
            })
        ])
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
