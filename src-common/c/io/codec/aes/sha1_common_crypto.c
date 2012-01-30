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

#if defined(SHA1_COMMON_CRYPTO)

#include <stdlib.h>

#include <CommonCrypto/CommonDigest.h>

#include "sha1.h"

void sha1_init (sha1_t *sha1) {
    CC_SHA1_Init(&(sha1->ctx));
}

void sha1_update (sha1_t *sha1, const void *data, unsigned int data_size) {
    CC_SHA1_Update(&(sha1->ctx), data, data_size);
}

void sha1_final (sha1_t *sha1, unsigned char digest[20]) {
    CC_SHA1_Final(digest, &(sha1->ctx));
}

#endif /* SHA1_COMMON_CRYPTO */

