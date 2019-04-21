/*
 StateEncodable.swift -- typed, keyed access to state encoding
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

import CoreData
import UIKit

protocol StateEncodable {
    associatedtype StateKey: RawRepresentable
}

extension StateEncodable where StateKey.RawValue == String {
    func encode(bool: Bool?, forKey key: StateKey, inCoder coder: NSCoder) {
        if let value = bool {
            coder.encode(value, forKey: key.rawValue)
        }
    }
    
    func decodeBool(forKey key: StateKey, fromCoder coder: NSCoder) -> Bool? {
        guard coder.containsValue(forKey: key.rawValue) else { return nil }
        return coder.decodeBool(forKey: key.rawValue)
    }

    func encode(cgFloat: CGFloat?, forKey key: StateKey, inCoder coder: NSCoder) {
        if let value = cgFloat {
            coder.encode(Double(value), forKey: key.rawValue)
        }
    }
    
    func decodeCGFloat(forKey key: StateKey, fromCoder coder: NSCoder) -> CGFloat? {
        guard coder.containsValue(forKey: key.rawValue) else { return nil }
        return CGFloat(coder.decodeDouble(forKey: key.rawValue))
    }

    func encode(int value: Int?, forKey key: StateKey, inCoder coder: NSCoder) {
        if let real_value = value {
            coder.encode(real_value, forKey: key.rawValue)
        }
    }
    
    func decodeInt(forKey key: StateKey, fromCoder coder: NSCoder) -> Int? {
        guard coder.containsValue(forKey: key.rawValue) else { return nil }
        return coder.decodeInteger(forKey: key.rawValue)
    }

    func encode(any: Any?, forKey key: StateKey, inCoder coder: NSCoder) {
        if let value = any {
            coder.encode(value, forKey: key.rawValue)
        }
    }
    
    func decodeAny(forKey key: StateKey, fromCoder coder: NSCoder) -> Any? {
        guard coder.containsValue(forKey: key.rawValue) else { return nil }
        return coder.decodeObject(forKey: key.rawValue)
    }
    
    func encode(managedObject: NSManagedObject?, forKey key: StateKey, inCoder coder: NSCoder) {
        encode(any: managedObject?.objectID.uriRepresentation(), forKey: key, inCoder: coder)
    }
    
    func decodeManagedObject(forKey key: StateKey, fromCoder coder: NSCoder) -> NSManagedObject? {
        guard let url = decodeAny(forKey: key, fromCoder: coder) as? URL,
            let managedObjectContext = ((UIApplication.shared.delegate) as? AppDelegate)?.persistentContainer.viewContext,
            let objectId = managedObjectContext.persistentStoreCoordinator?.managedObjectID(forURIRepresentation: url) else { return nil }
            return managedObjectContext.object(with: objectId)
    }
    
    func encode(string: String?, forKey key: StateKey, inCoder coder: NSCoder) {
        encode(any: string, forKey: key, inCoder: coder)
    }
    
    func decodeString(forKey key: StateKey, fromCoder coder: NSCoder) -> String? {
        return decodeAny(forKey: key, fromCoder: coder) as? String
    }
}
