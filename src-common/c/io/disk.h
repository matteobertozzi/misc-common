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

#ifndef _IO_DISK_H_
#define _IO_DISK_H_

#include "stream.h"

typedef struct disk_stream disk_stream_t;

struct disk_stream {
    stream_t __base_type__;
    uint64_t offset;
    int fd;
};

int     disk_stream_open    (disk_stream_t *disk, 
                             const char *filename, 
                             int flags);
int     disk_stream_create  (disk_stream_t *disk, 
                             const char *filename, 
                             int flags,
                             int mode);
void    disk_stream_close   (disk_stream_t *disk);

#endif /* !_IO_DISK_H_ */

