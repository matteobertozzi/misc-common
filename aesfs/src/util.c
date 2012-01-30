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

#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "util.h"

/* http://www.gnu.org/software/libc/manual/html_node/getpass.html */
ssize_t iogetpass (const char *prompt, char **lineptr) {
    struct termios old, new;
    ssize_t nread;
    size_t n;

    fprintf(stderr, "%s", prompt);

    /* Turn echoing off and fail if we can't. */
    if (tcgetattr(STDIN_FILENO, &old) != 0)
        return(-1);

    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
        return(-2);

    /* Read the password. */
    n = 0;
    *lineptr = NULL;
    if ((nread = getline(lineptr, &n, stdin)) > 0)
        (*lineptr)[--nread] = '\0';

    /* Restore terminal. */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);

    fprintf(stderr, "\n");
    return(nread);
}

void iofreepass (char **lineptr, ssize_t n) {
    char *pline = *lineptr;
    while (n--)
        *pline++ = 0;
    free(*lineptr);
    *lineptr = NULL;
}


crypto_aes_t *crypto_aes_from_input (void) {
    crypto_aes_t *aes;
    ssize_t salt_len;
    ssize_t key_len;
    char *salt;
    char *key;

    /* Retrive Key & Salt */
    if ((key_len = iogetpass("key: ", &key)) < 0)
        return(NULL);

    if ((salt_len = iogetpass("salt: ", &salt)) < 0) {
        iofreepass(&key, key_len);
        return(NULL);
    }

    /* Initialize AES */
    aes = crypto_aes_open(key, key_len, salt, salt_len);

    /* Free Key & Salt */
    iofreepass(&key, key_len);
    iofreepass(&salt, salt_len);

    return(aes);
}

uint64_t xor_from_input (void) {
    uint64_t xor = 0x215730a664adb230ULL;
    ssize_t key_len;
    char *key;

    if ((key_len = iogetpass("key: ", &key)) <= 0)
        return(0);

    if (key_len >= 8)
        xor = *((uint64_t *)key);
    else
        memcpy(&xor, key, key_len);

    iofreepass(&key, key_len);

    return(xor);
}

