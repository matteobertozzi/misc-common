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

#ifndef _IO_ENCODED_H_
#define _IO_ENCODED_H_

#include "buffered.h"
#include "stream.h"
#include "codec.h"

typedef struct encoded_writer encoded_writer_t;
typedef struct encoded_reader encoded_reader_t;

struct encoded_writer {
    stream_t __base_type__;
    stream_t *     stream;
    codec_t *      codec;
    unsigned char *blob;
    unsigned int   size;
    unsigned int   used;
};

struct encoded_reader {
    stream_t __base_type__;
    stream_t *     stream;
    codec_t *      codec;
    unsigned char *blob;
    unsigned int   size;
    unsigned int   used;
};

int encoded_writer_open (encoded_writer_t *writer,
                         codec_t *codec,
                         stream_t *stream,
                         unsigned int size);
void encoded_writer_close (encoded_writer_t *writer);

int encoded_reader_open (encoded_reader_t *reader,
                         codec_t *codec,
                         stream_t *stream);
void encoded_reader_close (encoded_reader_t *reader);

#endif /* !_IO_ENCODED_H_ */

