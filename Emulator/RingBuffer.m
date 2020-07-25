/*
 RingBuffer.m -- Thread Save Ring Buffer
 Copyright (C) 2020 Dieter Baron
 
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

#include "RingBuffer.h"

@implementation RingBuffer

-(instancetype)initSize: (size_t)size {
    pthread_mutex_init(&mutex, NULL);
    _size = size;
    _buffer = (uint8_t *)malloc(size);
    _end = _buffer + size;
    _read_position = _buffer;
    _write_position = _buffer;
    _isEmpty = false;
    return self;
}


- (void)dealloc {
    pthread_mutex_destroy(&mutex);
    free(_buffer);
}


- (size_t)bytesReadable {
    size_t ret;
    pthread_mutex_lock(&mutex);

    if (_isEmpty) {
        ret = 0;
    }
    else {
        ret = (_write_position - _read_position + _size) % _size;
    }

    pthread_mutex_unlock(&mutex);
    return ret;
}


- (size_t)bytesWritable {
    size_t ret;
    pthread_mutex_lock(&mutex);
    
    if (_isEmpty) {
        ret =  _size;
    }
    else {
        ret = (_read_position - _write_position + _size) % _size;
    }
    
    pthread_mutex_unlock(&mutex);
    return ret;
}

-(size_t)readBuffer: (uint8_t *)buffer length: (size_t)length {
    pthread_mutex_lock(&mutex);

    size_t offset = 0;
    
    if (!_isEmpty) {
        if (_read_position >= _write_position) {
            size_t n = MIN(_end - _read_position, length);
            memcpy(buffer, _read_position, n);
            offset = n;
            _read_position += n;
            if (_read_position == _end) {
                _read_position = _buffer;
            }
        }
        
        if (offset < length) {
            size_t n = MIN(_write_position - _read_position, length - offset);
            memcpy(buffer + offset, _read_position, n);
            offset += n;
            _read_position += n;
        }
    
        if (_read_position == _write_position) {
            _isEmpty = YES;
            _read_position = _buffer;
            _write_position = _buffer;
        }
    }
    
    pthread_mutex_unlock(&mutex);
    return offset;
}


-(size_t)writeBuffer: (const uint8_t *)buffer length: (size_t)length {
    pthread_mutex_lock(&mutex);
    
    size_t offset = 0;
    
    if ((_isEmpty || _write_position != _read_position) && length > 0) {
        if (_write_position >= _read_position) {
            size_t n = MIN(_end - _write_position, length);
            memcpy(_write_position, buffer, n);
            offset = n;
            _write_position += n;
            if (_write_position == _end) {
                _write_position = _buffer;
            }
        }
        
        if (offset < length) {
            size_t n = MIN(_read_position - _write_position, length - offset);
            memcpy(_write_position, buffer + offset, n);
            offset += n;
            _write_position += n;
        }

        _isEmpty = NO;
    }
    
    pthread_mutex_unlock(&mutex);
    return offset;
}

@end
