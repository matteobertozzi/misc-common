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

#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "crypto.h"
#include "block.h"

#define __align_down(x, size)    (((x) / (size)) * (size))
#define __ioblock_offset(x)      (IOFHEAD_SIZE + (((x) / IOBLOCK_USER_SIZE) * IOBLOCK_DISK_SIZE))

typedef struct ioblock ioblock_t;

struct ioblock {
    iohead_t head;
    uint8_t  body[IOBLOCK_BODY_SIZE];
} __attribute__((__packed__));

size_t ioread (int fd, void *buf, size_t size, off_t off) {
    unsigned char *pbuf = (unsigned char *)buf;
    ssize_t rd;
    size_t n;

    n = 0;
    while (n < size) {
        if ((rd = pread(fd, pbuf + n, size - n, off + n)) <= 0)
            break;

        n += rd;
    }

    memset(pbuf + n, 0, size - n);
    return(n);
}

size_t iowrite (int fd, const void *buf, size_t size, off_t off) {
    const unsigned char *pbuf = (const unsigned char *)buf;
    ssize_t wr;
    size_t n;

    n = 0;
    while (n < size) {
        if ((wr = pwrite(fd, pbuf + n, size - n, off + n)) <= 0)
            break;

        n += wr;
    }

    return(n);
}

int iofhead_read (int fd, struct iofhead *fhead) {
    if (ioread(fd, fhead, IOFHEAD_SIZE, 0) != IOFHEAD_SIZE)
        return(-1);

    if (fhead->magic != IOFHEAD_MAGIC)
        return(-2);

    return(0);
}

int iofhead_write (int fd, const struct iofhead *fhead) {
    return(iowrite(fd, fhead, IOFHEAD_SIZE, 0) != IOFHEAD_SIZE);
}

#define __ioblock_read(fd, block, offset)                                   \
    ioread(fd, block, sizeof(ioblock_t), offset)

#define __ioblock_write(fd, block, offset)                                  \
    iowrite(fd, block, sizeof(ioblock_t), offset)

#define __ioblock_encode(codec, dblock, ublock)                             \
    ((codec)->plug->encode(&((codec)->data), dblock, ublock))

#define __ioblock_decode(codec, ublock, dblock)                             \
    ((codec)->plug->decode(&((codec)->data), ublock, dblock))

static int __ioblock_fetch (iocodec_t *codec,
                            int fd,
                            off_t offset,
                            ioblock_t *dblock,
                            ioblock_t *ublock)
{
    uint32_t crc;
    ssize_t rd;

    if ((rd = __ioblock_read(fd, dblock, offset)) == 0) {
        memset(ublock, 0, sizeof(ioblock_t));
        return(1);
    }

    if (rd != IOBLOCK_DISK_SIZE)
        return(-1);

    if (__ioblock_decode(codec, ublock, dblock))
        return(-2);

    /* Check magic */
    if (ublock->head.magic != IOBLOCK_MAGIC)
        return(-3);

    /* Check crc */
    crc = crc32c(ublock->body, ublock->head.length);
    if (ublock->head.crc != crc) {
        fprintf(stderr, "fetch(): FAIL CRC %u != %u\n", ublock->head.crc, crc);
        return(-4);
    }

    return(0);
}

static int __ioblock_store (iocodec_t *codec,
                            int fd,
                            off_t offset,
                            ioblock_t *dblock,
                            ioblock_t *ublock)
{
    ublock->head.magic = IOBLOCK_MAGIC;
    ublock->head.crc = crc32c(ublock->body, ublock->head.length);

    if (__ioblock_encode(codec, dblock, ublock))
        return(-1);

    if (__ioblock_write(fd, dblock, offset) != IOBLOCK_DISK_SIZE)
        return(-2);

    return(0);
}

int ioblock_read (iocodec_t *codec,
                  int fd,
                  char *buf,
                  size_t size,
                  off_t offset)
{
    ioblock_t dblock;
    ioblock_t ublock;
    off_t doffset;
    size_t avail;
    int wr;

    doffset = __ioblock_offset(offset);
    if (__ioblock_fetch(codec, fd, doffset, &dblock, &ublock))
        return(-1);

    offset = (offset - __align_down(offset, IOBLOCK_USER_SIZE));
    avail = ublock.head.length - offset;
    avail = (size > avail) ? avail : size;
    memcpy(buf, ublock.body + offset, avail);

    wr = avail;
    size -= avail;
    while (size > 0) {
        doffset += IOBLOCK_DISK_SIZE;
        if (__ioblock_fetch(codec, fd, doffset, &dblock, &ublock))
            break;

        avail = (size > ublock.head.length) ? ublock.head.length : size;
        memcpy(buf + wr, ublock.body, avail);

        wr += avail;
        size -= avail;
    }

    return(wr);
}

