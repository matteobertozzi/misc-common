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

#ifndef _BYTE_SLICE_H_
#define _BYTE_SLICE_H_

#include <stdint.h>
#include <stddef.h>

class Writable;

class ByteSlice {
    public:
        virtual size_t length (void) const = 0;

        virtual int write (Writable *writable) const;

        virtual uint8_t  fetch8  (size_t index) const = 0;
        virtual uint16_t fetch16 (size_t index) const;
        virtual uint32_t fetch32 (size_t index) const;
        virtual uint64_t fetch64 (size_t index) const;

        virtual int compare (const ByteSlice *other) const;

        uint8_t operator[] (size_t index) const { return(fetch8(index)); }

        bool isEmpty (void) const { return(!length()); };

        int  compare (const ByteSlice& other) const { return(compare(&other)); }
        bool equal   (const ByteSlice& other) const { return(!compare(other)); }
        bool equal   (const ByteSlice *other) const { return(!compare(other)); }
};

inline bool operator==(const ByteSlice& a, const ByteSlice& b) {
    return(a.equal(b));
}

inline bool operator!=(const ByteSlice& a, const ByteSlice& b) {
    return(!a.equal(b));
}

inline uint16_t ByteSlice::fetch16 (size_t index) const {
    uint16_t x;
    uint8_t *p = (uint8_t *)&x;
    index <<= 1;
    p[0] = fetch8(index + 0);
    p[1] = fetch8(index + 1);
    return(x);
}

inline uint32_t ByteSlice::fetch32 (size_t index) const {
    uint32_t x;
    uint16_t *p = (uint16_t *)&x;
    index <<= 1;
    p[0] = fetch16(index + 0);
    p[1] = fetch16(index + 1);
    return(x);
}

inline uint64_t ByteSlice::fetch64 (size_t index) const {
    uint64_t x;
    uint32_t *p = (uint32_t *)&x;
    index <<= 1;
    p[0] = fetch32(index + 0);
    p[1] = fetch32(index + 1);
    return(x);
}

#endif /* !_BYTE_SLICE_H_ */

