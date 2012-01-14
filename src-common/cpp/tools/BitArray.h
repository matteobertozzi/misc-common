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

#ifndef _BIT_ARRAY_H_
#define _BIT_ARRAY_H_

#include <stdint.h>

class BitArray {
    public:
        BitArray(size_t length) {
            _blocks = new uint64_t[(length + 63) >> 6];
            _length = length;
        }

        size_t length (void) const {
            return(_length);
        }

        void set (size_t index, bool value) {
            if (value)
                set(index);
            else
                unset(index);
        }

        void set (size_t index) {
            _blocks[index >> 6] |= (1 << (index & 63));
        }

        void unset (size_t index) {
            _blocks[index >> 6] &= ~(1 << (index & 63));
        }

        bool operator[] (size_t index) const {
            return(_blocks[index >> 6] & (1 << (index & 63)));
        }

    private:
        uint64_t *_blocks;
        size_t    _length;
};

#endif /* !_BIT_ARRAY_H_ */