int ioblock_write (iocodec_t *codec,
                   int fd,
                   const char *buf,
                   size_t size,
                   off_t offset)
{
    ioblock_t dblock;
    ioblock_t ublock;
    off_t doffset;
    size_t avail;
    int wr;

    doffset = __ioblock_offset(offset);

    offset = (offset - __align_down(offset, IOBLOCK_USER_SIZE));
    avail = offset + size;
    if (avail >= IOBLOCK_USER_SIZE && offset == 0) {
        avail = IOBLOCK_USER_SIZE;
        ublock.head.length = avail;
    } else {
        if (__ioblock_fetch(codec, fd, doffset, &dblock, &ublock) < 0)
            return(-1);

        if (avail > IOBLOCK_USER_SIZE)
            avail = IOBLOCK_USER_SIZE;

        if (avail > ublock.head.length)
            ublock.head.length = avail;

        avail = avail - offset;
    }

    memcpy(ublock.body + offset, buf, avail);
    if (__ioblock_store(codec, fd, doffset, &dblock, &ublock))
        return(0);

    wr = avail;
    size -= avail;
    while (size > 0) {
        doffset += IOBLOCK_DISK_SIZE;

        if (size >= IOBLOCK_USER_SIZE) {
            avail = IOBLOCK_USER_SIZE;
            ublock.head.length = avail;
        } else {
            if (__ioblock_fetch(codec, fd, doffset, &dblock, &ublock) < 0)
                break;

            avail = size;
            if (size > ublock.head.length)
                ublock.head.length = avail;
        }

        memcpy(ublock.body, buf + wr, avail);
        if (__ioblock_store(codec, fd, doffset, &dblock, &ublock))
            break;

        wr += avail;
        size -= avail;
    }

    return(wr);
}

static int __ioblock_encode_plain (iocodec_data_t *data, void *dst, const void *src) {
    memset(dst, 0, IOBLOCK_DISK_SIZE);
    memcpy(dst, src, IOBLOCK_DISK_SIZE);
    return(0);
}

static int __ioblock_decode_plain (iocodec_data_t *data, void *dst, const void *src) {
    memset(dst, 0, IOBLOCK_DISK_SIZE);
    memcpy(dst, src, IOBLOCK_DISK_SIZE);
    return(0);
}

iocodec_plug_t ioblock_plain_codec = {
    .encode = __ioblock_encode_plain,
    .decode = __ioblock_decode_plain,
};

static int __ioblock_codec_xor (iocodec_data_t *data, void *dst, const void *src) {
    const uint64_t *psrc = (const uint64_t *)src;
    uint64_t *pdst = (uint64_t *)dst;
    unsigned int n;

    memset(dst, 0, IOBLOCK_DISK_SIZE);
    for (n = 0; n < IOBLOCK_DISK_SIZE; n += 8) {
        *pdst = *psrc ^ data->u64;
        pdst++;
        psrc++;
    }

    return(0);
}

iocodec_plug_t ioblock_xor_codec = {
    .encode = __ioblock_codec_xor,
    .decode = __ioblock_codec_xor,
};

static int __ioblock_encode_aes (iocodec_data_t *data, void *dst, const void *src) {
    return(crypto_aes_encrypt((crypto_aes_t *)data->ptr,
                              src, IOBLOCK_DISK_SIZE - IOBLOCK_AES_SIZE,
                              dst, NULL));
}

static int __ioblock_decode_aes (iocodec_data_t *data, void *dst, const void *src) {
    return(crypto_aes_decrypt((crypto_aes_t *)data->ptr,
                              src, IOBLOCK_DISK_SIZE,
                              dst, NULL));
}

iocodec_plug_t ioblock_aes_codec = {
    .encode = __ioblock_encode_aes,
    .decode = __ioblock_decode_aes,
};

#ifdef __BLOCK_DEBUG_MAIN
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char **argv) {
    char buffer[1024];
    int fd;
    int n;

    if ((fd = open("test.data", O_CREAT | O_RDWR, 0644)) < 0) {
        perror("open()");
        return(1);
    }

    n = ioblock_write(&ioblock_plain_codec, fd, "Hello World.", 12, 0);
    printf("1. write %d\n", n);

    n = ioblock_write(&ioblock_plain_codec, fd, "This is a test...!", 18, 12);
    printf("2. write %d\n", n);

    if ((n = ioblock_read(&ioblock_plain_codec, fd, buffer, 8, 0)) > 0) {
        buffer[n] = '\0';
        printf("3. read %d %s\n", n, buffer);
    } else {
        printf("4. read fail\n");
    }

    if ((n = ioblock_read(&ioblock_plain_codec, fd, buffer, 50, 11)) > 0) {
        buffer[n] = '\0';
        printf("5. read %d %s\n", n, buffer);
    } else {
        printf("6. read fail\n");
    }

    close(fd);
    return(0);
}
#endif /* !__BLOCK_DEBUG_MAIN */

