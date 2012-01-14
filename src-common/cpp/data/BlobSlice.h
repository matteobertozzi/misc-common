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

#ifndef _BLOB_SLICE_H_
#define _BLOB_SLICE_H_

#include "ByteSlice.h"

class BlobSlice : public ByteSlice {
    public:
        BlobSlice() {
            _blob = NULL;
            _length = 0;
        }

        BlobSlice (const void *blob, size_t length) {
            _blob = (const uint8_t *)blob;
            _length = length;
        }

        size_t length (void) const { return(_length); }

        uint8_t fetch8 (size_t index) const { return(_blob[index]); }
        uint16_t fetch16 (size_t index) const { return(((uint16_t *)_blob)[index]); }
        uint32_t fetch32 (size_t index) const { return(((uint32_t *)_blob)[index]); }
        uint64_t fetch64 (size_t index) const { return(((uint64_t *)_blob)[index]); }

    private:
        const uint8_t *_blob;
        size_t _length;
};

#endif /* !_BLOB_SLICE_H_ */

