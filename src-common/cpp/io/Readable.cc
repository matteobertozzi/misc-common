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

#include "Readable.h"

/* ============================================================================
 *  Readable
 */
int Readable::readFully (void *buffer, unsigned int size) {
    uint8_t *pbuf = (uint8_t *)buffer;
    int n = 0;
    int rd;

    while (size > 0) {
        if ((rd = read(pbuf, size)) <= 0)
            break;

        n += rd;
        pbuf += rd;
        size -= rd;
    }

    return(n);
}

int Readable::readUInt8 (uint8_t *value) {
    return(read(value, 1));
}

int Readable::readUInt16 (uint16_t *value) {
    uint8_t buffer[2];
    int rd;

    if ((rd = readFully(buffer, 2)) != 2)
        return(rd);

    *value = ((uint16_t)(buffer[0]) <<  8) +
             ((uint16_t)(buffer[1]) <<  0);

    return(rd);
}

int Readable::readUInt32 (uint32_t *value) {
    uint8_t buffer[4];
    int rd;

    if ((rd = readFully(buffer, 4)) != 4)
        return(rd);

    *value = ((uint32_t)(buffer[0]) << 24) +
             ((uint32_t)(buffer[1]) << 16) +
             ((uint32_t)(buffer[2]) <<  8) +
             ((uint32_t)(buffer[3]) <<  0);

    return(rd);
}

int Readable::readUInt64 (uint64_t *value) {
    uint8_t buffer[8];
    int rd;

    if ((rd = readFully(buffer, 8)) != 8)
        return(rd);

    *value = ((uint64_t)(buffer[0]) << 56) +
             ((uint64_t)(buffer[1]) << 48) +
             ((uint64_t)(buffer[2]) << 40) +
             ((uint64_t)(buffer[3]) << 32) +
             ((uint64_t)(buffer[4]) << 24) +
             ((uint64_t)(buffer[5]) << 16) +
             ((uint64_t)(buffer[6]) <<  8) +
             ((uint64_t)(buffer[7]) <<  0);

    return(rd);
}

int Readable::readVUInt (uint64_t *value) {
    uint64_t result = 0;
    unsigned int shift;
    uint8_t buffer;
    int rd;

    for (shift = 0; shift < 64; shift += 7) {
        if (read(&buffer, 1) != 1)
            break;

        rd++;
        if (buffer & 128) {
            result |= ((buffer & 0x7f) << shift);
        } else {
            result |= (buffer << shift);
            break;
        }
    }

    *value = result;
    return(rd);
}

int Readable::readInt8 (int8_t *value) {
    return(readUInt8((uint8_t *)value));
}

int Readable::readInt16 (int16_t *value) {
    return(readUInt16((uint16_t *)value));
}

int Readable::readInt32 (int32_t *value) {
    return(readUInt32((uint32_t *)value));
}

int Readable::readInt64 (int64_t *value) {
    return(readUInt64((uint64_t *)value));
}

int Readable::readVInt (int64_t *value) {
    uint64_t v;
    int rd;

    rd = readVUInt(&v);
    *value = (int64_t)((v >> 1) ^ -(v & 1));

    return(rd);
}

