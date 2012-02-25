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
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

#include "byteslice.h"
#include "buffered.h"
#include "encoded.h"
#include "stream.h"
#include "disk.h"
#include "aes.h"

/* ============================================================================
 *  I/O Tests
 */
int __print_write (stream_t *stream, const void *buf, unsigned int n) {
    const char *pbuf = (const char *)buf;
    unsigned int i;
    printf("%2d: ", n);
    for (i = 0; i < n; ++i) {
        if (isprint(*pbuf))
            putchar(*pbuf);
        else
            printf("[0x%x (%d)] ", *pbuf, *pbuf);
        pbuf++;
    }
    putchar('\n');
    return(n);
}

static const stream_vtable_t __print_writer = {
    .write     = __print_write,
    .flush     = NULL,
    .zread     = NULL,
    .read      = NULL,
    .seek      = NULL,

    .can_write = NULL,
    .can_zread = NULL,
    .can_read  = NULL,
    .can_seek  = NULL,

    .position  = NULL,
    .length    = NULL,
};

static void __test0 (void) {
    buffered_writer_t buffer_stream;
    stream_t print_stream = { &__print_writer };
    stream_t *stream = (stream_t *)&buffer_stream;

    printf("================================================\n");
    printf("TEST 0 - Buffered Writer\n");
    buffered_writer_open(&buffer_stream, &print_stream, 4);

    io_write(stream, "Hello World", 11);
    io_write(stream, "Hello World", 11);
    io_write_uint8(stream, 'A');
    io_write_uint16(stream, 0x4142);
    io_write_uint32(stream, 0x41424344);
    io_write_uint64(stream, 0x4142434445464748);

    io_flush(stream);
    buffered_writer_close(&buffer_stream);
}

static void __test1 (void) {
    encoded_writer_t encoded_writer;
    stream_t print_stream = { &__print_writer };
    stream_t *stream = (stream_t *)&encoded_writer;
    codec_t lz4_codec;

    lz4_codec.vtable = &codec_lz4;

    printf("================================================\n");
    printf("TEST 1 - Encoded Writer\n");
    encoded_writer_open(&encoded_writer, &lz4_codec, &print_stream, 11);
    stream = (stream_t *)&encoded_writer;

    io_write(stream, "Hello World", 11);
    io_write(stream, "Hello World", 11);
    io_write_uint8(stream, 'A');
    io_write_uint16(stream, 0x4142);
    io_write_uint32(stream, 0x41424344);
    io_write_uint64(stream, 0x4142434445464748);

    io_flush(stream);
    encoded_writer_close(&encoded_writer);
}

static void __test2 (void) {
    char buf[32];
    disk_stream_t disk;
    stream_t *stream = (stream_t *)&disk;

    printf("================================================\n");
    printf("TEST 2 - Disk Stream\n");

    disk_stream_create(&disk, "test2.data", O_CREAT | O_TRUNC | O_RDWR, 0644);
    io_write(stream, "Hello World", 11);
    io_write_uint64(stream, 0x4142434445464748);
    io_seek(stream, 0);
    io_read(stream, buf, 11 + 8);
    buf[11 + 8] = 0;
    printf("BUF: %s\n", buf);
    printf("FILE SIZE %llu\n", io_length(stream));
    disk_stream_close(&disk);
}

static void __test3 (void) {
    buffered_writer_t buffered_writer;
    buffered_reader_t buffered_reader;
    disk_stream_t disk;
    stream_t *stream;
    char buffer[32];
    int n;

    printf("================================================\n");
    printf("TEST 3 - Disk Stream + Buffered Writer/Reader\n");

    disk_stream_create(&disk, "test3.data", O_CREAT | O_TRUNC | O_RDWR, 0644);

    /* Write something */
    buffered_writer_open(&buffered_writer, (stream_t *)&disk, 11);
    stream = (stream_t *)&buffered_writer;

    io_write(stream, "Hello World", 11);
    io_write(stream, "Hello World", 11);
    io_write_uint8(stream, 'A');
    io_write_uint16(stream, 0x4142);
    io_write_uint32(stream, 0x41424344);
    io_write_uint64(stream, 0x4142434445464748);
    io_flush(stream);

    buffered_writer_close(&buffered_writer);

    /* Read something */
    io_seek((stream_t *)&disk, 0);

    buffered_reader_open(&buffered_reader, (stream_t *)&disk, 8);
    stream = (stream_t *)&buffered_reader;

    n = io_read(stream, buffer, 22);
    buffer[n] = 0;
    printf("read %d %s\n", n, buffer);

    n = io_read(stream, buffer, 32);
    buffer[n] = 0;
    printf("read %d %s\n", n, buffer);

    buffered_reader_close(&buffered_reader);

    disk_stream_close(&disk);
}

