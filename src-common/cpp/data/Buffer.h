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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "Seekable.h"
#include "Readable.h"
#include "Writable.h"

class Buffer {
    public:
        Buffer(size_t n=0);
        ~Buffer();

        void clear   (void);

        int squeeze  (void);
        int reserve  (size_t n);
        int truncate (size_t n);

        int set      (const void *blob, size_t n);
        int append   (const void *blob, size_t n);
        int prepend  (const void *blob, size_t n);
        int insert   (size_t index, const void *blob, size_t n);
        int replace  (size_t index, size_t size, const void *blob, size_t n);
        int remove   (size_t index, size_t size);

        bool isEmpty (void) const { return(_size == 0); }
        size_t size  (void) const { return(_size); }

        int compare (const Buffer& other) const {
            return(compare(other._blob, other._size));
        }

        int compare (const Buffer *other) const {
            return(compare(other->_blob, other->_size));
        }

        int compare (const void *blob, size_t n) const;

        bool equals (const Buffer& other) const {
            return(equals(other._blob, other._size));
        }

        bool equals (const Buffer *other) const {
            return(equals(other->_blob, other->_size));
        }

        bool equals (const void *blob, size_t n) const;

        uint8_t at(size_t n) const { return(_blob[n]); }
        uint8_t& operator[] (size_t n) { return(_blob[n]);}
        uint8_t operator[]  (size_t n) const { return(_blob[n]); }

    protected:
        int bufferGrow (size_t n);

    private:
        uint8_t *_blob;
        size_t   _block;
        size_t   _size;
};

inline bool operator==(const Buffer& x, const Buffer& y) {
  return(x.equals(y));
}

inline bool operator!=(const Buffer& x, const Buffer& y) {
  return(!(x.equals(y)));
}

class BufferReader : public Readable, public Seekable {
    public:
        BufferReader(const Buffer *buffer, size_t offset=0) {
            _buffer = buffer;
            _offset = offset;
        }

        int read (void *buffer, unsigned int n) {
            size_t length = (_buffer->size() - _offset);
            uint8_t *pbuf = (uint8_t *)buffer;
            int rd;

            length = rd = (n < length) ? n : length;
            while (length-- > 0)
                *pbuf++ = _buffer->at(_offset++);

            return(rd);
        }

        int seek (uint64_t offset) {
            if (offset > _buffer->size())
                return(1);

            _offset = offset;
            return(0);
        }

        int skip (uint64_t n) { return(seek(_offset + n)); }

        uint64_t tell (void) { return(_offset); }
        uint64_t length (void) { return(_buffer->size()); }

    protected:
        const Buffer *_buffer;

    private:
        size_t _offset;
};

class BufferWriter : public Writable {
    public:
        BufferWriter(Buffer *buffer) {
            _buffer = buffer;
        }

        int write (const void *buffer, unsigned int n) {
            if (_buffer->append(buffer, n))
                return(-1);
            return(n);
        }

        int flush (void) { return(0); }

    protected:
        Buffer *_buffer;
};

#endif /* !_BUFFER_H_ */
