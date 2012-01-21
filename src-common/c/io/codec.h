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

#ifndef _CODEC_H_
#define _CODEC_H_

#include <stdint.h>

typedef const struct codec_vtable codec_vtable_t;
typedef struct codec codec_t;

struct codec_vtable {
    int (*encode)     (codec_t *codec,
                       void *dst,
                       unsigned int dst_size,
                       const void *src,
                       unsigned int src_size);
    int (*decode)     (codec_t *codec,
                       void *dst,
                       unsigned int dst_size,
                       const void *src,
                       unsigned int src_size);
    int (*max_length) (codec_t *codec,
                       unsigned int size);
};

struct codec {
    codec_vtable_t *vtable;
    union {
        void *ptr;
        int fd;
        uint32_t u32;
        uint64_t u64;
    } data;
};

extern const codec_vtable_t codec_lz4;
extern const codec_vtable_t codec_aes;

#define codec_encode(codec, dst, dst_size, src, src_size)               \
    (codec)->vtable->encode(codec, dst, dst_size, src, src_size)

#define codec_decode(codec, dst, dst_size, src, src_size)               \
    (codec)->vtable->decode(codec, dst, dst_size, src, src_size)

#define codec_max_length(codec, size)                                   \
    (codec)->vtable->max_length(codec, size)

#endif /* _CODEC_H_ */

