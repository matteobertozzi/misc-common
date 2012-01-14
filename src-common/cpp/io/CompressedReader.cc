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

#include <string.h>

#include "CompressedReader.h"

int CompressedReader::read (void *buffer, unsigned int size) {
    unsigned int buf_avail = _buf_size - _buf_readed;
    uint8_t *pbuf = (uint8_t *)buffer;
    int n;

    if (size <= buf_avail) {
        memcpy(buffer, _buffer + _buf_readed, size);
        _buf_readed += size;
        return(size);
    }

    // Copy the old buffer to the end.
    if ((n = buf_avail) > 0) {
        memcpy(pbuf, _buffer + _buf_readed, buf_avail);
        pbuf += buf_avail;
        size -= buf_avail;
    }

    do {
        // Read new Buffer
        if (readBuffer() < 0)
            return(-1);

        // Copy to user
        buf_avail = (size > _buf_size) ? _buf_size : size;
        memcpy(pbuf, _buffer, buf_avail);
        pbuf += buf_avail;
        size -= buf_avail;
        n += buf_avail;
    } while (size > 0);

    return(n);
}

int CompressedReader::readBuffer (void) {
    uint64_t size, csize;

    // Read Header
    if (_readable->readVUInt(&size) <= 0)
        return(-1);

    if (_readable->readVUInt(&csize) <= 0)
        return(-2);

    // Read Compressed Data
    uint8_t cbuffer[csize];
    if (_readable->readFully(cbuffer, csize) != (int)csize)
        return(-3);

    // Prepare new buffer for data
    if (size != _buf_size) {
        if (_buffer != NULL)
            delete[] _buffer;
        _buffer = new uint8_t[size];
        _buf_size = size;
        _buf_readed = 0;
    }

    // Uncompress
    return(decompress(cbuffer, csize, _buffer, _buf_size));
}

