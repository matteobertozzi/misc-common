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

#include "ByteSlice.h"
#include "Writable.h"

int ByteSlice::write (Writable *writable) const {
    size_t len = length();

    size_t index = 0;
    for (; len >= 8; len -= 8) {
        uint64_t v = fetch64(index);
        if (writable->writeFully(&v, 8) != 8)
            return(-1);
        index++;
    }

    index <<= 3;
    while (len-- > 0) {
        uint8_t v = fetch8(index);
        if (writable->writeFully(&v, 1) != 1)
            return(-2);
        index++;
    }

    return(0);
}

int ByteSlice::compare (const ByteSlice *other) const {
    size_t a_len = length();
    size_t b_len = other->length();
    size_t min_len = (a_len < b_len) ? a_len : b_len;
    size_t index = 0;
    int cmp;

    for (; min_len >= 8; min_len -= 8) {
        uint64_t u1 = other->fetch64(index);
        uint64_t u2 = fetch64(index);

        if (u1 != u2) {
            const char *m1 = (const char *)&u1;
            const char *m2 = (const char *)&u2;
            if (m1[0] != m2[0]) return(m1[0] - m2[0]);
            if (m1[1] != m2[1]) return(m1[1] - m2[1]);
            if (m1[2] != m2[2]) return(m1[2] - m2[2]);
            if (m1[3] != m2[3]) return(m1[3] - m2[3]);
            if (m1[4] != m2[4]) return(m1[4] - m2[4]);
            if (m1[5] != m2[5]) return(m1[5] - m2[5]);
            if (m1[6] != m2[6]) return(m1[6] - m2[6]);
            if (m1[7] != m2[7]) return(m1[7] - m2[7]);
        }

        index++;
    }

    index <<= 3;
    while (min_len-- > 0) {
        if ((cmp = fetch8(index) - other->fetch8(index)) != 0)
            return(cmp);

        index++;
    }

    return((a_len < b_len) ? -1 : (a_len > b_len) ? 1 : 0);
}

