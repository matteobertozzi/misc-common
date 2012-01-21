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

#include "encoded.h"

/* ============================================================================
 *  Encoded Writer (Encoder)
 */
static int __encoded_write_block (encoded_writer_t *buffer,
                                  const void *blob,
                                  unsigned int size)
{
    unsigned char *cbuf;
    int csize;

    csize = codec_max_length(buffer->codec, size);
    if ((cbuf = (unsigned char *) malloc(csize)) == NULL)
        return(-1);

    csize = codec_encode(buffer->codec, cbuf, csize, blob, size);
    if (io_write_vuint(buffer->stream, size) <= 0)
        return(-2);

    if (io_write_vuint(buffer->stream, csize) <= 0)
        return(-2);
        
    if (io_write_fully(buffer->stream, cbuf, csize) <= 0)
        return(-3);

    free(cbuf);
    return(size);
}

static int __encoded_write (stream_t *stream,
                            const void *blob,
                            unsigned int size)
{
    encoded_writer_t *writer = (encoded_writer_t *)stream;
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
    if ((wr = __encoded_write_block(writer, writer->blob, writer->size)) != writer->size)
        return(avail - (writer->size - wr));

    /* Flush directly if input is greater than buffer */
    n = avail;
    while (size >= writer->size) {
        if ((wr = __encoded_write_block(writer, pblob, writer->size)) != writer->size)
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

static int __encoded_flush (stream_t *stream) {
    encoded_writer_t *writer = (encoded_writer_t *)stream;
    if (writer->used > 0) {
        int wr;
        wr = __encoded_write_block(writer, writer->blob, writer->used);
        writer->used = 0;
        return(wr);
    }
    return(0);
}

static stream_vtable_t __encoded_writer = {
    .write     = __encoded_write,
    .flush     = __encoded_flush,
    .zread     = NULL,
    .read      = NULL,
    .seek      = NULL,

    .can_write = NULL,
    .can_zread = NULL,
    .can_read  = NULL,
    .can_seek  = NULL,

    .position  = NULL,
    .length    = NULL,
};


int encoded_writer_open (encoded_writer_t *writer,
                         codec_t *codec,
                         stream_t *stream,
                         unsigned int size)
{
    writer->__base_type__.vtable = &__encoded_writer;
    writer->stream = stream;
    writer->codec = codec;
    writer->blob = NULL;
    writer->size = size;
    writer->used = 0U;
    return(0);
}

void encoded_writer_close (encoded_writer_t *writer) {
    if (writer->blob != NULL) {
        free(writer->blob);
        writer->blob = NULL;
    }
    writer->size = 0;
    writer->used = 0;
}

/* ============================================================================
 *  Encoded Reader (Decoder)
 */
static int __encoded_read_block (encoded_reader_t *buffer) {
    unsigned char *cbuf;
    uint64_t csize;
    uint64_t size;
    
    /* Read header */
    if (io_read_vuint(buffer->stream, &size) <= 0) 
        return(-1);

    if (io_read_vuint(buffer->stream, &csize) <= 0)
        return(-2);

    /* Alloc compressed buffer */
    if ((cbuf = (unsigned char *) malloc(csize)) == NULL)
        return(-3);

    /* Read uncompressed data */
    if (io_read_fully(buffer->stream, cbuf, csize) != (int)csize) {
        free(cbuf);
        return(-4);
    }

    /* Prepare new buffer for data */
    if (buffer->size != size) {
        if (buffer->blob != NULL)
            free(buffer->blob);

        if ((buffer->blob = (unsigned char *) malloc(size)) == NULL) {
            free(cbuf);
            return(-5);
        }

        buffer->size = size;
        buffer->used = 0;
    }

    /* Transform */
    if (codec_decode(buffer->codec, buffer->blob, size, cbuf, csize)) {
        free(cbuf);
        return(-6);
    }

    /* Free compressed buffer */
    free(cbuf);
    return(0);
}

static int __encoded_read (stream_t *stream, void *blob, unsigned int size) {
    encoded_reader_t *buffer = (encoded_reader_t *)stream;
    unsigned int avail = buffer->size - buffer->used;
    unsigned char *pblob = (unsigned char *)blob;

    if (size <= avail) {
        memcpy(blob, buffer->blob + buffer->used, size);
        buffer->used += size;
        return(size);
    }

    /* Copy the old buffer to the end. */
    if (avail > 0) {
        memcpy(pblob, buffer->blob + buffer->used, avail);
        pblob += avail;
        size -= avail;
    }

    do {
        if (__encoded_read_block(buffer) < 0)
            break;

        /* Copy to user */
        avail = (size > buffer->size) ? buffer->size : size;
        memcpy(pblob, buffer->blob, avail);
        buffer->used = avail;
        pblob += avail;
        size -= avail;
    } while (size > 0);

    return(pblob - (unsigned char *)blob);
}

static stream_vtable_t __encoded_reader = {
    .write     = NULL,
    .flush     = NULL,
    .zread     = NULL,
    .read      = __encoded_read,
    .seek      = NULL,

    .can_write = NULL,
    .can_zread = NULL,
    .can_read  = NULL,
    .can_seek  = NULL,

    .position  = NULL,
    .length    = NULL,
};

int encoded_reader_open (encoded_reader_t *reader,
                         codec_t *codec,
                         stream_t *stream)
{
    reader->__base_type__.vtable = &__encoded_reader;
    reader->codec = codec;
    reader->stream = stream;
    reader->blob = NULL;
    reader->size = 0;
    reader->used = 0;
    return(0);
}

void encoded_reader_close (encoded_reader_t *reader) {
    if (reader->blob != NULL) {
        free(reader->blob);
        reader->blob = NULL;
    }
    reader->used = 0;
    reader->size = 0;
}

