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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "block.h"
#include "util.h"

typedef int (*file_func_t) (iocodec_t *codec, const char *src, const char *dst);

static int __file_encrypt (iocodec_t *codec, const char *src, const char *dst) {
    char buffer[IOBLOCK_BODY_SIZE];
    struct iofhead fhead;
    int sfd, dfd;
    size_t rd;
    off_t off;

    printf("encrypt %s -> %s\n", src, dst);

    if ((sfd = open(src, O_RDONLY)) < 0) {
        perror("open()");
        return(1);
    }

    if ((dfd = open(dst, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
        perror("open()");
        close(sfd);
        return(2);
    }

    off = 0;
    while ((rd  = ioread(sfd, buffer, IOBLOCK_USER_SIZE, off)) > 0) {
        if (ioblock_write(codec, dfd, buffer, rd, off) <= 0)
            break;
        off += rd;
    }

    fhead.magic = IOFHEAD_MAGIC;
    fhead.flags = 0;
    fhead.length = off;
    iofhead_write(dfd, &fhead);

    close(dfd);
    close(sfd);
    return(0);
}

static int __file_decrypt (iocodec_t *codec, const char *src, const char *dst) {
    char buffer[IOBLOCK_DISK_SIZE];
    int sfd, dfd;
    off_t off;
    size_t rd;

    printf("decrypt %s -> %s\n", src, dst);

    if ((sfd = open(src, O_RDONLY)) < 0) {
        perror("open()");
        return(1);
    }

    if ((dfd = open(dst, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
        perror("open()");
        close(sfd);
        return(2);
    }

    off = 0;
    while ((rd  = ioblock_read(codec, sfd, buffer, IOBLOCK_USER_SIZE, off)) > 0)
    {
        if (iowrite(dfd, buffer, rd, off) <= 0)
            break;
        off += rd;
    }

    close(dfd);
    close(sfd);
    return(0);
}

int main (int argc, char **argv) {
    file_func_t ffunc = __file_encrypt;
    iocodec_t codec;
    int o;

    while ((o = getopt(argc, argv, "hdax")) != -1) {
      switch (o) {
        case 'h':
          fprintf(stderr, "usage: aespack [-d] <codec> <files...>\n");
          fprintf(stderr, "codec:\n");
          fprintf(stderr, "   -a   AES codec:\n");
          fprintf(stderr, "   -x   XOR Codec:\n");
          fprintf(stderr, "   -p   Plain Codec:\n");
          return(0);
        case 'd': /* Decompress */
          ffunc = __file_decrypt;
          break;
        case 'a': /* AES codec */
          codec.plug = &ioblock_aes_codec;
          if ((codec.data.ptr = crypto_aes_from_input()) == NULL) {
            fprintf(stderr, "Failed to initialize AES.\n");
            return(EXIT_FAILURE);
          }
          break;
        case 'x': /* Xor codec */
          codec.plug = &ioblock_xor_codec;
          if (!xor_from_input()) {
            fprintf(stderr, "Failed to initialize XOR.\n");
            return(EXIT_FAILURE);
          }
          break;
        case 'p': /* Plain codec */
          codec.plug = &ioblock_plain_codec;
          break;
        default:
          abort();
      }
    }

    if (codec.plug == NULL) {
        fprintf(stderr, "missing codec.\n");
        return(EXIT_FAILURE);
    }

    if ((argc - optind) & 1) {
        fprintf(stderr, "file must be provided as <src> <dst> [<src> <dst>]\n");
        return(EXIT_FAILURE);
    }

    for (o = optind; o < argc; o += 2) {
        if (ffunc(&codec, argv[o], argv[o + 1])) {
            fprintf(stderr, "error during '%s'.\n", argv[o]);
            return(EXIT_FAILURE);
        }
    }

    return(0);
}

