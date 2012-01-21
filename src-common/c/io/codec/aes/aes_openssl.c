#include "aes.h"

#define __AES_DERIVATION_ROUNDS         (7)

int aes_open (aes_t *aes,
              const void *key,
              int key_size,
              const unsigned char salt[8])
{
    unsigned char ikey[32];
    unsigned char iv[32];
    int ivlen;

    ivlen = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(),
                           salt, key, key_size,
                           __AES_DERIVATION_ROUNDS,
                           ikey, iv);
    if (ivlen != 32)
        return(-1);

    EVP_CIPHER_CTX_init(&(aes->enc));
    if (!EVP_EncryptInit_ex(&(aes->enc), EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_cleanup(&(aes->enc));
        return(-2);
    }

    EVP_CIPHER_CTX_init(&(aes->dec));
    if (!EVP_DecryptInit_ex(&(aes->dec), EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_cleanup(&(aes->enc));
        EVP_CIPHER_CTX_cleanup(&(aes->dec));
        return(-3);
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