static void __test_rwencoded (const char *filename, codec_t *codec) {
    encoded_writer_t encoded_writer;
    encoded_reader_t encoded_reader;
    disk_stream_t disk;
    stream_t *stream;
    char buffer[32];
    int n;

    disk_stream_create(&disk, filename, O_CREAT | O_TRUNC | O_RDWR, 0644);

    /* Write something */
    encoded_writer_open(&encoded_writer, codec, (stream_t *)&disk, 11);
    stream = (stream_t *)&encoded_writer;

    io_write(stream, "Hello World", 11);
    io_write(stream, "Hello World", 11);
    io_write_uint8(stream, 'A');
    io_write_uint16(stream, 0x4142);
    io_write_uint32(stream, 0x41424344);
    io_write_uint64(stream, 0x4142434445464748);
    io_flush(stream);

    encoded_writer_close(&encoded_writer);

    /* Read something */
    io_seek((stream_t *)&disk, 0);

    encoded_reader_open(&encoded_reader, codec, (stream_t *)&disk);
    stream = (stream_t *)&encoded_reader;

    n = io_read(stream, buffer, 22);
    buffer[n] = 0;
    printf("read %d %s\n", n, buffer);

    n = io_read(stream, buffer, 32);
    buffer[n] = 0;
    printf("read %d %s\n", n, buffer);

    encoded_reader_close(&encoded_reader);

    disk_stream_close(&disk);
}

static void __test4 (void) {
    codec_t codec;
    codec.vtable = &codec_lz4;

    printf("================================================\n");
    printf("TEST 4 - Disk Stream + LZ4 Encoded Writer/Reader\n");
    __test_rwencoded("test4.data", &codec);
}

static void __test5 (void) {
    codec_t codec;
    aes_t aes;

    printf("================================================\n");
    printf("TEST 5 - Disk Stream + AES Encoded Writer/Reader\n");

    codec.vtable = &codec_aes;
    codec.data.ptr = &aes;

    aes_open(&aes, "MyAesKey", 8, "abcdefgh", 8);
    __test_rwencoded("test5.data", &codec);
    aes_close(&aes);
}

/* ============================================================================
 *  Data Tests
 */
static void __test6 (void) {
    const char *data = "Hello World";
    byte_slice_t slice;
    char buffer[10];
    int n;

    printf("================================================\n");
    printf("TEST 6 - Byte Slice\n");

    byte_slice_open(&slice, data + 6, 5);
    printf("Length: %u\n", slice_length(&slice));

    n = slice_copy(&slice, buffer, 0, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 0, 15);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 5, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 3, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 2, 2);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    byte_slice_close(&slice);
}

static void __test7 (void) {
    const char *data = "Hello World";
    prefix_slice_t slice;
    char buffer[16];
    int n;

    printf("================================================\n");
    printf("TEST 7 - Prefix Slice\n");

    prefix_slice_open(&slice, data, 5, data + 6, 5);
    printf("Length: %u\n", slice_length(&slice));

    n = slice_copy(&slice, buffer, 0, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 0, 15);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 5, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 3, 5);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    n = slice_copy(&slice, buffer, 2, 2);
    buffer[n] = 0;
    printf("%d '%s'\n", n, buffer);

    prefix_slice_close(&slice);
}


int main (int argc, char **argv) {
    printf("I/O\n");
    printf("stream vtable:   %ld\n", sizeof(stream_vtable_t));
    printf("stream:          %ld\n", sizeof(stream_t));
    printf("disk_stream:     %ld\n", sizeof(disk_stream_t));
    printf("buffered_writer: %ld\n", sizeof(buffered_writer_t));
    printf("buffered_reader: %ld\n", sizeof(buffered_reader_t));
    printf("encoded_writer:  %ld\n", sizeof(encoded_writer_t));
    printf("encoded_reader:  %ld\n", sizeof(encoded_reader_t));
    printf("DATA\n");
    printf("slice vtable:    %ld\n", sizeof(slice_vtable_t));
    printf("slice:           %ld\n", sizeof(slice_t));
    printf("byte slice:      %ld\n", sizeof(byte_slice_t));
    printf("prefix slice:    %ld\n", sizeof(prefix_slice_t));

    __test0();
    __test1();
    __test2();
    __test3();
    __test4();
    __test5();

    __test6();
    __test7();

    return(0);
}

