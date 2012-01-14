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

#ifndef _SEEKABLE_H_
#define _SEEKABLE_H_

#include <stdint.h>

class Seekable {
    public:
        virtual int      seek   (uint64_t offset) = 0;
        virtual int      skip   (uint64_t n) = 0;
        virtual uint64_t tell   (void) = 0;
        virtual uint64_t length (void) = 0;
};

#endif /* !_SEEKABLE_H_ */

