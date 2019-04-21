/*
 DirectoryWatcher.swift -- Watch file system directory for changes.
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

class DirectoryWatcher {
    typealias ChangeHandler = (_ directoryWatcher: DirectoryWatcher) -> Void

    var changeHandler: ChangeHandler
    let url: URL
    
    private var dispatchSource: DispatchSourceFileSystemObject
    
    init?(url: URL, changeHandler: @escaping ChangeHandler) {
        self.url = url
        self.changeHandler = changeHandler
        
        let fileDescriptor = open(url.path, O_EVTONLY)
        guard fileDescriptor >= 0 else { return nil }

        dispatchSource = DispatchSource.makeFileSystemObjectSource(fileDescriptor: fileDescriptor, eventMask: .write, queue: DispatchQueue.main)
        dispatchSource.setEventHandler { [unowned self] in
            self.changeHandler(self)
        }
        dispatchSource.setCancelHandler {
            close(fileDescriptor)
        }
        
        dispatchSource.resume()
    }
    
    deinit {
        dispatchSource.setEventHandler(handler: nil)
        dispatchSource.cancel()
    }
}
