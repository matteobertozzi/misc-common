#ifndef _AES_H_
#define _AES_H_

#if defined(AES_OPENSSL)
    #include <openssl/aes.h>
    #include <openssl/evp.h>
#endif

typedef struct aes {
#if defined(AES_OPENSSL)
    EVP_CIPHER_CTX enc;
    EVP_CIPHER_CTX dec;
#endif
} aes_t;

int     aes_open        (aes_t *aes,
                         const void *key,
                         int key_size,
                         const unsigned char salt[8]);
void    aes_close       (aes_t *aes);
int     aes_encrypt     (aes_t *aes,
                         void *dst,
                         const void *src,
                         unsigned int src_size);
int     aes_decrypt     (aes_t *aes,
                         void *dst,
                         const void *src,
                         unsigned int src_size);

#endif /* !_AES_H_ */

