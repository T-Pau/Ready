/*
 GeneralSettingsViewController.swift
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
import Emulator

class GeneralSettingsViewController: UIViewController {
    enum ReuseIdentifier: String {
        case Enum
        case Switch
     }
    
    struct Row {
        var name: String
        var reuseIdentifier: ReuseIdentifier
        var defaultsKey: Defaults.Key?
    }
    
    struct Section {
        var title: String?
        var rows: [Row]
    }
    
    let sections = [
        Section(title: nil, rows: [
            Row(name: "Border Mode", reuseIdentifier: .Enum, defaultsKey: nil),
            Row(name: "Video Scaling", reuseIdentifier: .Enum, defaultsKey: .VideoFilter),
            Row(name: "Emulate Drive Sounds", reuseIdentifier: .Switch, defaultsKey: .EmulateDriveSounds),
            Row(name: "Enable JiffyDOS", reuseIdentifier: .Switch, defaultsKey: .EnableJiffyDos),
            Row(name: "Use Caps Lock as Commodore Key", reuseIdentifier: .Switch, defaultsKey: .CapsLockAsCommodore)
        ])
    ]
    
    struct BorderMode: Equatable, CustomStringConvertible {
        var name: String { return value.label }
        var value: MachineConfigOld.BorderMode
        
        var description: String {
            return name
        }
    }
    
    let borderModes = [
        BorderMode(value: .auto),
        BorderMode(value: .show),
        BorderMode(value: .hide)
    ]
    
    let videoFilters = [
        "None",
        "Linear"
    ]
    
    var machineSpecification: MachineSpecification = Defaults.standard.machineSpecification

    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destination.
        // Pass the selected object to the new view controller.
    }
    
    @IBAction func done(_ sender: Any) {
        dismiss(animated: true)
    }
}

extension GeneralSettingsViewController: UITableViewDelegate, UITableViewDataSource {
    func numberOfSections(in tableView: UITableView) -> Int {
        return sections.count
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return sections[section].rows.count
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        // nil results in no visual separation of the groups
        return sections[section].title
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let row = rowFor(indexPath: indexPath)
        let cell = tableView.dequeueReusableCell(withIdentifier: row.reuseIdentifier.rawValue, for: indexPath)
        
        switch row.reuseIdentifier {
        case .Enum:
            cell.textLabel?.text = rowFor(indexPath: indexPath).name
            cell.detailTextLabel?.text = currentChoiceFor(row: row)

        case .Switch:
            guard let cell = cell as? SwitchTableViewCell else { break }
            cell.titleLabel?.text = rowFor(indexPath: indexPath).name
            guard let key = row.defaultsKey else { break }
            cell.switcher?.isOn = Defaults.standard.boolValue(for: key)
            cell.changeHandler = { value in
                Defaults.standard.set(value: value, for: key)
            }
        }
        return cell
    }

    func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
        let row = rowFor(indexPath: indexPath)
        
        switch row.reuseIdentifier {
        case .Enum:
            return indexPath
            
        case .Switch:
            return nil
        }
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let row = rowFor(indexPath: indexPath)

        switch row.reuseIdentifier {
        case .Enum:
            switch row.defaultsKey {
            case nil:
                presentEnumChooser(title: "Border Mode", choices: borderModes, selectedRow: currentChoiceIndexFor(row: row)) { _, choice in
                    guard let choice = choice as? BorderMode else { return }
                    self.machineSpecification.borderMode = choice.value
                    do {
                        try self.machineSpecification.save()
                    }
                    catch { }
                    tableView.reloadRows(at: [indexPath], with: .automatic)
                }
                
            case .VideoFilter?:
                presentEnumChooser(title: "Video Filter", choices: videoFilters, selectedRow: currentChoiceIndexFor(row: row)) { _, choice in
                    guard let choice = choice as? String else { return }
                    Defaults.standard.videoFilter = choice
                    tableView.reloadRows(at: [indexPath], with: .automatic)
                }
                
            default:
                break
            }
            tableView.deselectRow(at: indexPath, animated: true)

        case .Switch:
            break
        }
    }
    
    private func rowFor(indexPath: IndexPath) -> Row {
        return sections[indexPath.section].rows[indexPath.row]
    }
    
    private func currentChoiceIndexFor(row: Row) -> Int {
        switch row.defaultsKey {
        case nil:
            let currentValue = machineSpecification.borderMode
            return borderModes.firstIndex(where: { $0.value == currentValue }) ?? 0
            
        case .VideoFilter?:
            let currentValue = Defaults.standard.videoFilter
            return videoFilters.firstIndex(where: { $0 == currentValue }) ?? 0

        default:
            return 0
        }
    }
    
    private func currentChoiceFor(row: Row) -> String? {
        switch row.defaultsKey {
        case nil:
            return borderModes[currentChoiceIndexFor(row: row)].name
        case .VideoFilter?:
            return videoFilters[currentChoiceIndexFor(row: row)]

        default:
            return nil
        }
    }
    
    private func presentEnumChooser(title: String, choices: [Any], selectedRow: Int, changeHandler: @escaping ((Int, Any) -> ())) {
        guard let controller = UIStoryboard(name: "Main", bundle: nil).instantiateViewController(withIdentifier: "EnumChooser") as? EnumChooserViewController else { return }
        
        controller.title = title
        controller.choices = choices
        controller.selectedRow = selectedRow
        controller.changeHandler = changeHandler
        
        navigationController?.pushViewController(controller, animated: true)
    }
}
