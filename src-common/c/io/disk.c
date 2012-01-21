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

#include "disk.h"

static int __disk_write (stream_t *stream, const void *buf, unsigned int n) {
    disk_stream_t *disk = ((disk_stream_t *)stream);
    ssize_t rd;
    
    if ((rd = pwrite(disk->fd, buf, n, disk->offset)) > 0)
        disk->offset += rd;
        
    return((int)rd);
}

static int __disk_flush (stream_t *stream) {
    return(fsync(((disk_stream_t *)stream)->fd));    
}

static int __disk_read (stream_t *stream, void *buf, unsigned int n) {
    disk_stream_t *disk = ((disk_stream_t *)stream);
    ssize_t wr;
    
    if ((wr = pread(disk->fd, buf, n, disk->offset)) > 0)
        disk->offset += wr;
        
    return((int)wr);
}

static int __disk_seek (stream_t *stream, uint64_t offset) {
    ((disk_stream_t *)stream)->offset = offset;
    return(0);
}

static int __disk_can_write (stream_t *stream) {
    int flags = fcntl(((disk_stream_t *)stream)->fd, F_GETFL) & O_ACCMODE;
    return((flags & O_WRONLY) || (flags & O_RDWR));
}

static int __disk_can_read (stream_t *stream) {
    int flags = fcntl(((disk_stream_t *)stream)->fd, F_GETFL) & O_ACCMODE;
    return((flags & O_RDONLY) || (flags & O_RDWR));
}

uint64_t __disk_position (stream_t *stream) {
    return(((disk_stream_t *)stream)->offset);
}

uint64_t __disk_length (stream_t *stream) {
    struct stat buf;
    fstat(((disk_stream_t *)stream)->fd, &buf);
    return(buf.st_size);
}

static stream_vtable_t __disk_vtable = {
    .write = __disk_write,
    .flush = __disk_flush,
    .zread = NULL,
    .read  = __disk_read,
    .seek  = __disk_seek,
    
    .can_write = __disk_can_write,
    .can_zread = NULL,
    .can_read  = __disk_can_read,
    .can_seek  = NULL,
    
    .position  = __disk_position,
    .length    = __disk_length,
};

int disk_stream_open (disk_stream_t *disk, const char *filename, int flags) {
    disk->__base_type__.vtable = &__disk_vtable;
    if ((disk->fd = open(filename, flags)) < 0)
        return(-1);
    disk->offset = 0;
    return(0);
}

int disk_stream_create (disk_stream_t *disk, 
                        const char *filename, 
                        int flags,
                        int mode)
{
    disk->__base_type__.vtable = &__disk_vtable;
    if ((disk->fd = open(filename, flags, mode)) < 0)
        return(-1);
    disk->offset = 0;
    return(0);
}

void disk_stream_close (disk_stream_t *disk) {
    close(disk->fd);    
}

