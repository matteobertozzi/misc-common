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

#ifndef _IO_STREAM_H_
#define _IO_STREAM_H_

#include <stdint.h>
#include <stdio.h>

typedef const struct stream_vtable stream_vtable_t;
typedef struct stream stream_t;

struct stream_vtable {
    int (*write)     (stream_t *stream, const void *buf, unsigned int n);
    int (*flush)     (stream_t *stream);
    int (*zread)     (stream_t *stream, void **buf, unsigned int n);
    int (*read)      (stream_t *stream, void *buf, unsigned int n);
    int (*seek)      (stream_t *stream, uint64_t offset);

    int (*can_write) (stream_t *stream);
    int (*can_zread) (stream_t *stream);
    int (*can_read)  (stream_t *stream);
    int (*can_seek)  (stream_t *stream);

    uint64_t (*position) (stream_t *stream);
    uint64_t (*length)   (stream_t *stream);
};

struct stream {
    stream_vtable_t *vtable;
};

#define io_stream_vtable(stream)            (((stream_t *)(stream))->vtable)

#define io_stream_call(stream, method, ...)                                 \
    io_stream_vtable(stream)->method((stream_t *)stream, ##__VA_ARGS__)

#define io_stream_has_method(stream, method)                                \
    (io_stream_vtable(stream)->method != NULL)

#define io_stream_can(stream, method)                                       \
    (io_stream_has_method(can_ ## method) ?                                 \
        io_stream_call(can_ ## method) :                                    \
        io_stream_has_method(method))

#define io_write(stream, buf, n)        io_stream_call(stream, write, buf, n)
#define io_read(stream, buf, n)         io_stream_call(stream, read, buf, n)
#define io_flush(stream)                io_stream_call(stream, flush)
#define io_seek(stream, offset)         io_stream_call(stream, seek, offset)
#define io_position(stream)             io_stream_call(stream, position)
#define io_length(stream)               io_stream_call(stream, length)

#define io_can_write(stream)            io_stream_can(stream, write)
#define io_can_read(stream)             io_stream_can(stream, read)

int     io_write_fully  (stream_t *stream, const void *buffer, unsigned int size);
int     io_write_uint8  (stream_t *stream, uint8_t value);
int     io_write_uint16 (stream_t *stream, uint16_t value);
int     io_write_uint32 (stream_t *stream, uint32_t value);
int     io_write_uint64 (stream_t *stream, uint64_t value);
int     io_write_vuint  (stream_t *stream, uint64_t value);

int     io_write_int8   (stream_t *stream, int8_t value);
int     io_write_int16  (stream_t *stream, int16_t value);
int     io_write_int32  (stream_t *stream, int32_t value);
int     io_write_int64  (stream_t *stream, int64_t value);
int     io_write_vint   (stream_t *stream, int64_t value);

int     io_read_fully   (stream_t *stream, void *buffer, unsigned int size);
int     io_read_uint8   (stream_t *stream, uint8_t *value);
int     io_read_uint16  (stream_t *stream, uint16_t *value);
int     io_read_uint32  (stream_t *stream, uint32_t *value);
int     io_read_uint64  (stream_t *stream, uint64_t *value);
int     io_read_vuint   (stream_t *stream, uint64_t *value);

int     io_read_int8    (stream_t *stream, int8_t *value);
int     io_read_int16   (stream_t *stream, int16_t *value);
int     io_read_int32   (stream_t *stream, int32_t *value);
int     io_read_int64   (stream_t *stream, int64_t *value);
int     io_read_vint    (stream_t *stream, int64_t *value);

#endif /* !_IO_STREAM_H_ */

