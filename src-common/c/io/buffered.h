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

#ifndef _IO_BUFFERED_H_
#define _IO_BUFFERED_H_

#include "stream.h"

typedef struct buffered_writer buffered_writer_t;
typedef struct buffered_reader buffered_reader_t;

struct buffered_writer {
    stream_t __base_type__;
    stream_t *     stream;
    unsigned char *blob;
    unsigned int   size;
    unsigned int   used;
};

struct buffered_reader {
    stream_t __base_type__;
    stream_t *     stream;
    unsigned char *blob;
    unsigned int   size;
    unsigned int   used;
    unsigned int   reqs;
};

int     buffered_writer_open        (buffered_writer_t *writer,
                                     stream_t *stream,
                                     unsigned int size);
void    buffered_writer_close       (buffered_writer_t *writer);

int     buffered_reader_open        (buffered_reader_t *reader,
                                     stream_t *stream,
                                     unsigned int size);
void    buffered_reader_close       (buffered_reader_t *reader);

#endif /* !_IO_BUFFERED_H_ */

