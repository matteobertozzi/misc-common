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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "crypto.h"

struct crypto_aes {
    EVP_CIPHER_CTX enc;
    EVP_CIPHER_CTX dec;
};

crypto_aes_t *crypto_aes_open (const void *key,
                               int key_size,
                               unsigned char salt[8])
{
    unsigned char ikey[32];
    unsigned char iv[32];
    crypto_aes_t *crypto;
    int ivlen;

    /* Key Derivation */
    ivlen = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(),
                           salt, key, key_size,
                           __AES_DERIVATION_ROUNDS,
                           ikey, iv);
    if (ivlen != 32)
        return(NULL);

    /* Allocate Crypto AES Object */
    if ((crypto = (crypto_aes_t *) malloc(sizeof(crypto_aes_t))) == NULL)
        return(NULL);

    /* Initialize Encryption */
    EVP_CIPHER_CTX_init(&(crypto->enc));
    if (!EVP_EncryptInit_ex(&(crypto->enc), EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_cleanup(&(crypto->enc));
        free(crypto);
        return(NULL);
    }

    /* Initialize Decryption */
    EVP_CIPHER_CTX_init(&(crypto->dec));
    if (!EVP_DecryptInit_ex(&(crypto->dec), EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_cleanup(&(crypto->enc));
        EVP_CIPHER_CTX_cleanup(&(crypto->dec));
        free(crypto);
        return(NULL);
    }

    return(crypto);
}

void crypto_aes_close (crypto_aes_t *crypto) {
    EVP_CIPHER_CTX_cleanup(&(crypto->enc));
    EVP_CIPHER_CTX_cleanup(&(crypto->dec));
    free(crypto);
}

int crypto_aes_encrypt (crypto_aes_t *crypto,
                        const void *src,
                        int src_size,
                        void *dst,
                        int *dst_size)
{
    EVP_CIPHER_CTX *e = &(crypto->enc);
    int psize = 0;
    int fsize = 0;

    /* allows reusing of 'e' for multiple encryption cycles */
    if (!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL))
        return(-1);

    /* update ciphertext, c_len is filled with the length of ciphertext
     * generated, *len is the size of plaintext in bytes
     */
    if (!EVP_EncryptUpdate(e, dst, &psize, src, src_size))
        return(-2);

    /* update ciphertext with the final remaining bytes */
    if (!EVP_EncryptFinal_ex(e, dst + psize, &fsize))
        return(-3);

    if (dst_size != NULL)
        *dst_size = psize + fsize;
    return(0);
}

int crypto_aes_decrypt (crypto_aes_t *crypto,
                        const void *src,
                        int src_size,
                        void *dst,
                        int *dst_size)
{
    EVP_CIPHER_CTX *e = &(crypto->dec);
    int psize = 0;
    int fsize = 0;

    if (!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL))
        return(-1);

    if (!EVP_DecryptUpdate(e, dst, &psize, src, src_size))
        return(-2);

    if (!EVP_DecryptFinal_ex(e, dst + psize, &fsize))
        return(-3);

    if (dst_size != NULL)
        *dst_size = psize + fsize;
    return(0);
}

#ifdef __AES_DEBUG_MAIN
int main (int argc, char **argv) {
    unsigned char buf0[2048];
    unsigned char buf1[2048];
    int buf0sz;
    int buf1sz;
    crypto_aes_t *aes;

    if ((aes = crypto_aes_open("key", 3, "01234567")) == NULL)
        return(1);

    memset(buf0, 0, sizeof(buf0));
    memcpy(buf0, "Hello World", 11);

    crypto_aes_encrypt(aes, buf0, 1024, buf1, &buf1sz);
    crypto_aes_decrypt(aes, buf1, buf1sz, buf0, &buf0sz);

    buf1[buf1sz] = 0;
    printf("BUF0SZ %d\n", buf1sz);
    printf("BUF1SZ %d %s\n", buf0sz, buf0);
    crypto_aes_close(aes);

    return(0);
}
#endif /* !__AES_DEBUG_MAIN */

