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
#include <stdlib.h>

#include "Buffer.h"

Buffer::Buffer(size_t n) {
    _blob = NULL;
    _block = n;
    _size = 0;
}

Buffer::~Buffer() {
    if (_blob != NULL) {
        free(_blob);
        _blob = NULL;
        _size = 0;
    }
}

void Buffer::clear (void) {
    _size = 0;
}

int Buffer::squeeze (void) {
    if (_size > 0) {
        uint8_t *blob;

        if ((blob = (uint8_t *) realloc(_blob, _size)) == NULL)
            return(-1);

        _block = _size;
        _blob = blob;
    } else {
        free(_blob);
        _blob = NULL;
        _block = 0;
    }

    return(0);
}

int Buffer::reserve (size_t n) {
    if (n > _block) {
        if (bufferGrow(n))
            return(-1);
    }
    return(0);
}

int Buffer::truncate (size_t n) {
    if (n >= _size)
        return(-1);

    _size = n;
    return(0);
}

int Buffer::set (const void *blob, size_t size) {
    if (size > _block) {
        if (bufferGrow(size))
            return(-1);
    }

    memmove(_blob, blob, size);
    _size = size;
    return(0);
}


int Buffer::append  (const void *blob, size_t size) {
    size_t n;

    if ((n = _size + size) >= _block) {
        if (bufferGrow(n))
            return(-1);
    }

    memcpy(_blob + _size, blob, size);
    _size = n;
    return(0);
}

int Buffer::prepend (const void *blob, size_t size) {
    size_t n;

    if ((n = (_size + size)) > _block) {
        if (bufferGrow(n))
            return(-1);
    }

    if ((uint8_t *)blob >= _blob && (uint8_t *)blob < (_blob + _block)) {
        uint8_t *dblob;

        if ((dblob = (uint8_t *) malloc(size)) == NULL)
            return(-1);

        memcpy(dblob, _blob, size);
        memmove(_blob + size, _blob, _size);
        memcpy(_blob, dblob, size);

        free(dblob);
    } else {
        memmove(_blob + size, _blob, _size);
        memcpy(_blob, blob, size);
    }

    _size += size;

    return(0);
}

int Buffer::insert (size_t index, const void *blob, size_t n) {
    unsigned char *p;
    size_t size;

    size = _size + n;
    if (index > _size)
        size += (index - _size);

    if (size > _block) {
        if (bufferGrow(size))
            return(-1);
    }

    p = (_blob + index);
    if (index > _size) {
        memset(_blob + _size, 0, index - _size);
        memcpy(p, blob, n);
    } else if ((uint8_t *)blob >= _blob &&
               (uint8_t *)blob < (_blob + _block))
    {
        uint8_t *dblob;

        if ((dblob = (uint8_t *) malloc(n)) == NULL)
            return(-1);

        memcpy(dblob, _blob, n);
        memmove(p + n, p, _size - index);
        memcpy(p, dblob, n);

        free(dblob);
    } else {
        memmove(p + n, p, _size - index);
        memcpy(p, blob, n);
    }

    _size = size;

    return(0);
}

int Buffer::replace (size_t index, size_t size, const void *blob, size_t n) {
    if (size == n && (index + n) <= _size) {
        memmove(_blob + index, blob, n);
    } else {
        remove(index, size);
        if (insert(index, blob, n))
            return(-3);
    }
    return(0);
}

int Buffer::remove (size_t index, size_t size) {
    if (index >= _size || size == 0)
        return(-1);

    if ((index + size) >= _size) {
        _size = index;
    } else {
        uint8_t *p;

        p = _blob + index;
        memmove(p, p + size, _size - index - size);
        _size -= size;
    }

    return(0);
}

int Buffer::compare (const void *blob, size_t n) const {
    size_t min_size = (n < _size) ? n : _size;
    int cmp;

    if ((cmp = memcmp(_blob, blob, min_size)) != 0)
        return(cmp);

    return((_size < n) ? -1 : (_size > n) ? 1 : 0);
}

bool Buffer::equals (const void *blob, size_t n) const {
    if (n != _size)
        return(false);

    return(!memcmp(_blob, blob, n));
}

int Buffer::bufferGrow (size_t n) {
    uint8_t *blob;
    size_t size;

    size = (n + 0x1ff) & (-512);
    if ((blob = (uint8_t *) realloc(_blob, size)) == NULL)
        return(-1);

    _block = size;
    _blob = blob;
    return(0);
}

