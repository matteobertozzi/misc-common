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

#ifndef _SLICE_H_
#define _SLICE_H_

#include "stream.h"

typedef const struct slice_vtable slice_vtable_t;
typedef struct slice slice_t;

struct slice_vtable {
    unsigned int (*length)   (const slice_t *slice);
    int          (*compare)  (const slice_t *a,
                              const slice_t *b);

    int          (*copy)     (const slice_t *slice,
                              void *buffer,
                              unsigned int offset,
                              unsigned int length);
    int          (*write)    (const slice_t *slice,
                              stream_t *stream);
};

struct slice {
    slice_vtable_t *vtable;
};

#define slice_vtable(slice)         (((slice_t *)(slice))->vtable)

#define slice_call(slice, method, ...)                                 \
    slice_vtable(slice)->method((slice_t *)slice, ##__VA_ARGS__)

#define slice_has_method(slice, method)                                \
    (slice_vtable(slice)->method != NULL)

#define slice_is_empty(slice)       (slice_call(slice, length) == 0)
#define slice_length(slice)         slice_call(slice, length)
#define slice_compare(a, b)         slice_call(slice, compare, a, b)

#define slice_copy(slice, buffer, offset, length)                       \
    slice_call(slice, copy, buffer, offset, length)

#define slice_write(slice, stream)                                      \
    slice_call(slice, write, stream)

#endif /* !_SLICE_H_ */

