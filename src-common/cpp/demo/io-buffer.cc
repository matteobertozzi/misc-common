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

#include <stdint.h>
#include <stdio.h>
#include "Buffer.h"

void test0 (void) {
    unsigned int i;
    char blob[20];
    Buffer buf;

    buf.append("Hello World", 11);
    buf.prepend("this ", 5);
    buf[0] = 'T';

    for (i = 0; i < buf.size(); ++i)
        printf("%c", buf[i]);
    printf("\n");

    BufferWriter bwriter(&buf);
    bwriter.write(" - Test!", 8);
    bwriter.write(" - Peng!", 8);

    for (i = 0; i < buf.size(); ++i)
        printf("%c", buf[i]);
    printf("\n");

    BufferReader breader(&buf);
    i = breader.read(blob, 11);
    printf("readed %d/11 - ", i);
    for (i = 0; i < 11; ++i)
        printf("%c", blob[i]);
    printf("\n");

    i = breader.read(blob, 13);
    printf("readed %d/13 - ", i);
    for (i = 0; i < 13; ++i)
        printf("%c", blob[i]);
    printf("\n");

    i = breader.read(blob, 20);
    printf("readed %d/20 - ", i);
    for (i = 0; i < 8; ++i)
        printf("%c", blob[i]);
    printf("\n");
}

void test1 (void) {
    uint64_t vu64;
    uint64_t u64;
    uint32_t u32;
    uint16_t u16;
    int64_t vi64;
    int64_t i64;
    int32_t i32;
    int16_t i16;
    uint8_t u8;
    int8_t i8;

    Buffer buf;

    BufferWriter writer(&buf);
    writer.writeInt8(-((1 << 7) - 1));
    writer.writeUInt8((1 << 8) - 1);
    writer.writeInt16(-((1 << 15) - 1));
    writer.writeUInt16((1 << 16) - 1);
    writer.writeInt32(-(((uint32_t)(1UL << 31)) - 1));
    writer.writeUInt32(((1UL << 31) - 1) + (1UL << 31));
    writer.writeInt64(-((1ULL << 63) - 1));
    writer.writeUInt64((1ULL << 63) + (1ULL << 63) - 1);
    writer.writeVInt(-((1ULL << 63) - 1));
    writer.writeVUInt((1ULL << 63) + (1ULL << 63)- 1);

    BufferReader reader(&buf);
    reader.readInt8(&i8);
    reader.readUInt8(&u8);
    reader.readInt16(&i16);
    reader.readUInt16(&u16);
    reader.readInt32(&i32);
    reader.readUInt32(&u32);
    reader.readInt64(&i64);
    reader.readUInt64(&u64);
    reader.readVInt(&vi64);
    reader.readVUInt(&vu64);

    printf("i8   %-12d ", i8);
    printf("u8   %-12u\n", u8);
    printf("i16  %-12d ", i16);
    printf("u16  %-12u\n", u16);
    printf("i32  %-12d ", i32);
    printf("u32  %-12u\n", u32);
    printf("i64  %-12ld ", i64);
    printf("u64  %-12lu\n", u64);
    printf("vi64 %-12ld ", i64);
    printf("vu64 %-12lu\n", u64);
}

int main (int argc, char **argv) {
    test0();
    test1();
    return(0);
}

