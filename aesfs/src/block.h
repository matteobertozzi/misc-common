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

#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdint.h>

#define IOFHEAD_SIZE             (sizeof(struct iofhead))
#define IOFHEAD_MAGIC            (0x71cc75bf)
#define IOBLOCK_MAGIC            (0x7d95)

#define IOBLOCK_AES_SIZE         (16)
#define IOBLOCK_DISK_SIZE        (512)
#define IOBLOCK_HEAD_SIZE        (sizeof(struct iohead))
#define IOBLOCK_BODY_SIZE        (IOBLOCK_DISK_SIZE - IOBLOCK_HEAD_SIZE)
#define IOBLOCK_USER_SIZE        (IOBLOCK_BODY_SIZE - IOBLOCK_AES_SIZE)

typedef struct iocodec_plug iocodec_plug_t;
typedef union  iocodec_data iocodec_data_t;
typedef struct iocodec iocodec_t;
typedef struct iohead iohead_t;

struct iofhead {
    uint32_t magic;
    uint32_t flags;
    uint64_t length;
} __attribute__((__packed__));

struct iohead {
    uint16_t magic;
    uint16_t length;
    uint32_t crc;
} __attribute__((__packed__));

struct iocodec_plug {
    int (*encode) (iocodec_data_t *data, void *dst, const void *src);
    int (*decode) (iocodec_data_t *data, void *dst, const void *src);
};

union iocodec_data {
    void *   ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
};

struct iocodec {
    iocodec_plug_t *plug;
    iocodec_data_t data;
};

extern iocodec_plug_t ioblock_plain_codec;
extern iocodec_plug_t ioblock_xor_codec;
extern iocodec_plug_t ioblock_aes_codec;

size_t  ioread          (int fd, void *buf, size_t size, off_t offset);
size_t  iowrite         (int fd, const void *buf, size_t size, off_t offset);

int     iofhead_read    (int fd, struct iofhead *fhead);
int     iofhead_write   (int fd, const struct iofhead *fhead);

int     ioblock_read    (iocodec_t *codec,
                         int fd,
                         char *buf,
                         size_t size,
                         off_t offset);
int     ioblock_write   (iocodec_t *codec,
                         int fd,
                         const char *buf,
                         size_t size,
                         off_t offset);

uint32_t crc32c (const void *data, unsigned int n);

#endif /* !_BLOCK_H_ */

