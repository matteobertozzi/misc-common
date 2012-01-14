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

#ifndef _DISK_READER_H_
#define _DISK_READER_H_

#include "Seekable.h"
#include "Readable.h"

class DiskReader : public Readable, public Seekable {
    public:
        DiskReader();
        ~DiskReader();

        bool open (int fd);
        bool open (const char *path);
        void close (void);

        int read (void *buf, unsigned int size);

        int      seek   (uint64_t offset);
        int      skip   (uint64_t n);
        uint64_t tell   (void);
        uint64_t length (void);

    private:
        uint64_t _offset;
        int _fd;
};

#endif /* !_DISK_READER_H_ */

