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

#ifndef __AES_DERIVATION_ROUNDS
    #define __AES_DERIVATION_ROUNDS     (5)
#endif /* !__AES_DERIVATION_ROUNDS */

typedef struct crypto_aes crypto_aes_t;

crypto_aes_t *  crypto_aes_open        (const void *key,
                                        int key_size,
                                        unsigned char salt[8]);
void            crypto_aes_close       (crypto_aes_t *crypto);

int             crypto_aes_encrypt     (crypto_aes_t *crypto,
                                        const void *src,
                                        int src_size,
                                        void *dst,
                                        int *dst_size);
int             crypto_aes_decrypt     (crypto_aes_t *crypto,
                                        const void *src,
                                        int src_size,
                                        void *dst,
                                        int *dst_size);

#endif /* _CRYPTO_H_ */

