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

#ifndef _COMPRESSED_WRITER_H_
#define _COMPRESSED_WRITER_H_

#include <stdint.h>
#include <stddef.h>

#include "BufferedWriter.h"

class CompressedWriter : public BufferedWriter {
    public:
        CompressedWriter(Writable *writable, unsigned int buf_size)
            : BufferedWriter(writable, buf_size)
        {
        }

    protected:
        virtual int flushBuffer (const void *buffer, unsigned int size) {
            uint8_t cbuffer[maxLengthForInput(size)];
            unsigned int csize = compress(buffer, cbuffer, size);
            // TODO: Check Returns
            int n;
            n  = _writable->writeVUInt(size);       // Uncompressed Size
            n += _writable->writeVUInt(csize);      // Compressed Size
            n += _writable->writeFully(cbuffer, csize);
            return(n);
        }

        virtual unsigned int maxLengthForInput (unsigned int size) const = 0;
        virtual unsigned int compress (const void *src, void *dst, unsigned int size) = 0;
};

int LZ4_compress (char* source, char* dest, int isize);
class Lz4Writer : public CompressedWriter {
    public:
        Lz4Writer(Writable *writable, unsigned int buf_size)
            : CompressedWriter(writable, buf_size)
        {
        }

    protected:
        unsigned int compress (const void *src, void *dst, unsigned int size) {
            return(LZ4_compress((char *)src, (char *)dst, size));
        }

        unsigned int maxLengthForInput (unsigned int size) const {
            unsigned int extra = (size * 0.4f);
            return(size + ((extra > 8) ? extra : 8));
        }
};

#endif /* !_COMPRESSED_WRITER_H_ */

