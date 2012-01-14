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

#include "BlobSlice.h"
#include <stdio.h>

int main (int argc, char **argv) {
    const char *text = "Sono un testo di prova! e contengo un testo di prova!";
    BlobSlice s1(text + 5, 18);
    BlobSlice s2(text + 35, 18);
    BlobSlice s3(text + 17, 5);
    BlobSlice s4;

    printf("1. Equal %d\n", s1.equal(s2));
    printf("1. Compare %d\n", s1.compare(s2));

    printf("2. Equal %d\n", s1.equal(s3));
    printf("2. Compare %d\n", s1.compare(s3));

    printf("3. Equal %d\n", s1.equal(s4));
    printf("3. Compare %d\n", s1.compare(s4));

    printf("4. Equal %d\n", s4.equal(s1));
    printf("4. Compare %d\n", s4.compare(s1));

    printf("5. Equal %d\n", s3.equal(s1));
    printf("5. Compare %d\n", s3.compare(s1));

    return(0);
}

