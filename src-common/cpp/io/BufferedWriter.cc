/*
 *   Copyright 2012 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include "BufferedWriter.h"

#include <stdio.h>

/* ============================================================================
 *  Buffered Writer
 */
BufferedWriter::BufferedWriter(Writable *writable, unsigned int buf_size) {
    _writable = writable;
    _buffer = NULL;
    _buf_size = buf_size;
    _buf_used = 0;
}

BufferedWriter::~BufferedWriter() {
    if (_buf_used > 0)
        fprintf(stderr, "WARNING: Data not flushed before destruction\n");

    if (_buffer != NULL)
        delete[] _buffer;
}

int BufferedWriter::write (const void *buffer, unsigned int size) {
    const uint8_t *pbuf = (const uint8_t *)buffer;
    unsigned int buf_avail;
    unsigned int n;

    if (_buffer == NULL)
        _buffer = new uint8_t[_buf_size];

    buf_avail = _buf_size - _buf_used;
    if (size <= buf_avail) {
        memcpy(_buffer + _buf_used, pbuf, size);
        _buf_used += size;
        return(size);
    }

    // Fill buffer and flush
    memcpy(_buffer + _buf_used, pbuf, buf_avail);
    pbuf += buf_avail;
    size -= buf_avail;
    flushBuffer(_buffer, _buf_size);
    n = buf_avail;

    // Flush directly if input is greater than buffer
    while (size >= _buf_size) {
        flushBuffer(pbuf, _buf_size);
        pbuf += _buf_size;
        size -= _buf_size;
        n += _buf_size;
    }

    // Store remaining in buffer
    _buf_used = size;
    memcpy(_buffer, pbuf, size);
    n += size;

    return(n);
}

int BufferedWriter::flush (void) {
    if (_buf_used > 0) {
        int r = flushBuffer(_buffer, _buf_used);
        _buf_used = 0;
        return(r);
    }
    return(0);
}

