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

#if defined(AES_OPENSSL)

#include "aes.h"

int aes_open (aes_t *aes,
              const void *key,
              unsigned int key_size,
              const void *salt,
              unsigned int salt_size)
{
    unsigned char ikey[32];
    unsigned char iv[32];

    aes_key(ikey, iv, key, key_size, salt, salt_size);

    EVP_CIPHER_CTX_init(&(aes->enc));
    if (!EVP_EncryptInit_ex(&(aes->enc), EVP_aes_256_cbc(), NULL, ikey, iv)) {
        EVP_CIPHER_CTX_cleanup(&(aes->enc));
        return(-1);
    }

    EVP_CIPHER_CTX_init(&(aes->dec));
    if (!EVP_DecryptInit_ex(&(aes->dec), EVP_aes_256_cbc(), NULL, ikey, iv)) {
        EVP_CIPHER_CTX_cleanup(&(aes->enc));
        EVP_CIPHER_CTX_cleanup(&(aes->dec));
        return(-2);
    }

    return(0);
}

void aes_close (aes_t *aes) {
    EVP_CIPHER_CTX_cleanup(&(aes->enc));
    EVP_CIPHER_CTX_cleanup(&(aes->dec));
}

int aes_encrypt (aes_t *aes,
                 void *dst,
                 const void *src,
                 unsigned int src_size)
{
    EVP_CIPHER_CTX *e = &(aes->enc);
    int fsize = 0;
    int psize;

    if (!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL))
        return(-1);

   if (!EVP_EncryptUpdate(e, dst, &psize, src, src_size))
        return(-2);

    if (!EVP_EncryptFinal_ex(e, dst + psize, &fsize))
        return(-3);

    return(psize + fsize);
}

int aes_decrypt (aes_t *aes,
                 void *dst,
                 const void *src,
                 unsigned int src_size)
{
    EVP_CIPHER_CTX *e = &(aes->dec);
    int fsize = 0;
    int psize = 0;

    if (!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL))
        return(-1);

    if (!EVP_DecryptUpdate(e, dst, &psize, src, src_size))
        return(-2);

    if (!EVP_DecryptFinal_ex(e, dst + psize, &fsize))
        return(-3);

    return(psize + fsize);
}

#endif /* AES_OPENSSL */

