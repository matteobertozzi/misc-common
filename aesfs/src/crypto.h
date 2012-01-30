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

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

typedef struct crypto_sha1 crypto_sha1_t;
typedef struct crypto_aes crypto_aes_t;

int             crypto_aes_key         (unsigned char ikey[32],
                                        unsigned char iv[32],
                                        const void *key,
                                        unsigned int key_size,
                                        const void *salt,
                                        unsigned int salt_size);
crypto_aes_t *  crypto_aes_open        (const void *key,
                                        unsigned int key_size,
                                        const void *salt,
                                        unsigned int salt_size);
void            crypto_aes_close       (crypto_aes_t *crypto);

int             crypto_aes_encrypt     (crypto_aes_t *crypto,
                                        const void *src,
                                        unsigned int src_size,
                                        void *dst,
                                        unsigned int *dst_size);
int             crypto_aes_decrypt     (crypto_aes_t *crypto,
                                        const void *src,
                                        unsigned int src_size,
                                        void *dst,
                                        unsigned int *dst_size);

#define CRYPTO_SHA1_LENGTH              20

crypto_sha1_t *crypto_sha1_open         (void);
void           crypto_sha1_close        (crypto_sha1_t *sha1);
void           crypto_sha1_reset        (crypto_sha1_t *sha1);
void           crypto_sha1_update       (crypto_sha1_t *sha1,
                                         const void *data,
                                         unsigned int data_size);
void           crypto_sha1_final        (crypto_sha1_t *sha1,
                                         unsigned char digest[20]);

#endif /* _CRYPTO_H_ */

