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

#include "BufferedReader.h"

/* ============================================================================
 *  Buffered Reader
 */
BufferedReader::BufferedReader(Readable *readable, unsigned int buf_size) {
    _readable = readable;
    _buffer = NULL;
    _buf_required = buf_size;
    _buf_readed = 0;
    _buf_size = 0;
}

BufferedReader::~BufferedReader() {
    if (_buffer != NULL) {
        delete[] _buffer;
        _buffer = NULL;
    }
}

int BufferedReader::read (void *buffer, unsigned int size) {
    unsigned int buf_avail = _buf_size - _buf_readed;
    uint8_t *pbuf = (uint8_t *)buffer;
    int n, rd;

    // Direct read if requested size is larger than buffer and no buf
    if (!buf_avail && size >= _buf_required)
        return(_readable->readFully(buffer, size));

    if (size <= buf_avail) {
        memcpy(buffer, _buffer + _buf_readed, size);
        _buf_readed += size;
        return(size);
    }

    if (_buffer == NULL)
        _buffer = new uint8_t[_buf_required];

    // Copy the old buffer to the end
    if ((n = buf_avail) > 0) {
        memcpy(pbuf, _buffer + _buf_readed, buf_avail);
        pbuf += buf_avail;
        size -= buf_avail;
    }

    // Direct read if requested size is larger than buffer
    if (size >= _buf_required) {
        int rd = _readable->readFully(pbuf, size);
        _buf_readed = 0;
        _buf_size = 0;
        return((rd > 0) ? (n + rd) : n);
    }

    // Fill the buffer
    if ((rd = _readable->readFully(_buffer, _buf_required)) <= 0)
        return(n);

    // Copy to used
    memcpy(pbuf, _buffer, size);
    _buf_readed = size;
    _buf_size = rd;
    return(n + size);
}

