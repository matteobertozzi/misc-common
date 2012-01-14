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
#include <unistd.h>
#include <stdio.h>

#include "BufferedWriter.h"
#include "BufferedReader.h"
#include "DiskWriter.h"
#include "DiskReader.h"

static int testWrite (const char *filename) {
    DiskWriter disk_writer;

    if (!disk_writer.open(filename, true))
        return(1);

    BufferedWriter buffered_writer(&disk_writer, 30);
    for (unsigned int i = 0; i < 100; ++i) {
        buffered_writer.writeInt8(i);
        buffered_writer.writeInt16(i);
        buffered_writer.writeInt32(i);
        buffered_writer.writeInt64(i);
        buffered_writer.writeVInt(i);
    }
    buffered_writer.flush();

    disk_writer.close();
    return(0);
}

static int testRead (const char *filename) {
    DiskReader disk_reader;
    int64_t i64, v64;
    int32_t i32;
    int16_t i16;
    int8_t i8;

    if (!disk_reader.open(filename))
        return(1);

    BufferedReader bufferd_reader(&disk_reader, 30);
    while (1) {
        if (bufferd_reader.readInt8(&i8) <= 0)
            break;

        bufferd_reader.readInt16(&i16);
        bufferd_reader.readInt32(&i32);
        bufferd_reader.readInt64(&i64);
        bufferd_reader.readVInt(&v64);
        printf("READED %d %d %d %ld %ld\n", i8, i16, i32, i64, v64);
    }

    disk_reader.close();
    return(0);
}

int main (int argc, char **argv) {
    const char *filename = "io-buffered.disk";
    testWrite(filename);
    testRead(filename);
    unlink(filename);
    return(0);
}

