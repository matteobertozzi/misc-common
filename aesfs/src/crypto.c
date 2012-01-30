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

#include <stdlib.h>

#include "crypto.h"

#ifndef __AES_DERIVATION_ROUNDS
    #define __AES_DERIVATION_ROUNDS     (5U)
#endif /* !__AES_DERIVATION_ROUNDS */

int crypto_aes_key (unsigned char ikey[32],
                    unsigned char iv[32],
                    const void *key,
                    unsigned int key_size,
                    const void *salt,
                    unsigned int salt_size)
{
    unsigned char sha1_buf[CRYPTO_SHA1_LENGTH];
    unsigned int nkey = 32;
    unsigned int niv = 32;
    crypto_sha1_t *sha1;
    unsigned int i;

    if ((sha1 = crypto_sha1_open()) == NULL)
        return(1);

    for (;;) {
        crypto_sha1_update(sha1, key, key_size);
        crypto_sha1_update(sha1, salt, salt_size);
        crypto_sha1_final(sha1, sha1_buf);

        for (i = 1; i < __AES_DERIVATION_ROUNDS; ++i) {
            crypto_sha1_reset(sha1);
            crypto_sha1_update(sha1, sha1_buf, CRYPTO_SHA1_LENGTH);
            crypto_sha1_final(sha1, sha1_buf);
        }

        i = 0;
        if (nkey > 0) {
            for (;;) {
                if (nkey == 0 || i == CRYPTO_SHA1_LENGTH)
                    break;
                *(ikey++) = sha1_buf[i];
                nkey--;
                i++;
            }
        }

        if (niv > 0 && (i != CRYPTO_SHA1_LENGTH)) {
            for (;;) {
                if (niv == 0 || i == CRYPTO_SHA1_LENGTH)
                    break;

                *iv++ = sha1_buf[i];
                niv--;
                i++;
            }
        }

        if (nkey == 0 && niv == 0)
            break;
            
        crypto_sha1_reset(sha1);
        crypto_sha1_update(sha1, sha1_buf, CRYPTO_SHA1_LENGTH);
    }

    crypto_sha1_close(sha1);
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

