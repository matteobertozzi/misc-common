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

#ifndef _BUFFERED_WRITER_H_
#define _BUFFERED_WRITER_H_

#include "Writable.h"

class BufferedWriter : public Writable {
    public:
        BufferedWriter(Writable *writable, unsigned int buf_size);
        virtual ~BufferedWriter();

        int write (const void *buffer, unsigned int size);
        virtual int flush (void);

    protected:
        virtual int flushBuffer (const void *buffer, unsigned int size) {
            return(_writable->writeFully(buffer, size));
        }

    protected:
        Writable *_writable;

    private:
        uint8_t *    _buffer;
        unsigned int _buf_size;
        unsigned int _buf_used;
};

#endif /* !_BUFFERED_WRITER_H_ */

