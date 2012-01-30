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

#if defined(CRYPTO_COMMON_CRYPTO)

#include <CommonCrypto/CommonCryptor.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "crypto.h"

struct crypto_aes {
    CCCryptorRef  enc;
    CCCryptorRef  dec;
    unsigned char iv[32];
    pthread_mutex_t lock;
};

#define __aes_cryptor_create(cryptor, op, key, iv)                          \
    (CCCryptorCreate(op, kCCAlgorithmAES128, kCCOptionPKCS7Padding,         \
                     key, kCCKeySizeAES256, iv, cryptor) != kCCSuccess)

static int __aes_process (crypto_aes_t *crypto,
                          CCCryptorRef cryptor,
                          const void *src,
                          unsigned int src_size,
                          void *dst,
                          unsigned int *dst_size)
{
    size_t out_avail;
    size_t used = 0;
    uint8_t *outp;
    size_t moved;

    pthread_mutex_lock(&(crypto->lock));

    outp = (uint8_t *)dst;
    out_avail = src_size + 16;
    if (CCCryptorUpdate(cryptor, src, src_size, outp, out_avail, &moved)) {
        pthread_mutex_unlock(&(crypto->lock));
        CCCryptorReset(cryptor, crypto->iv);
        return(-2);
    }

    outp += moved;
    used += moved;
    out_avail -= moved;
    if (CCCryptorFinal(cryptor, outp, out_avail, &moved)) {
        pthread_mutex_unlock(&(crypto->lock));
        CCCryptorReset(cryptor, crypto->iv);
        return(-3);
    }

    used += moved;
    if (dst_size != NULL)
        *dst_size = used;

    CCCryptorReset(cryptor, crypto->iv);
    pthread_mutex_unlock(&(crypto->lock));
    return(0);
}

crypto_aes_t *crypto_aes_open (const void *key,
                               unsigned int key_size,
                               const void *salt,
                               unsigned int salt_size)
{
    unsigned char ikey[32];
    crypto_aes_t *crypto;

    /* Allocate Crypto AES Object */
    if ((crypto = (crypto_aes_t *) malloc(sizeof(crypto_aes_t))) == NULL)
        return(NULL);

    if (crypto_aes_key(ikey, crypto->iv, key, key_size, salt, salt_size)) {
        free(crypto);
        return(NULL);
    }

    if (pthread_mutex_init(&(crypto->lock), NULL)) {
        free(crypto);
        return(NULL);
    }

    /* Initialize Encryption */
    if (__aes_cryptor_create(&(crypto->enc), kCCEncrypt, ikey, crypto->iv)) {
        pthread_mutex_destroy(&(crypto->lock));
        free(crypto);
        return(NULL);
    }

    /* Initialize Decryption */
    if (__aes_cryptor_create(&(crypto->dec), kCCDecrypt, ikey, crypto->iv)) {
        CCCryptorRelease(crypto->enc);
        pthread_mutex_destroy(&(crypto->lock));
        free(crypto);
        return(NULL);
    }

    return(crypto);
}

void crypto_aes_close (crypto_aes_t *crypto) {
    CCCryptorRelease(crypto->enc);
    CCCryptorRelease(crypto->dec);
    pthread_mutex_destroy(&(crypto->lock));
    free(crypto);
}

int crypto_aes_encrypt (crypto_aes_t *crypto,
                        const void *src,
                        unsigned int src_size,
                        void *dst,
                        unsigned int *dst_size)
{
    return(__aes_process(crypto, crypto->enc, src, src_size, dst, dst_size));
}

int crypto_aes_decrypt (crypto_aes_t *crypto,
                        const void *src,
                        unsigned int src_size,
                        void *dst,
                        unsigned int *dst_size)
{
    return(__aes_process(crypto, crypto->dec, src, src_size, dst, dst_size));
}

#endif /* CRYPTO_COMMON_CRYPTO */

