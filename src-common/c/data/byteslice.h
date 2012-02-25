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

#ifndef _BYTE_SLICE_H_
#define _BYTE_SLICE_H_

#include "slice.h"

typedef struct prefix_slice prefix_slice_t;
typedef struct byte_slice byte_slice_t;

struct byte_slice {
    slice_t __base_type__;
    const uint8_t *data;
    unsigned int size;
};

struct prefix_slice {
    slice_t __base_type__;
    const uint8_t *prefix;
    const uint8_t *data;
    unsigned int prefix_size;
    unsigned int data_size;
};

int     byte_slice_open         (byte_slice_t *slice,
                                 const void *data,
                                 unsigned int size);
void    byte_slice_close        (byte_slice_t *slice);

int     prefix_slice_open       (prefix_slice_t *slice,
                                 const void *prefix,
                                 unsigned int prefix_size,
                                 const void *data,
                                 unsigned int data_size);
void    prefix_slice_close      (prefix_slice_t *slice);


#endif /* _BYTE_SLICE_H_ */

