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

#ifndef _SHA1_H_
#define _SHA1_H_

#define SHA1_DIGEST_LENGTH          20

#if defined(SHA1_OPENSSL)
    #include <openssl/sha.h>
#elif defined(SHA1_COMMON_CRYPTO)
    #include <CommonCrypto/CommonDigest.h>
#endif

typedef struct sha1 {
#if defined(SHA1_OPENSSL)
    SHA_CTX ctx;
#elif defined(SHA1_COMMON_CRYPTO)
    CC_SHA1_CTX ctx;
#endif
} sha1_t;

void sha1_init   (sha1_t *sha1);
void sha1_update (sha1_t *sha1, const void *data, unsigned int data_size);
void sha1_final  (sha1_t *sha1, unsigned char digest[20]);

#endif /* !_SHA1_H_ */

