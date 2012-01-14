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

#include <stdlib.h>
#include <string.h>

#include "Writable.h"

/* ============================================================================
 *  Writable
 */
int Writable::writeFully (const void *buffer, unsigned int size) {
    const uint8_t *pbuf = (const uint8_t *)buffer;
    int n = 0;
    int wr;

    while (size > 0) {
        if ((wr = write(pbuf, size)) <= 0)
            break;

        n += wr;
        pbuf += wr;
        size -= wr;
    }

    return(n);
}

int Writable::writeUInt8 (uint8_t value) {
    return(writeFully(&value, 1));
}

int Writable::writeUInt16 (uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xff;
    buffer[1] = (value >> 0) & 0xff;
    return(writeFully(buffer, 2));
}
int Writable::writeUInt32 (uint32_t value) {
    uint8_t buffer[4];
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) & 0xff;
    buffer[3] = (value >> 0) & 0xff;
    return(writeFully(buffer, 4));
}

int Writable::writeUInt64 (uint64_t value) {
    uint8_t buffer[8];
    buffer[0] = (value >> 56) & 0xff;
    buffer[1] = (value >> 48) & 0xff;
    buffer[2] = (value >> 40) & 0xff;
    buffer[3] = (value >> 32) & 0xff;
    buffer[4] = (value >> 24) & 0xff;
    buffer[5] = (value >> 16) & 0xff;
    buffer[6] = (value >> 8) & 0xff;
    buffer[7] = (value >> 0) & 0xff;
    return(writeFully(buffer, 8));
}

int Writable::writeVUInt (uint64_t value) {
    unsigned int length = 0;
    uint8_t buffer[10];

    while (value >= 128) {
        buffer[length++] = (value & 0x7f) | 128;
        value >>= 7;
    }
    buffer[length++] = value & 0xff;

    return(writeFully(buffer, length));
}

int Writable::writeInt8 (int8_t value) {
    return(writeUInt8((uint8_t)value));
}

int Writable::writeInt16 (int16_t value) {
    return(writeUInt16((uint16_t)value));
}

int Writable::writeInt32 (int32_t value) {
    return(writeUInt32((uint32_t)value));
}

int Writable::writeInt64 (int64_t value) {
    return(writeUInt64((uint64_t)value));
}

int Writable::writeVInt (int64_t value) {
    uint64_t v = (value << 1) ^ (value >> 63);
    return(writeVUInt(v));
}

