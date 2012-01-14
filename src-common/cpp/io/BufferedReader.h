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

#ifndef _BUFFERED_READER_H_
#define _BUFFERED_READER_H_

#include "Readable.h"

class BufferedReader : public Readable {
    public:
        BufferedReader(Readable *readable, unsigned int buf_size);
        virtual ~BufferedReader();

        int read (void *buffer, unsigned int size);

    protected:
        Readable *_readable;

    private:
        uint8_t *    _buffer;
        unsigned int _buf_size;
        unsigned int _buf_readed;
        unsigned int _buf_required;
};

#endif /* !_BUFFERED_READER_H_ */

