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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "DiskReader.h"

DiskReader::DiskReader() {
    _fd = -1;
    _offset = 0;
}

DiskReader::~DiskReader() {
    close();
}

bool DiskReader::open (int fd) {
    _fd = fd;
    _offset = lseek(_fd, 0, SEEK_CUR);
    return((off_t)_offset != (off_t)-1);
}

bool DiskReader::open (const char *path) {
    _fd = ::open(path, O_RDONLY);
    _offset = 0;
    return(_fd >= 0);
}

void DiskReader::close (void) {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

int DiskReader::read (void *buf, unsigned int size) {
    int rd;

    if ((rd = pread(_fd, buf, size, _offset)) > 0)
        _offset += rd;

    return(rd);
}

int DiskReader::seek (uint64_t offset) {
    _offset = offset;
    return(0);
}

int DiskReader::skip (uint64_t n) {
    _offset += n;
    return(0);
}

uint64_t DiskReader::tell (void) {
    return(_offset);
}

uint64_t DiskReader::length (void) {
    uint64_t length;
    off_t off;

    off = lseek(_fd, 0, SEEK_CUR);
    length = lseek(_fd, 0, SEEK_END);
    lseek(_fd, off, SEEK_SET);

    return(length);
}

