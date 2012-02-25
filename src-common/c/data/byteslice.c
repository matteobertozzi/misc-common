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

#include "byteslice.h"

int memncmp (const void *a,
             unsigned int asize,
             const void *b,
             unsigned int bsize)
{
    unsigned int min_len;
    int cmp;

    min_len = ((asize) < (bsize)) ? (asize) : (bsize);
    if ((cmp = memcmp(a, b, min_len)))
        return(cmp);

    return((asize) - (bsize));
}

/* ============================================================================
 *  Byte Slice
 */
static unsigned int __byte_slice_length (const slice_t *slice) {
    return(((const byte_slice_t *)slice)->size);
}

static int __byte_slice_compare (const slice_t *a, const slice_t *b) {
    const byte_slice_t *ba = (const byte_slice_t *)a;
    const byte_slice_t *bb = (const byte_slice_t *)b;
    return(memncmp(ba->data, ba->size, bb->data, bb->size));
}

static int __byte_slice_copy (const slice_t *slice,
                              void *buffer,
                              unsigned int offset,
                              unsigned int length)
{
    const byte_slice_t *bs = (const byte_slice_t *)slice;
    unsigned int n;

    if (offset > bs->size)
        return(0);

    if ((n = (bs->size - offset)) > length)
        n = length;

    memcpy(buffer, bs->data + offset, n);
    return(n);
}

static int __byte_slice_write (const slice_t *slice,
                               stream_t *stream)
{
    const byte_slice_t *bs = (const byte_slice_t *)slice;
    return(io_write_fully(stream, bs->data, bs->size));
}

static slice_vtable_t __byte_slice = {
    .length  = __byte_slice_length,
    .compare = __byte_slice_compare,
    .copy    = __byte_slice_copy,
    .write   = __byte_slice_write,
};

int byte_slice_open (byte_slice_t *slice,
                     const void *data,
                     unsigned int size)
{
    slice->__base_type__.vtable = &__byte_slice;
    slice->data = data;
    slice->size = size;
    return(0);
}

void byte_slice_close (byte_slice_t *slice) {
    slice->data = NULL;
    slice->size = 0;
}

/* ============================================================================
 *  Prefix (Byte) Slice
 */
static unsigned int __prefix_slice_length (const slice_t *slice) {
    const prefix_slice_t *ps = (const prefix_slice_t *)slice;
    return(ps->prefix_size + ps->data_size);
}

static int __prefix_slice_compare (const slice_t *a, const slice_t *b) {
    const prefix_slice_t *ba = (const prefix_slice_t *)a;
    const prefix_slice_t *bb = (const prefix_slice_t *)b;

    if (ba->prefix_size != bb->prefix_size) {
        return(memncmp(ba->prefix, ba->prefix_size,
                       bb->prefix, bb->prefix_size));
    }

    return(memncmp(ba->data, ba->data_size, bb->data, bb->data_size));
}

static int __prefix_slice_copy (const slice_t *slice,
                                void *buffer,
                                unsigned int offset,
                                unsigned int length)
{
    const prefix_slice_t *ps = (const prefix_slice_t *)slice;
    unsigned char *pbuf = (unsigned char *)buffer;
    unsigned int n;

    if (offset > (ps->prefix_size + ps->data_size))
        return(0);

    if (offset < ps->prefix_size) {
        if ((n = ps->prefix_size - offset) > length) {
            memcpy(pbuf, ps->prefix, length);
            return(length);
        }

        memcpy(pbuf, ps->prefix + offset, n);
        length -= n;
        offset = 0;
        pbuf += n;
    }

    n = ps->data_size - offset;
    n = (n < length) ? n : length;
    memcpy(pbuf, ps->data + offset, n);
    return((pbuf + n) - ((unsigned char *)buffer));
}

static int __prefix_slice_write (const slice_t *slice,
                                 stream_t *stream)
{
    const prefix_slice_t *ps = (const prefix_slice_t *)slice;
    unsigned int np, nd;

    np = io_write_fully(stream, ps->prefix, ps->prefix_size);
    if (np != ps->prefix_size)
        return(np);

    nd = io_write_fully(stream, ps->data, ps->data_size);
    return(np + nd);
}


static slice_vtable_t __prefix_slice = {
    .length  = __prefix_slice_length,
    .compare = __prefix_slice_compare,
    .copy    = __prefix_slice_copy,
    .write   = __prefix_slice_write,
};

int prefix_slice_open (prefix_slice_t *slice,
                       const void *prefix,
                       unsigned int prefix_size,
                       const void *data,
                       unsigned int data_size)
{
    slice->__base_type__.vtable = &__prefix_slice;
    slice->prefix = prefix;
    slice->data = data;
    slice->prefix_size = prefix_size;
    slice->data_size = data_size;
    return(0);
}

void prefix_slice_close (prefix_slice_t *slice) {
    slice->prefix = NULL;
    slice->data = NULL;
    slice->prefix_size = 0;
    slice->data_size = 0;
}

