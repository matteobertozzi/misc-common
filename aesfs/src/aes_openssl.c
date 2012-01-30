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

#if defined(CRYPTO_OPENSSL)

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "crypto.h"

struct crypto_aes {
    EVP_CIPHER_CTX enc;
    EVP_CIPHER_CTX dec;
    pthread_mutex_t lock;
};

crypto_aes_t *crypto_aes_open (const void *key,
                               unsigned int key_size,
                               const void *salt,
                               unsigned int salt_size)
{
    unsigned char ikey[32];
    unsigned char iv[32];
    crypto_aes_t *crypto;

    /* Key Derivation */
    if (crypto_aes_key(ikey, iv, key, key_size, salt, salt_size))
        return(NULL);

    /* Allocate Crypto AES Object */
    if ((crypto = (crypto_aes_t *) malloc(sizeof(crypto_aes_t))) == NULL)
        return(NULL);

    if (pthread_mutex_init(&(crypto->lock), NULL)) {
        free(crypto);
        return(NULL);
    }

    /* Initialize Encryption */
    EVP_CIPHER_CTX_init(&(crypto->enc));
    if (!EVP_EncryptInit_ex(&(crypto->enc), EVP_aes_256_cbc(), NULL, ikey, iv)) {
        EVP_CIPHER_CTX_cleanup(&(crypto->enc));
        pthread_mutex_destroy(&(crypto->lock));
        free(crypto);
        return(NULL);
    }

    /* Initialize Decryption */
    EVP_CIPHER_CTX_init(&(crypto->dec));
    if (!EVP_DecryptInit_ex(&(crypto->dec), EVP_aes_256_cbc(), NULL, ikey, iv)) {
        EVP_CIPHER_CTX_cleanup(&(crypto->enc));
        EVP_CIPHER_CTX_cleanup(&(crypto->dec));
        pthread_mutex_destroy(&(crypto->lock));
        free(crypto);
        return(NULL);
    }

    return(crypto);
}

void crypto_aes_close (crypto_aes_t *crypto) {
    EVP_CIPHER_CTX_cleanup(&(crypto->enc));
    EVP_CIPHER_CTX_cleanup(&(crypto->dec));
    pthread_mutex_destroy(&(crypto->lock));
    free(crypto);
}

int crypto_aes_encrypt (crypto_aes_t *crypto,
                        const void *src,
                        unsigned int src_size,
                        void *dst,
                        unsigned int *dst_size)
{
    EVP_CIPHER_CTX *e = &(crypto->enc);
    int psize = 0;
    int fsize = 0;

    /* allows reusing of 'e' for multiple encryption cycles */
    if (!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-1);
    }

    /* update ciphertext, c_len is filled with the length of ciphertext
     * generated, *len is the size of plaintext in bytes
     */
    if (!EVP_EncryptUpdate(e, dst, &psize, src, src_size)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-2);
    }

    /* update ciphertext with the final remaining bytes */
    if (!EVP_EncryptFinal_ex(e, dst + psize, &fsize)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-3);
    }

    if (dst_size != NULL)
        *dst_size = psize + fsize;

    pthread_mutex_unlock(&(crypto->lock));
    return(0);
}

int crypto_aes_decrypt (crypto_aes_t *crypto,
                        const void *src,
                        unsigned int src_size,
                        void *dst,
                        unsigned int *dst_size)
{
    EVP_CIPHER_CTX *e = &(crypto->dec);
    int psize = 0;
    int fsize = 0;

    pthread_mutex_lock(&(crypto->lock));

    if (!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-1);
    }

    if (!EVP_DecryptUpdate(e, dst, &psize, src, src_size)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-2);
    }

    if (!EVP_DecryptFinal_ex(e, dst + psize, &fsize)) {
        pthread_mutex_unlock(&(crypto->lock));
        return(-3);
    }

    if (dst_size != NULL)
        *dst_size = psize + fsize;

    pthread_mutex_unlock(&(crypto->lock));
    return(0);
}

#endif /* CRYPTO_OPENSSL */

