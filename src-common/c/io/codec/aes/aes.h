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

#ifndef _AES_H_
#define _AES_H_

#if defined(AES_OPENSSL)
    #include <openssl/aes.h>
    #include <openssl/evp.h>
#elif defined(AES_COMMON_CRYPTO)
    #include <CommonCrypto/CommonCryptor.h>
#endif

typedef struct aes {
#if defined(AES_OPENSSL)
    EVP_CIPHER_CTX enc;
    EVP_CIPHER_CTX dec;
#elif defined(AES_COMMON_CRYPTO)
    CCCryptorRef  enc;
    CCCryptorRef  dec;
    unsigned char iv[32];
#endif
} aes_t;

void   aes_key          (unsigned char ikey[32],
                         unsigned char iv[32],
                         const void *key,
                         unsigned int key_size,
                         const void *salt,
                         unsigned int salt_size);

int     aes_open        (aes_t *aes,
                         const void *key,
                         unsigned int key_size,
                         const void *salt,
                         unsigned int salt_size);
void    aes_close       (aes_t *aes);
int     aes_encrypt     (aes_t *aes,
                         void *dst,
                         const void *src,
                         unsigned int src_size);
int     aes_decrypt     (aes_t *aes,
                         void *dst,
                         const void *src,
                         unsigned int src_size);

#endif /* !_AES_H_ */

