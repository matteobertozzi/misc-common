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

#include "codec.h"

/* ============================================================================
 *  Lz4 Codec (http://code.google.com/p/lz4/)
 */
int LZ4_compress (const char *src, char *dst, int isize);
int LZ4_uncompress (const char *src, char *dst, int osize);

static int __lz4_encode (codec_t *obj,
                         void *dst,
                         unsigned int dst_size,
                         const void *src,
                         unsigned int src_size)
{
    return(LZ4_compress((const char *)src, (char *)dst, src_size));
}

static int __lz4_decode (codec_t *obj,
                         void *dst,
                         unsigned int dst_size,
                         const void *src,
                         unsigned int src_size)
{
    return(LZ4_uncompress((const char*)src, (char*)dst, dst_size) != src_size);
}

static int __lz4_max_length (codec_t *obj, unsigned int size) {
    int extra = 1 + ((size << 2) / 100);
    return(size + ((extra + 7) & (-8)));
}

codec_vtable_t codec_lz4 = {
    .encode     = __lz4_encode,
    .decode     = __lz4_decode,
    .max_length = __lz4_max_length,
};


/* ============================================================================
 *  AES Codec
 */
#include "codec/aes/aes.h"

static int __aes_encode (codec_t *obj,
                         void *dst,
                         unsigned int dst_size,
                         const void *src,
                         unsigned int src_size)
{
    aes_t *aes = (aes_t *)obj->data.ptr;
    return(aes_encrypt(aes, dst, src, src_size));
}

static int __aes_decode (codec_t *obj,
                         void *dst,
                         unsigned int dst_size,
                         const void *src,
                         unsigned int src_size)
{
    aes_t *aes = (aes_t *)obj->data.ptr;
    return(aes_decrypt(aes, dst, src, src_size) != dst_size);
}

static int __aes_max_length (codec_t *obj, unsigned int size) {
    return(size + 16);
}

codec_vtable_t codec_aes = {
    .encode     = __aes_encode,
    .decode     = __aes_decode,
    .max_length = __aes_max_length,
};

