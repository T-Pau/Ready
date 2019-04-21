/*
 Copyright (C) 2019 Dieter Baron
 
 The author can be contacted at <dillo@nih.at>
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
 
 3. The names of the authors may not be used to endorse or promote
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


import Foundation

func isValidInt(oldString: String?, range: NSRange, replacementString string: String, maximumValue: Int? = nil) -> Bool {
    if string.rangeOfCharacter(from: NSCharacterSet.decimalDigits.inverted) != nil {
        return false
    }
    
    let newString = ((oldString ?? "") as NSString).replacingCharacters(in: range, with: string)
    
    if newString.isEmpty {
        return true
    }
    
    guard let value = Int(newString) else { return false }
    
    guard let maximum = maximumValue else { return true }
    
    return value < maximum
}

func typeName(some: Any) -> String {
    return (some is Any.Type) ? "\(some)" : "\(type(of: some))"
}

func uniqeName(directory: URL, name: String?, pathExtension: String, create: Bool = false) throws -> URL {
    let fileManager = FileManager.default
    let basename = (name as NSString?)?.deletingPathExtension ?? "unknown"
    let suffix = pathExtension.isEmpty ? "" : ".\(pathExtension)"
    var i = 0
    if create {
        try ensureDirectory(directory)
    }
    while (true) {
        let fileUrl = directory.appendingPathComponent(basename + (i > 0 ? " \(i)" : "") + suffix)
        if !fileManager.fileExists(atPath: fileUrl.path) {
            if create {
                do {
                    try Data().write(to: fileUrl)
                }
                catch { continue }
            }
            return fileUrl
        }

        i += 1
        continue
    }
}

func ensureDirectory(_ directory: URL) throws {
    let fileManager = FileManager.default
    
    guard !fileManager.fileExists(atPath: directory.path) else { return }

    try fileManager.createDirectory(at: directory, withIntermediateDirectories: true, attributes: [:])
}

func itemProvider(for url: URL) -> NSItemProvider? {
    guard let fileType = C64FileType(pathExtension: url.pathExtension) else { return nil }

    let itemProvider = NSItemProvider()

    /*
    itemProvider.registerDataRepresentation(forTypeIdentifier: fileType.typeIdentifier, visibility: .all) { completion in
        do {
            let data = try Data(contentsOf: url)
            completion(data, nil)
        }
        catch {
            completion(nil, error)
        }
        return nil
    }
 */
    itemProvider.registerFileRepresentation(forTypeIdentifier: fileType.typeIdentifier, fileOptions: [], visibility: .all) { completion in
        completion(url, false, nil)
        return nil
    }
    itemProvider.suggestedName = url.lastPathComponent

    return itemProvider
}

func createEmptyFile(for itemProvider: NSItemProvider, typeIdentifiers: Set<String>, directory: URL) -> URL? {

    for typeIdentifier in itemProvider.registeredTypeIdentifiers {
        guard typeIdentifiers.contains(typeIdentifier) else { continue }
        guard let type = C64FileType(typeIdentifier: typeIdentifier) else { continue }
        
        do {
            return try uniqeName(directory: directory, name: itemProvider.suggestedName, pathExtension: type.pathExtension, create: true)
        }
        catch {
            return nil
        }
    }
    
    return nil
}

func saveFrom(_ itemProvider: NSItemProvider, typeIdentifiers: Set<String>, to fileURL: URL, completionHandler: @escaping ((_ error: Error?) -> Void)) -> Bool {
    for typeIdentifier in itemProvider.registeredTypeIdentifiers {
        guard typeIdentifiers.contains(typeIdentifier) else { continue }
        
        itemProvider.loadDataRepresentation(forTypeIdentifier: typeIdentifier) { data, error in
            guard error == nil else {
                DispatchQueue.main.async {
                    completionHandler(error)
                }
                return
            }
            
            do {
                try data?.write(to: fileURL)
                DispatchQueue.main.async {
                    completionHandler(nil)
                }
            }
            catch {
                DispatchQueue.main.async {
                    completionHandler(error)
                }
            }
        }
        return true
    }
    return false
}

extension NSRegularExpression {
    convenience init(_ pattern: String) {
        do {
            try self.init(pattern: pattern)
        } catch {
            fatalError("Illegal regular expression: \(pattern).")
        }
    }
    
    func doesMatch(_ string: String) -> Bool {
        let range = NSRange(location: 0, length: string.utf16.count)
        return firstMatch(in: string, options: [], range: range) != nil
    }
    
    func matches(_ string: String) -> [String]? {
        let range = NSRange(location: 0, length: string.utf16.count)
        guard let match = firstMatch(in: string, options: [], range: range) else { return nil }
        let nsString = string as NSString
        return (0 ..< match.numberOfRanges).map { index in
            let subRange = match.range(at: index)
            if subRange.location == NSNotFound {
                return ""
            }
            else {
                return nsString.substring(with: subRange)
            }
        }
    }
    
    func stringByReplacingMatches(in string: String, withTemplate template: String) {
        let range = NSRange(location: 0, length: string.utf16.count)
        stringByReplacingMatches(in: string, options: [], range: range, withTemplate: template)
    }
}
