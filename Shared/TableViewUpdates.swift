/*
 TableViewUpdates.swift -- collect and apply updates to UITableView data source
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

struct TableViewUpdates {
    var insertedSections = IndexSet()
    var deletedSections = IndexSet()
    var changedSections = IndexSet()
    var movedSections = [Int : Int]()

    var insertedRows = [IndexPath]()
    var deletedRows = [IndexPath]()
    var changedRows = [IndexPath]()
    var movedRows = [IndexPath : IndexPath]()
    
    var isEmpty: Bool {
        return insertedSections.isEmpty && deletedSections.isEmpty && changedSections.isEmpty && movedRows.isEmpty && insertedRows.isEmpty && deletedRows.isEmpty && changedRows.isEmpty && movedRows.isEmpty
    }
    
    mutating func insert(section: Int) {
        insertedSections.insert(section)
    }
    
    mutating func delete(section: Int) {
        deletedSections.insert(section)
    }
    
    mutating func reload(section: Int) {
        changedSections.insert(section)
    }
    
    mutating func move(section: Int, to newSection: Int) {
        movedSections[section] = newSection
    }
    
    mutating func insert(row: IndexPath) {
        insertedRows.append(row)
    }
    
    mutating func delete(row: IndexPath) {
        deletedRows.append(row)
    }
    
    mutating func reload(row: IndexPath) {
        changedRows.append(row)
    }
    
    mutating func move(row: IndexPath, to newRow: IndexPath) {
        movedRows[row] = newRow
    }
    
    func apply(to tableView: UITableView?, animation: UITableView.RowAnimation) {
        guard let tableView = tableView else { return }

        tableView.beginUpdates()

        tableView.insertSections(insertedSections, with: animation)
        tableView.deleteSections(deletedSections, with: animation)
        tableView.reloadSections(changedSections, with: animation)
        for (source, destination) in movedSections {
            tableView.moveSection(source, toSection: destination)
        }

        tableView.insertRows(at: insertedRows, with: animation)
        tableView.deleteRows(at: deletedRows, with: animation)
        tableView.reloadRows(at: changedRows, with: animation)
        for (source, destination) in movedRows {
            tableView.moveRow(at: source, to: destination)
        }

        tableView.endUpdates()
    }
    
    mutating func clear() {
        insertedSections.removeAll()
        deletedSections.removeAll()
        changedSections.removeAll()
        movedSections.removeAll()

        insertedRows.removeAll()
        deletedRows.removeAll()
        changedRows.removeAll()
        movedRows.removeAll()
    }
}
