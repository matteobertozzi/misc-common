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

#ifndef _READABLE_H_
#define _READABLE_H_

#include <stdint.h>

class Readable {
    public:
        virtual int read (void *buf, unsigned int size) = 0;

        int readFully  (void *buffer, unsigned int size);

        int readInt8   (int8_t *value);
        int readInt16  (int16_t *value);
        int readInt32  (int32_t *value);
        int readInt64  (int64_t *value);
        int readVInt   (int64_t *value);

        int readUInt8  (uint8_t *value);
        int readUInt16 (uint16_t *value);
        int readUInt32 (uint32_t *value);
        int readUInt64 (uint64_t *value);
        int readVUInt  (uint64_t *value);
};

#endif /* !_READABLE_H_ */
