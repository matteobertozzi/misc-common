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

#ifndef _COMPRESSED_READER_H_
#define _COMPRESSED_READER_H_

#include <stdint.h>
#include <stddef.h>

#include "Readable.h"

class CompressedReader : public Readable {
    public:
        CompressedReader(Readable *readable) {
            _readable = readable;
            _buf_readed = 0;
            _buf_size = 0;
            _buffer = NULL;
        }

        int read (void *buffer, unsigned int size);

    protected:
        virtual unsigned int decompress (const void *src,
                                         unsigned int isize,
                                         void *dst,
                                         unsigned int osize) = 0;

    private:
        int readBuffer (void);

    protected:
        Readable *_readable;

    private:
        uint8_t *    _buffer;
        unsigned int _buf_size;
        unsigned int _buf_readed;
};

int LZ4_uncompress (char* source, char* dest, int osize);
class Lz4Reader : public CompressedReader {
    public:
        Lz4Reader(Readable *readable)
            : CompressedReader(readable)
        {
        }

    protected:
        unsigned int decompress (const void *src,
                                 unsigned int isize,
                                 void *dst,
                                 unsigned int osize)
        {
            return(LZ4_uncompress((char *)src, (char *)dst, osize));
        }
};

#endif /* !_COMPRESSED_READER_H_ */

