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

#if defined(AES_COMMON_CRYPTO)

#include <CommonCrypto/CommonCryptor.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aes.h"

#define __aes_cryptor_create(cryptor, op, key, iv)                          \
    (CCCryptorCreate(op, kCCAlgorithmAES128, kCCOptionPKCS7Padding,         \
                     key, kCCKeySizeAES256, iv, cryptor) != kCCSuccess)

static int __aes_process (aes_t *aes,
                          CCCryptorRef cryptor,
                          void *dst,
                          const void *src,
                          unsigned int src_size)
{
    size_t out_avail;
	size_t used = 0;
    uint8_t *outp;
	size_t moved;

    outp = (uint8_t *)dst;
    out_avail = src_size + 16;
	if (CCCryptorUpdate(cryptor, src, src_size, outp, out_avail, &moved))
        return(-1);

	outp += moved;
	used += moved;
	out_avail -= moved;
    if (CCCryptorFinal(cryptor, outp, out_avail, &moved))
        return(-2);

	used += moved;

    CCCryptorReset(cryptor, aes->iv);
    return(used);
}

int aes_open (aes_t *aes,
              const void *key,
              unsigned int key_size,
              const void *salt,
              unsigned int salt_size)
{
    unsigned char ikey[32];

    aes_key(ikey, aes->iv, key, key_size, salt, salt_size);

    /* Initialize Encryption */
    if (__aes_cryptor_create(&(aes->enc), kCCEncrypt, ikey, aes->iv))
        return(-1);

    /* Initialize Decryption */
    if (__aes_cryptor_create(&(aes->dec), kCCDecrypt, ikey, aes->iv)) {
        CCCryptorRelease(aes->enc);
        return(-2);
    }

    return(0);
}

void aes_close (aes_t *aes) {
    CCCryptorRelease(aes->enc);
    CCCryptorRelease(aes->dec);
}

int aes_encrypt (aes_t *aes,
                 void *dst,
                 const void *src,
                 unsigned int src_size)
{
	return(__aes_process(aes, aes->enc, dst, src, src_size));
}

int aes_decrypt (aes_t *aes,
                 void *dst,
                 const void *src,
                 unsigned int src_size)
{
	return(__aes_process(aes, aes->dec, dst, src, src_size));
}

#endif /* AES_COMMON_CRYPTO */

