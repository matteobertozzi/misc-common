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

#include "sha1.h"
#include "aes.h"

#ifndef __AES_DERIVATION_ROUNDS
    #define __AES_DERIVATION_ROUNDS     (7U)
#endif /* !__AES_DERIVATION_ROUNDS */

void aes_key (unsigned char ikey[32],
             unsigned char iv[32],
             const void *key,
             unsigned int key_size,
             const void *salt,
             unsigned int salt_size)
{
    unsigned char sha1_buf[SHA1_DIGEST_LENGTH];
    unsigned int nkey = 32;
    unsigned int niv = 32;
    unsigned int i;
    sha1_t sha1;

    sha1_init(&sha1);

    for (;;) {
        sha1_update(&sha1, key, key_size);
        sha1_update(&sha1, salt, salt_size);
        sha1_final(&sha1, sha1_buf);

        for (i = 1; i < __AES_DERIVATION_ROUNDS; ++i) {
            sha1_init(&sha1);
            sha1_update(&sha1, sha1_buf, SHA1_DIGEST_LENGTH);
            sha1_final(&sha1, sha1_buf);
        }

        i = 0;
        if (nkey > 0) {
            for (;;) {
                if (nkey == 0 || i == SHA1_DIGEST_LENGTH)
                    break;
                *(ikey++) = sha1_buf[i];
                nkey--;
                i++;
            }
        }

        if (niv > 0 && (i != SHA1_DIGEST_LENGTH)) {
            for (;;) {
                if (niv == 0 || i == SHA1_DIGEST_LENGTH)
                    break;

                *iv++ = sha1_buf[i];
                niv--;
                i++;
            }
        }

        if (nkey == 0 && niv == 0)
            break;
            
        sha1_init(&sha1);
        sha1_update(&sha1, sha1_buf, SHA1_DIGEST_LENGTH);
    }
}

