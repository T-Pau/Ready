/*
 Multiset.swift -- Multiset
 Created by Simon Whitaker on 28/08/2017.

 Copyright (c) 2016 Matthijs Hollemans and contributors
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

import Foundation

public struct Multiset<T: Hashable> {
    private var storage: [T: UInt] = [:]
    public private(set) var count: UInt = 0
    
    public init() {}
    
    public init<C: Collection>(_ collection: C) where C.Element == T {
        for element in collection {
            self.add(element)
        }
    }
    
    public mutating func add (_ elem: T) {
        storage[elem, default: 0] += 1
        count += 1
    }
    
    public mutating func remove (_ elem: T) {
        if let currentCount = storage[elem] {
            if currentCount > 1 {
                storage[elem] = currentCount - 1
            } else {
                storage.removeValue(forKey: elem)
            }
            count -= 1
        }
    }
    
    public func isSubSet (of superset: Multiset<T>) -> Bool {
        for (key, count) in storage {
            let supersetcount = superset.storage[key] ?? 0
            if count > supersetcount {
                return false
            }
        }
        return true
    }
    
    public func count(for key: T) -> UInt {
        return storage[key] ?? 0
    }
    
    public var allItems: [T] {
        var result = [T]()
        for (key, count) in storage {
            for _ in 0 ..< count {
                result.append(key)
            }
        }
        return result
    }
}

// MARK: - Equatable
extension Multiset: Equatable {
    public static func == (lhs: Multiset<T>, rhs: Multiset<T>) -> Bool {
        if lhs.storage.count != rhs.storage.count {
            return false
        }
        for (lkey, lcount) in lhs.storage {
            let rcount = rhs.storage[lkey] ?? 0
            if lcount != rcount {
                return false
            }
        }
        return true
    }
}

// MARK: - ExpressibleByArrayLiteral
extension Multiset: ExpressibleByArrayLiteral {
    public init(arrayLiteral elements: T...) {
        self.init(elements)
    }
}
