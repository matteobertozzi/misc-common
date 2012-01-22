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

#ifndef _IO_UTIL_H_
#define _IO_UTIL_H_

#include <stdlib.h>
#include "crypto.h"

/* http://www.gnu.org/software/libc/manual/html_node/getpass.html */
ssize_t iogetpass (const char *prompt, char **lineptr);
void iofreepass (char **lineptr, ssize_t n);

crypto_aes_t *crypto_aes_from_input (void);
uint64_t xor_from_input (void);

#endif /* !_IO_UTIL_H_ */

