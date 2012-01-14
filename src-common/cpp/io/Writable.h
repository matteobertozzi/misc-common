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

#ifndef _WRITEABLE_H_
#define _WRITEABLE_H_

#include <stdint.h>

class Writable {
    public:
        virtual int write (const void *buf, unsigned int size) = 0;
        virtual int flush (void) = 0;

        int writeFully   (const void *buffer, unsigned int size);

        int writeInt8   (int8_t value);
        int writeInt16  (int16_t value);
        int writeInt32  (int32_t value);
        int writeInt64  (int64_t value);
        int writeVInt   (int64_t value);

        int writeUInt8  (uint8_t value);
        int writeUInt16 (uint16_t value);
        int writeUInt32 (uint32_t value);
        int writeUInt64 (uint64_t value);
        int writeVUInt  (uint64_t value);
};

#endif /* !_WRITEABLE_H_ */
