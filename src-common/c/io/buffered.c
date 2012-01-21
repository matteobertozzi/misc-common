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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffered.h"

/* ============================================================================
 *  Buffered Writer
 */
static int __buffered_write (stream_t *stream,
                             const void *blob,
                             unsigned int size)
{
    buffered_writer_t *writer = (buffered_writer_t *)stream;
    const unsigned char *pblob = (const unsigned char *)blob;
    unsigned int avail = writer->size - writer->used;
    int n, wr;

    if (writer->blob == NULL) {
        if ((writer->blob = (unsigned char *) malloc(writer->size)) == NULL)
            return(0);
    }

    /* There's all the space that you need! */
    if (size <= avail) {
        memcpy(writer->blob + writer->used, pblob, size);
        writer->used += size;
        return(size);
    }

    /* Fill the buffer, better to flush */
    memcpy(writer->blob + writer->used, pblob, avail);
    writer->used += avail;
    pblob += avail;
    size -= avail;
    if ((wr = io_write_fully(writer->stream, writer->blob, writer->size)) != writer->size)
        return(avail - (writer->size - wr));

    /* Flush directly if input is greater than buffer */
    n = avail;
    while (size >= writer->size) {
        if ((wr = io_write_fully(writer->stream, pblob, writer->size)) != writer->size)
            return(n + wr);

        pblob += writer->size;
        size -= writer->size;
        n += writer->size;
    }

    /* Store remaining in buffer */
    writer->used = size;
    memcpy(writer->blob, pblob, size);
    return(n + size);
}

static int __buffered_flush (stream_t *stream) {
    buffered_writer_t *writer = (buffered_writer_t *)stream;
    if (writer->used > 0) {
        int wr;
        wr = io_write_fully(writer->stream, writer->blob, writer->used);
        writer->used = 0;
        return(wr);
    }
    return(0);
}

uint64_t __buffered_position (stream_t *stream) {
    buffered_writer_t *writer = (buffered_writer_t *)stream;
    return(io_position(writer->stream) + writer->used);
}

uint64_t __buffered_length (stream_t *stream) {
    buffered_writer_t *writer = (buffered_writer_t *)stream;
    return(io_length(writer->stream) + writer->used);
}

static stream_vtable_t __buffered_writer = {
    .write     = __buffered_write,
    .flush     = __buffered_flush,
    .zread     = NULL,
    .read      = NULL,
    .seek      = NULL,

    .can_write = NULL,
    .can_zread = NULL,
    .can_read  = NULL,
    .can_seek  = NULL,

    .position  = __buffered_position,
    .length    = __buffered_length,
};

int buffered_writer_open (buffered_writer_t *writer,
                          stream_t *stream,
                          unsigned int size)
{
    writer->__base_type__.vtable = &__buffered_writer;
    writer->stream = stream;
    writer->blob = NULL;
    writer->size = size;
    writer->used = 0U;
    return(0);
}

void buffered_writer_close (buffered_writer_t *writer) {
    if (writer->blob != NULL) {
        free(writer->blob);
        writer->blob = NULL;
    }
    writer->size = 0;
    writer->used = 0;
}

/* ============================================================================
 *  Buffered Reader
 */
static int __buffered_read (stream_t *stream, void *blob, unsigned int size) {
    buffered_reader_t *reader = (buffered_reader_t *)stream;
    unsigned char *pblob = (unsigned char *)blob;
    unsigned int avail = reader->size - reader->used;
    int n, rd;

    /* Direct read if requested size is largen than buffer and no buf */
    if (!avail && size >= reader->reqs)
        return(io_read_fully(reader->stream, blob, size));

    /* There's all the data you need! */
    if (size <= avail) {
        memcpy(pblob, reader->blob + reader->used, size);
        reader->used += size;
        return(size);
    }

    if (reader->blob == NULL) {
        if ((reader->blob = (unsigned char *) malloc(reader->reqs)) == NULL)
            return(0);
    }

    /* Copy the old buffer, to the end */
    if ((n = avail) > 0) {
        memcpy(pblob, reader->blob + reader->used, avail);
        pblob += avail;
        size -= avail;
    }

    /* Direct read if requested size is larger than buffer */
    if (size >= reader->reqs) {
        rd = io_read_fully(reader->stream, pblob, size);
        reader->used = 0;
        reader->size = 0;
        return((rd > 0) ? (n + rd) : n);
    }

    /* Fill the buffer */
    if ((rd = io_read_fully(reader->stream, reader->blob, reader->reqs)) <= 0)
        return(n);

    /* Copy to user */
    memcpy(pblob, reader->blob, size);
    reader->used = size;
    reader->size = rd;
    return(n + size);
}

static stream_vtable_t __buffered_reader = {
    .write     = NULL,
    .flush     = NULL,
    .zread     = NULL,
    .read      = __buffered_read,
    .seek      = NULL,

    .can_write = NULL,
    .can_zread = NULL,
    .can_read  = NULL,
    .can_seek  = NULL,

    .position  = __buffered_position,
    .length    = __buffered_length,
};

int buffered_reader_open (buffered_reader_t *reader,
                          stream_t *stream,
                          unsigned int size)
{
    reader->__base_type__.vtable = &__buffered_reader;
    reader->stream = stream;
    reader->blob = NULL;
    reader->size = 0U;
    reader->used = 0U;
    reader->reqs = size;
    return(0);
}

void buffered_reader_close (buffered_reader_t *reader) {
    if (reader->blob != NULL) {
        free(reader->blob);
        reader->blob = NULL;
    }
    reader->size = 0U;
    reader->used = 0U;
    reader->reqs = 0U;
}

