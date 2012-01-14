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

#include "CompressedWriter.h"
#include "CompressedReader.h"
#include "DiskWriter.h"
#include "DiskReader.h"

#include <unistd.h>
#include <stdio.h>

int testWrite (const char *filename) {
    DiskWriter disk_writer;

    if (!disk_writer.open(filename, true))
        return(1);

    Lz4Writer lz4_writer(&disk_writer, 512);
    lz4_writer.write("Hello World", 11);
    lz4_writer.write("Hello World", 11);
    lz4_writer.write("Hello World", 11);
    lz4_writer.write("Hello World", 11);
    lz4_writer.flush();

    disk_writer.close();
    return(0);
}

int testRead (const char *filename) {
    DiskReader disk_reader;
    char buffer[12];
    int n;

    if (!disk_reader.open(filename))
        return(1);

    Lz4Reader lz4_reader(&disk_reader);
    while ((n = lz4_reader.read(buffer, 11)) > 0) {
        buffer[n] = 0;
        printf("READED %d '%s'\n", n, buffer);
    }

    disk_reader.close();
    return(0);
}

int main (int argc, char **argv) {
    const char *filename = "io-compressed.disk";

    testWrite(filename);
    testRead(filename);

    unlink(filename);
    return(0);
}

