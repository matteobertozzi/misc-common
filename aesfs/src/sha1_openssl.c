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

#include <stdlib.h>

#include <openssl/sha.h>

#include "crypto.h"

struct crypto_sha1 {
    SHA_CTX ctx;
};

crypto_sha1_t *crypto_sha1_open (void) {
    crypto_sha1_t *sha1;

    if ((sha1 = (crypto_sha1_t *) malloc(sizeof(crypto_sha1_t))) != NULL)
        SHA1_Init(&(sha1->ctx));

    return(sha1);
}

void crypto_sha1_close (crypto_sha1_t *sha1) {
    free(sha1);
}

void crypto_sha1_reset (crypto_sha1_t *sha1) {
    SHA1_Init(&(sha1->ctx));
}

void crypto_sha1_update (crypto_sha1_t *sha1,
                         const void *data,
                         unsigned int data_size)
{
    SHA1_Update(&(sha1->ctx), data, data_size);
}

void crypto_sha1_final (crypto_sha1_t *sha1,
                        unsigned char digest[20])
{
    SHA1_Final(digest, &(sha1->ctx));
}

#endif /* CRYPTO_OPENSSL */

